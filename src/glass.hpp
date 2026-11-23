#pragma once

// An in-window material. Alpha is composited over the application's own canvas.
namespace glass {
inline constexpr int menuCommand = 16410;
inline bool preferred = true;
inline bool systemAllows = true;
inline bool enabled() { return preferred && systemAllows; }

inline void refreshSettings() {
    auto setting = [](const wchar_t* path, const wchar_t* name) {
        DWORD value = 1, bytes = sizeof(value);
        return RegGetValueW(HKEY_CURRENT_USER, path, name, RRF_RT_REG_DWORD,
                            nullptr, &value, &bytes) != ERROR_SUCCESS || value != 0;
    };
    HIGHCONTRASTW contrast{sizeof(contrast)};
    const bool highContrast = SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) &&
                              (contrast.dwFlags & HCF_HIGHCONTRASTON);
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

inline void wash(HDC dc, const RECT& box, COLORREF canvas) {
    if (box.right<=box.left || box.bottom<=box.top) return;
    if (!enabled()) {
        const auto old=SetDCBrushColor(dc,canvas);
        FillRect(dc,&box,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        SetDCBrushColor(dc,old);
        return;
    }
    // Render the diffused background once per palette, independently of DPI and window size.
    constexpr int width=128,height=96;
    static std::array<DWORD,width*height> pixels{};
    static COLORREF lastCanvas=CLR_INVALID;
    static int lastTheme=-1;
    if (lastCanvas!=canvas || lastTheme!=Theme::index()) {
        lastCanvas=canvas; lastTheme=Theme::index();
        constexpr COLORREF lights[][2]={{RGB(222,184,148),RGB(234,212,180)},
            {RGB(135,206,180),RGB(177,211,163)}, {RGB(136,192,238),RGB(177,174,225)},
            {RGB(59,95,142),RGB(83,74,125)}};
        const auto& light=lights[lastTheme];
        for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
            const double u=double(x)/(width-1),v=double(y)/(height-1);
            const auto glow=[](double dx,double dy) { const double w=std::max(0.0,1-dx*dx-dy*dy); return w*w; };
            auto c=mix(canvas,light[0],0.58*glow((u-.04)/.72,(v-.08)/.78));
            c=mix(c,light[1],0.45*glow((u-.9)/.72,(v+.05)/.6));
            c=mix(c,light[0],0.20*glow((u-.74)/.82,(v-1.1)/.55));
            const double distance=v-(.30-.44*u+.10*std::sin(5*u));
            const double ribbon=std::exp(-distance*distance*115);
            c=mix(c,Theme::dark()?Theme::blue:RGB(255,255,255),(Theme::dark()?.06:.30)*ribbon);
            pixels[y*width+x]=RGB(GetBValue(c),GetGValue(c),GetRValue(c));
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

inline void surface(HDC dc,const RECT& box,COLORREF tint,int radiusPixels,bool active=false) {
    const int w=box.right-box.left,h=box.bottom-box.top;
    if (w<2 || h<2) return;
    const int diameter=std::clamp(2*radiusPixels,2,std::min(w,h));
    const int saved=SaveDC(dc);
    SelectObject(dc,GetStockObject(DC_PEN));
    SelectObject(dc,GetStockObject(DC_BRUSH));
    SetDCPenColor(dc,Theme::line); SetDCBrushColor(dc,tint);
    if (!enabled()) {
        RoundRect(dc,box.left,box.top,box.right,box.bottom,diameter,diameter);
        RestoreDC(dc,saved);return;
    }
    // A tiny optical gradient is stretched inside the rounded contour. Use constant
    // alpha so the local backdrop contributes to the material even on hover/press.
    HDC layer=CreateCompatibleDC(dc);
    HBITMAP tile=CreateCompatibleBitmap(dc,48,48);
    if (layer && tile) {
        auto previous=SelectObject(layer,tile);
        auto vertex=[](LONG x,LONG y,COLORREF color) -> TRIVERTEX {
            return {x,y,static_cast<COLOR16>(GetRValue(color)*257),static_cast<COLOR16>(GetGValue(color)*257),
                    static_cast<COLOR16>(GetBValue(color)*257),0};
        };
        const auto white=RGB(255,255,255);
        const bool darkTint=(GetRValue(tint)*3+GetGValue(tint)*6+GetBValue(tint))<1400;
        const auto gleam=mix(tint,white,darkTint?.16:.65);
        const auto shade=mix(tint,Theme::accent,darkTint?.12:.19);
        TRIVERTEX vertices[]={vertex(0,0,gleam),vertex(48,0,mix(gleam,tint,.55)),
                              vertex(0,48,shade),vertex(48,48,mix(tint,gleam,.38))};
        GRADIENT_TRIANGLE faces[]={{0,1,2},{1,2,3}};
        auto region=CreateRoundRectRgn(box.left,box.top,box.right+1,box.bottom+1,diameter,diameter);
        const int clipped=SaveDC(dc);
        ExtSelectClipRgn(dc,region,RGN_AND);
        // Diffuse a slightly enlarged sample of this pane's own backdrop.
        // Bound the sampling cost even for a sidebar filling a 4K screen.
        const int inset=std::min({radiusPixels/3,w/8,h/8});
        if(inset>0){
            SetStretchBltMode(layer,HALFTONE);SetBrushOrgEx(layer,0,0,nullptr);
            if(StretchBlt(layer,0,0,48,48,dc,box.left+inset,box.top+inset,w-2*inset,h-2*inset,SRCCOPY))
                GdiAlphaBlend(dc,box.left,box.top,w,h,layer,0,0,48,48,{AC_SRC_OVER,0,115,0});
        }
        GdiGradientFill(layer,vertices,4,faces,2,GRADIENT_FILL_TRIANGLE);
        BLENDFUNCTION blend{AC_SRC_OVER,0,static_cast<BYTE>(active?219:158),0};
        if(!GdiAlphaBlend(dc,box.left,box.top,w,h,layer,0,0,48,48,blend))
            FillRect(dc,&box,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        RestoreDC(dc,clipped);
        DeleteObject(region);
        SelectObject(layer,previous);
        SelectObject(dc,GetStockObject(NULL_BRUSH));
        SetDCPenColor(dc,mix(tint,Theme::dark()?white:Theme::accentInk,Theme::dark()?.34:.35));
        RoundRect(dc,box.left,box.top,box.right,box.bottom,diameter,diameter);
        // The upper inner lip catches light, leaving the lower edge in shade.
        if(w>6 && h>6) {
            const int lip=SaveDC(dc);
            IntersectClipRect(dc,box.left,box.top,box.right,box.top+h/2);
            SetDCPenColor(dc,mix(tint,white,darkTint?.32:.90));
            RoundRect(dc,box.left+1,box.top+1,box.right-1,box.bottom-1,std::max(2,diameter-2),std::max(2,diameter-2));
            RestoreDC(dc,lip);
        }
    } else RoundRect(dc,box.left,box.top,box.right,box.bottom,diameter,diameter);
    if(tile)DeleteObject(tile);
    if(layer)DeleteDC(layer);
    RestoreDC(dc,saved);
}
} // namespace glass
