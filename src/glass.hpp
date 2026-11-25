#pragma once

// An in-window material. Alpha is composited over the application's own canvas.
namespace glass {
inline constexpr int menuCommand = 16410;
inline bool preferred = true;
inline bool systemAllows = true;
inline bool motionAllowed = true;
inline bool enabled() { return preferred && systemAllows; }
inline bool followsPointer() { return enabled() && motionAllowed; }

inline void refreshSettings() {
    auto setting = [](const wchar_t* path, const wchar_t* name) {
        DWORD value = 1, bytes = sizeof(value);
        return RegGetValueW(HKEY_CURRENT_USER, path, name, RRF_RT_REG_DWORD,
                            nullptr, &value, &bytes) != ERROR_SUCCESS || value != 0;
    };
    HIGHCONTRASTW contrast{sizeof(contrast)};
    const bool highContrast = SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) &&
                              (contrast.dwFlags & HCF_HIGHCONTRASTON);
    BOOL animations=TRUE;
    motionAllowed=!SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&animations,0) || animations;
    preferred = setting(Theme::settingsKey, L"GlassEffects");
    systemAllows = !highContrast && setting(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"EnableTransparency");
}
inline bool toggle() {
    const DWORD value = preferred ? 0 : 1;
    if (RegSetKeyValueW(HKEY_CURRENT_USER, Theme::settingsKey, L"GlassEffects",
                        REG_DWORD, &value, sizeof(value)) != ERROR_SUCCESS) return false;
    refreshSettings();
    return true;
}
inline const wchar_t* label() {
    static constexpr const wchar_t* names[] = {L"液态玻璃",L"液態玻璃",L"Liquid Glass",L"リキッドグラス",
        L"리퀴드 글라스",L"Liquid Glass",L"Liquid Glass",L"Liquid Glass",L"Liquid Glass",L"Liquid Glass",L"Liquid Glass"};
    return names[i18n::language];
}

inline COLORREF mix(COLORREF a, COLORREF b, double fraction) {
    const auto channel = [fraction](int x, int y) { return static_cast<int>(x+(y-x)*fraction+0.5); };
    return RGB(channel(GetRValue(a),GetRValue(b)),channel(GetGValue(a),GetGValue(b)),channel(GetBValue(a),GetBValue(b)));
}

inline double bump(double d) { return std::exp(-d*d); }
inline DWORD rgbPixel(COLORREF c) {
    return (DWORD(GetRValue(c))<<16) | (DWORD(GetGValue(c))<<8) | GetBValue(c);
}

inline void wash(HDC dc,const RECT& box,COLORREF canvas) {
    if (box.right<=box.left || box.bottom<=box.top) return;
    if (!enabled()) {
        const auto old=SetDCBrushColor(dc,canvas);
        FillRect(dc,&box,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        SetDCBrushColor(dc,old);
        return;
    }
    // The crest changes width along the arc, like a sheet curving toward the light.
    // It is part of the backdrop, so adjacent panes catch different parts of it.
    constexpr int width=256,height=176;
    static std::array<DWORD,width*height> pixels{};
    static COLORREF lastCanvas=CLR_INVALID;
    static int lastTheme=-1;
    if (lastCanvas!=canvas || lastTheme!=Theme::index()) {
        lastCanvas=canvas; lastTheme=Theme::index();
        constexpr COLORREF lights[][3]={{RGB(217,174,142),RGB(219,191,173),RGB(248,233,206)},
            {RGB(128,191,169),RGB(146,173,164),RGB(229,241,203)},
            {RGB(128,195,211),RGB(167,169,220),RGB(215,241,247)},
            {RGB(49,104,122),RGB(83,80,135),RGB(112,160,178)}};
        const auto& light=lights[lastTheme];
        const bool dark=Theme::dark();
        for (int x=0;x<width;++x) {
            const double u=double(x)/(width-1);
            const double crest=.10+.30*u-.54*u*u+.040*std::sin(5.2*u-.45);
            const double second=.58-.61*u+.08*std::sin(4.8*u);
            for (int y=0;y<height;++y) {
                const double v=double(y)/(height-1);
                const auto glow=[](double x,double y) { return std::exp(-x*x-y*y); };
                auto c=mix(canvas,light[0],.67*glow((u+.06)/.51,(v-.12)/.66));
                c=mix(c,light[1],.70*glow((u-.69)/.46,(v+.08)/.53));
                c=mix(c,light[2],.34*glow((u-.08)/.7,(v-.85)/.52));
                const double d=v-crest;
                c=mix(c,light[1],.27*bump((d-.061)/.083));
                c=mix(c,light[2],(dark?.36:.58)*bump((d+.009)/(.023+.039*u)));
                c=mix(c,dark?light[2]:RGB(255,255,255),.28*bump(d/.012)*bump((u-.31)/.69));
                c=mix(c,light[2],.23*bump((v-second)/(.043+.042*u))*bump((u-.82)/.46));
                pixels[y*width+x]=rgbPixel(c);
            }
        }
    }
    BITMAPINFO info{};
    info.bmiHeader={sizeof(BITMAPINFOHEADER),width,-height,1,32,BI_RGB};
    const int saved=SaveDC(dc);
    SetStretchBltMode(dc,HALFTONE); SetBrushOrgEx(dc,0,0,nullptr);
    StretchDIBits(dc,box.left,box.top,box.right-box.left,box.bottom-box.top,0,0,width,height,
                  pixels.data(),&info,DIB_RGB_COLORS,SRCCOPY);
    RestoreDC(dc,saved);
}

struct Dib {
    HDC dc=nullptr;
    HBITMAP bitmap=nullptr;
    HGDIOBJ original=nullptr;
    DWORD* pixels=nullptr;
    int width=0,height=0;
    Dib()=default;
    Dib(const Dib&)=delete;
    Dib& operator=(const Dib&)=delete;
    ~Dib() { clear(); }
    void clear() {
        if(original) SelectObject(dc,original);
        if(bitmap) DeleteObject(bitmap);
        if(dc) DeleteDC(dc);
        dc=nullptr; bitmap=nullptr; original=nullptr; pixels=nullptr; width=height=0;
    }
    bool size(HDC source,int w,int h) {
        if(dc && width==w && height==h) return true;
        clear();
        dc=CreateCompatibleDC(source);
        BITMAPINFO info{};
        info.bmiHeader={sizeof(BITMAPINFOHEADER),w,-h,1,32,BI_RGB};
        void* data=nullptr;
        if(dc) bitmap=CreateDIBSection(source,&info,DIB_RGB_COLORS,&data,nullptr,0);
        if(!bitmap || !data) { clear(); return false; }
        original=SelectObject(dc,bitmap);
        if(!original || original==HGDI_ERROR) { original=nullptr; clear(); return false; }
        pixels=static_cast<DWORD*>(data); width=w; height=h;
        return true;
    }
};

// A translucent, premultiplied pigment mixed over the existing rim layers.
inline void pigment(double& a,double& red,double& green,double& blue,COLORREF c,double coverage) {
    const double k=std::clamp(coverage,0.,1.);
    red=red*(1-k)+GetRValue(c)*k; green=green*(1-k)+GetGValue(c)*k;
    blue=blue*(1-k)+GetBValue(c)*k; a=a*(1-k)+255*k;
}
inline DWORD argb(double a,double r,double g,double b) {
    return (DWORD(a+.5)<<24)|(DWORD(r+.5)<<16)|(DWORD(g+.5)<<8)|DWORD(b+.5);
}

// Signed distance and outward normal of a rounded rectangle, in surface pixels.
inline double contour(double x,double y,double halfW,double halfH,double radius,double& nx,double& ny) {
    const double qx=std::abs(x)-halfW+radius,qy=std::abs(y)-halfH+radius;
    const double dx=std::max(qx,0.),dy=std::max(qy,0.);
    const double distance=std::sqrt(dx*dx+dy*dy);
    nx=distance>0?dx/distance:(qx>qy?1.:0.);
    ny=distance>0?dy/distance:(qy>=qx?1.:0.);
    if(x<0) nx=-nx;
    if(y<0) ny=-ny;
    return distance+std::min(std::max(qx,qy),0.)-radius;
}

struct Rim {
    Dib image;
    int radius=0,pad=0,palette=-1;
    bool dark=false;
};

inline void rim(HDC dc,const RECT& box,int radius,bool darkTint) {
    constexpr int middle=35;
    static std::array<Rim,12> cache{};
    static size_t next=0;
    Rim* found=nullptr;
    for(auto& entry:cache) {
        if(entry.radius==radius && entry.palette==Theme::index() && entry.dark==darkTint) { found=&entry; break; }
    }
    if(!found) {
        Rim& entry=cache[next++%cache.size()];
        const double scale=std::clamp(radius/17.,.8,2.6);
        const int pad=int(std::ceil(5*scale)),half=pad+radius;
        const int size=2*half+middle;
        if(!entry.image.size(dc,size,size)) { entry.palette=-1; return; }
        entry.radius=radius; entry.pad=pad; entry.palette=Theme::index(); entry.dark=darkTint;
        const COLORREF shadow=mix(Theme::accent,RGB(9,18,29),.78);
        const COLORREF reflection=mix(Theme::accent,RGB(251,253,255),.77);
        GdiFlush();
        for(int y=0;y<size;++y) for(int x=0;x<size;++x) {
            double nx,ny;
            const double extent=radius+middle*.5;
            const double d=contour(x-(size-1)*.5,y-(size-1)*.5,extent,extent,radius,nx,ny);
            // Give long edges a broad pool of light instead of stretching a uniform line.
            const double u=std::clamp(double(x-half)/(middle-1),0.,1.);
            const double v=std::clamp(double(y-half)/(middle-1),0.,1.);
            const double key=.10+.90*bump((u-.22)/(darkTint?.24:.40))*bump((v-.13)/(darkTint?.41:.58));
            const double reflected=.10+.90*bump((u-.76)/(darkTint?.36:.52))*bump((v-.89)/.55);
            const double sun=std::max(0.,-.54*nx-.84*ny)*key;
            const double bounce=std::max(0.,.57*nx+.82*ny)*reflected;
            double a=0,r=0,g=0,b=0;
            if(d>0) {
                const double below=std::max(0.,ny);
                pigment(a,r,g,b,shadow,(.025+.14*below)*bump(d/((1.4+1.7*below)*scale)));
            }
            // The fine dark silhouette sits outside the specular lip. Keeping
            // them apart leaves the sides clear where neither light strikes.
            const double depth=-d/scale;
            pigment(a,r,g,b,shadow,(darkTint?.64:.35)*bump((depth-.03)/.70));
            pigment(a,r,g,b,shadow,(darkTint?.13:.075)*bump((depth-2.45)/1.20));
            pigment(a,r,g,b,reflection,(darkTint?.30:.24)*bounce*bump((depth-3.0)/1.75));
            const double specular=darkTint?.035+.75*sun+.43*bounce:.055+.92*sun+.56*bounce;
            pigment(a,r,g,b,RGB(255,255,255),specular*bump((depth-1.12)/(darkTint?.65:.75)));
            entry.image.pixels[y*size+x]=argb(a,r,g,b);
        }
        found=&entry;
    }
    const int half=radius+found->pad,edge=2*half+middle;
    const int src[]={0,half,half+middle,edge};
    const int xs[]={int(box.left)-found->pad,int(box.left)+radius,int(box.right)-radius,int(box.right)+found->pad};
    const int ys[]={int(box.top)-found->pad,int(box.top)+radius,int(box.bottom)-radius,int(box.bottom)+found->pad};
    for(int y=0;y<3;++y) for(int x=0;x<3;++x) {
        if((x==1 && y==1) || xs[x]==xs[x+1] || ys[y]==ys[y+1]) continue;
        GdiAlphaBlend(dc,xs[x],ys[y],xs[x+1]-xs[x],ys[y+1]-ys[y],found->image.dc,
                      src[x],src[y],src[x+1]-src[x],src[y+1]-src[y],{AC_SRC_OVER,0,255,AC_SRC_ALPHA});
    }
}

// Stretch the center once. Alpha-compose the four small corners separately,
// preserving the real backdrop at subpixel coverage along their silhouettes.
inline bool paintLens(HDC dc,const RECT& box,int radius,Dib& image) {
    const int w=box.right-box.left,h=box.bottom-box.top;
    static Dib corners;
    if(!corners.size(dc,4*radius,radius)) return false;
    const int cols=image.width,rows=image.height;
    GdiFlush();
    for(int y=0;y<radius;++y) for(int x=0;x<radius;++x) {
        const double dx=radius-.5-x,dy=radius-.5-y;
        const double coverage=std::clamp(radius+.5-std::sqrt(dx*dx+dy*dy),0.,1.);
        for(int q=0;q<4;++q) {
            const int xx=q&1?w-1-x:x,yy=q&2?h-1-y:y;
            const int sx=std::min(cols-1,int((xx+.5)*cols/w)),sy=std::min(rows-1,int((yy+.5)*rows/h));
            const DWORD color=image.pixels[sy*cols+sx];
            const int u=q&1?radius-1-x:x,v=q&2?radius-1-y:y;
            corners.pixels[v*corners.width+q*radius+u]=argb(255*coverage,((color>>16)&255)*coverage,
                                                            ((color>>8)&255)*coverage,(color&255)*coverage);
        }
    }
    const int saved=SaveDC(dc);
    ExcludeClipRect(dc,box.left,box.top,box.left+radius,box.top+radius);
    ExcludeClipRect(dc,box.right-radius,box.top,box.right,box.top+radius);
    ExcludeClipRect(dc,box.left,box.bottom-radius,box.left+radius,box.bottom);
    ExcludeClipRect(dc,box.right-radius,box.bottom-radius,box.right,box.bottom);
    SetStretchBltMode(dc,HALFTONE); SetBrushOrgEx(dc,0,0,nullptr);
    const bool painted=StretchBlt(dc,box.left,box.top,w,h,image.dc,0,0,cols,rows,SRCCOPY)!=0;
    RestoreDC(dc,saved);
    if(!painted) return false;
    for(int q=0;q<4;++q) {
        const int left=q&1?box.right-radius:box.left,top=q&2?box.bottom-radius:box.top;
        GdiAlphaBlend(dc,left,top,radius,radius,corners.dc,q*radius,0,radius,radius,{AC_SRC_OVER,0,255,AC_SRC_ALPHA});
    }
    return true;
}

inline void surface(HDC dc,const RECT& box,COLORREF tint,int radiusPixels,bool active=false) {
    const int w=box.right-box.left,h=box.bottom-box.top;
    if(w<2 || h<2) return;
    const int radius=std::clamp(radiusPixels,1,std::min({w/2,h/2,72}));
    const int saved=SaveDC(dc);
    SelectObject(dc,GetStockObject(DC_PEN)); SelectObject(dc,GetStockObject(DC_BRUSH));
    SetDCPenColor(dc,Theme::line); SetDCBrushColor(dc,tint);
    constexpr int cols=80,rows=64;
    static Dib image;
    if(!enabled() || !image.size(dc,cols,rows)) {
        RoundRect(dc,box.left,box.top,box.right,box.bottom,2*radius,2*radius);
        RestoreDC(dc,saved); return;
    }
    const auto white=RGB(255,255,255);
    const bool darkTint=(GetRValue(tint)*3+GetGValue(tint)*6+GetBValue(tint))<1400;
    SetStretchBltMode(image.dc,HALFTONE); SetBrushOrgEx(image.dc,0,0,nullptr);
    const bool sampled=StretchBlt(image.dc,0,0,cols,rows,dc,box.left,box.top,w,h,SRCCOPY)!=0;
    GdiFlush();
    std::array<DWORD,cols*rows> backdrop{};
    if(sampled) std::copy_n(image.pixels,cols*rows,backdrop.begin());
    else backdrop.fill(rgbPixel(tint));
    // The bevel bends nearby colors inward. Limit sampling to the local material
    // and taper displacement to zero across the flat optical center.
    auto refract=[](int i,int count,int extent,int radius) {
        const int edgeDistance=std::min(i,count-1-i);
        const double thickness=std::max(1.,double(radius)*count/extent);
        const double falloff=std::max(0.,1.-edgeDistance/(1.7*thickness));
        const int shift=int(std::min(count*.15,thickness*.61)*falloff*falloff+.5);
        return std::clamp(i+(i<count/2?shift:-shift),0,count-1);
    };
    std::array<int,cols> sx{};
    for(int x=0;x<cols;++x) sx[x]=refract(x,cols,w,radius);
    const auto crest=mix(tint,white,darkTint?(active?.07:.14):.68);
    const auto base=mix(tint,Theme::accent,darkTint?.075:.09);
    const unsigned density=active?(darkTint?244:202):(darkTint?150:112);
    // Diffuse the already downsampled backdrop across the shoulder and keep
    // its refracted color. Reading the four surrounding samples also softens
    // hard background transitions as they pass under a floating control.
    auto averagePixel=[](DWORD p,DWORD q) {
        return (p&q)+(((p^q)&0x00fefefeu)>>1);
    };
    for(int y=0;y<rows;++y) {
        const int sy=refract(y,rows,h,radius);
        const DWORD* scan=backdrop.data()+sy*cols;
        const DWORD* above=backdrop.data()+std::max(0,sy-1)*cols;
        const DWORD* below=backdrop.data()+std::min(rows-1,sy+1)*cols;
        const double v=double(y)/(rows-1);
        const auto left=mix(base,crest,.14+.64*(1.-v)*(1.-v));
        const auto right=mix(base,crest,.12+.27*v);
        for(int x=0;x<cols;++x) {
            const int ix=sx[x];
            const DWORD horizontal=averagePixel(scan[std::max(0,ix-1)],scan[std::min(cols-1,ix+1)]);
            const DWORD vertical=averagePixel(above[ix],below[ix]);
            const DWORD sample=averagePixel(scan[ix],averagePixel(horizontal,vertical));
            const COLORREF local=RGB((sample>>16)&255,(sample>>8)&255,sample&255);
            const auto color=mix(left,right,double(x)/(cols-1));
            image.pixels[y*cols+x]=rgbPixel(mix(local,color,density/255.));
        }
    }
    if(!paintLens(dc,box,radius,image)) RoundRect(dc,box.left,box.top,box.right,box.bottom,2*radius,2*radius);
    RestoreDC(dc,saved);
    rim(dc,box,radius,darkTint);
}

// Highlight one action within a shared toolbar. The translucent fill keeps
// its parent's reflection continuous through the button. Reuse the mask as
// the pointer moves; the opacity changes independently when the button presses.
inline void tintSegment(HDC dc,const RECT& box,int radiusPixels,COLORREF tint,BYTE opacity) {
    const int w=box.right-box.left,h=box.bottom-box.top;
    if(w<2 || h<2) return;
    const int radius=std::clamp(radiusPixels,1,std::min({w/2,h/2,72}));
    const int stampWidth=2*radius+1;
    static Dib stamp;
    static COLORREF lastTint=CLR_INVALID;
    if(stamp.width!=stampWidth || stamp.height!=h || tint!=lastTint) {
        if(!stamp.size(dc,stampWidth,h)) return;
        GdiFlush();
        for(int y=0;y<h;++y) for(int x=0;x<stampWidth;++x) {
            const double dx=std::max(0.,std::abs(x-radius)-.5);
            const double dy=std::max(0.,radius-std::min(y,h-1-y)-.5);
            const double coverage=std::clamp(radius+.5-std::hypot(dx,dy),0.,1.);
            stamp.pixels[y*stampWidth+x]=argb(255*coverage,GetRValue(tint)*coverage,
                                                GetGValue(tint)*coverage,GetBValue(tint)*coverage);
        }
        lastTint=tint;
    }
    const BLENDFUNCTION blend{AC_SRC_OVER,0,opacity,AC_SRC_ALPHA};
    GdiAlphaBlend(dc,box.left,box.top,radius,h,stamp.dc,0,0,radius,h,blend);
    if(w>2*radius)
        GdiAlphaBlend(dc,box.left+radius,box.top,w-2*radius,h,stamp.dc,radius,0,1,h,blend);
    GdiAlphaBlend(dc,box.right-radius,box.top,radius,h,stamp.dc,radius+1,0,radius,h,blend);
}

// A pointer lights only the bevel close to it. The stationary material remains
// unchanged and the patch has a fixed cost independent of the pane's area.
inline void glint(HDC dc,const RECT& box,int radiusPixels,POINT light,bool pressed=false) {
    if(!followsPointer() || !PtInRect(&box,light)) return;
    const int w=box.right-box.left,h=box.bottom-box.top;
    if(w<2 || h<2) return;
    const int radius=std::clamp(radiusPixels,1,std::min({w/2,h/2,72}));
    const double scale=std::clamp(radius/16.,.8,2.);
    const int reach=int(30*scale);
    const int left=std::max(int(box.left),int(light.x)-reach),top=std::max(int(box.top),int(light.y)-reach);
    const int right=std::min(int(box.right),int(light.x)+reach+1),bottom=std::min(int(box.bottom),int(light.y)+reach+1);
    static Dib patch;
    if(!patch.size(dc,2*reach+1,2*reach+1)) return;
    GdiFlush();
    for(int y=top;y<bottom;++y) for(int x=left;x<right;++x) {
        const double dx=x-light.x,dy=y-light.y;
        const double radial=std::max(0.,1.-(dx*dx+dy*dy)/(reach*reach));
        double nx,ny;
        const double d=contour(x-box.left+.5-w/2.,y-box.top+.5-h/2.,w/2.,h/2.,radius,nx,ny);
        // A pressed lens contracts. Its reflection moves just inside the lip
        // and broadens without whitening the label at the center of the button.
        const double depth=-d/scale;
        const double bevel=std::max(0.,1.-std::abs(depth-(pressed?2.35:1.05))/(pressed?3.1:1.95));
        const BYTE alpha=BYTE((pressed?171:239)*radial*radial*bevel);
        patch.pixels[(y-top)*patch.width+x-left]=(DWORD(alpha)*0x01010101u);
    }
    GdiAlphaBlend(dc,left,top,right-left,bottom-top,patch.dc,0,0,right-left,bottom-top,
                  {AC_SRC_OVER,0,255,AC_SRC_ALPHA});
}
} // namespace glass
