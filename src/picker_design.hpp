#pragma once
// Drawing for the save picker. All dimensions use the dialog's own monitor DPI.
inline void surface(const State& s,HDC dc,RECT box,COLORREF color,COLORREF edge,int radius,int weight=1){
    HBRUSH brush=CreateSolidBrush(color);
    HPEN pen=CreatePen(PS_SOLID,std::max(1,s.scale(weight)),edge);
    auto oldBrush=SelectObject(dc,brush),oldPen=SelectObject(dc,pen);
    const int diameter=s.scale(glass::enabled()?radius:Theme::radius(radius));
    RoundRect(dc,box.left,box.top,box.right,box.bottom,diameter,diameter);
    SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);DeleteObject(brush);
}
inline void lettering(HDC dc,const std::wstring& caption,RECT box,HFONT font,COLORREF color,UINT align){
    auto old=SelectObject(dc,font);
    SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);
    DrawTextW(dc,caption.c_str(),(int)caption.size(),&box,align|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
    SelectObject(dc,old);
}
inline int textWidth(HDC dc,const std::wstring& caption,HFONT font){
    SIZE extent{};auto previous=SelectObject(dc,font);
    GetTextExtentPoint32W(dc,caption.c_str(),static_cast<int>(caption.size()),&extent);
    SelectObject(dc,previous);return extent.cx;
}
inline COLORREF campaignInk(int icon){
    if(icon==DEMO_MAIN)return Theme::blue;
    if(icon==DEMO_RIZIA)return Theme::dark()?RGB(224,191,149):RGB(126,83,44);
    return Theme::accentInk;
}
inline int fileIconInset(const State& s){
    return ListView_GetColumnWidth(s.list,0)>=s.scale(186)?s.scale(35):0;
}
inline void symbol(const State& s,HDC dc,int id,int x,int y,COLORREF color){
    using icons::Symbol;
    Symbol icon;
    switch (id) {
        case FILTER: icon=Symbol::Search; break;
        case BROWSE: case ADD_FOLDER: icon=Symbol::Folder; break;
        case RESCAN: icon=Symbol::Reload; break;
        case DEMO_MAIN: icon=Symbol::Inventory; break;
        case DEMO_RIZIA: icon=Symbol::Crown; break;
        case LIVE_MODE: icon=Symbol::Pulse; break;
        case IDOK: icon=Symbol::Arrow; break;
        case LIST: icon=Symbol::Document; break;
        default: icon=Symbol::Archive; break;
    }
    icons::draw(dc,icon,x,y,s.scale(18),color);
}
inline void focusRing(const State& s,HDC dc,RECT box,COLORREF ink,int diameter){
    const int saved=SaveDC(dc);
    HPEN pen=CreatePen(PS_SOLID,std::max(1,s.scale(2)),ink);
    SelectObject(dc,pen);SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
    RoundRect(dc,box.left,box.top,box.right,box.bottom,s.scale(diameter),s.scale(diameter));
    RestoreDC(dc,saved);DeleteObject(pen);
}
inline void archiveMark(const State& s,HDC dc){
    const RECT mark=s.headingIcon;
    const auto base=Theme::accentInk;
    auto rect=[&](int x,int y,int w,int h){return RECT{mark.left+s.scale(x),mark.top+s.scale(y),mark.left+s.scale(x+w),mark.top+s.scale(y+h)};};
    const auto mid=glass::mix(Theme::panel,base,.36);
    const auto front=Theme::dark()?Theme::select:Theme::panel;
    // Three overlapping folios identify the library at title size.
    surface(s,dc,rect(8,5,22,26),mid,mid,7);
    surface(s,dc,rect(11,9,23,26),glass::mix(mid,front,.4),mid,7);
    surface(s,dc,rect(14,13,22,25),front,base,7);
    ::fill(dc,rect(18,21,12,2),base);
    ::fill(dc,rect(18,26,8,1),mid);
}
// Child windows sample this same canvas so their corners reveal the material beneath them.
inline void drawBackdrop(const State& s,HDC dc,const RECT& client){
    const bool material=glass::enabled();
    if(material)glass::wash(dc,client,Theme::bg);
    else ::fill(dc,client,Theme::bg);
    surface(s,dc,s.library,Theme::panel,Theme::line,22);
    // The live entry is its own lens. The practice instructions and the
    // save table have a stable reading surface, even on the clear finish.
    RECT live{};
    GetWindowRect(s.live,&live);
    MapWindowPoints(nullptr,s.win,reinterpret_cast<POINT*>(&live),2);
    surface(s,dc,s.practice,Theme::panel,Theme::line,28);
    const COLORREF liveTint=glass::mix(Theme::sidebar,Theme::accent,Theme::dark()?.09:.12);
    glass::surface(dc,live,liveTint,s.scale(s.stacked?20:24));
    if(material){
        glass::surface(dc,s.headerTools,Theme::sidebar,s.scale(20));
        RECT appearance{};GetWindowRect(s.appearance,&appearance);
        MapWindowPoints(nullptr,s.win,reinterpret_cast<POINT*>(&appearance),2);
        const int mid=(s.headerTools.top+s.headerTools.bottom)/2;
        RECT split{appearance.right+s.scale(3),mid-s.scale(7),appearance.right+s.scale(3)+1,mid+s.scale(7)};
        ::fill(dc,split,glass::mix(Theme::line,Theme::muted,.2));
    }
    if(!s.stacked){
        RECT demo{};GetWindowRect(s.riziaDemo,&demo);
        MapWindowPoints(nullptr,s.win,reinterpret_cast<POINT*>(&demo),2);
        RECT divider{demo.left+s.scale(52),demo.top,demo.right-s.scale(9),demo.top+1};
        ::fill(dc,divider,Theme::line);
    }
    if(!IsRectEmpty(&s.headingIcon))archiveMark(s,dc);
    const int inset=material?s.library.left:0;
    RECT divider{inset,s.scale(s.footerY),client.right-inset,s.scale(s.footerY)+1};
    ::fill(dc,divider,Theme::line);
}
inline HDC backdrop(State& s,HDC reference){
    if(!s.empty || s.positioning || s.backdrop)return s.backdrop;
    RECT client{};GetClientRect(s.win,&client);
    if(client.right<=0 || client.bottom<=0)return nullptr;
    auto dc=CreateCompatibleDC(reference);
    if(!dc)return nullptr;
    auto bmp=CreateCompatibleBitmap(reference,client.right,client.bottom);
    if(!bmp){DeleteDC(dc);return nullptr;}
    s.previousBitmap=SelectObject(dc,bmp);
    drawBackdrop(s,dc,client);
    s.backdropBitmap=bmp;
    s.backdrop=dc;
    return dc;
}
inline void paintFrame(State& s,HDC dc,const RECT& dirty){
    if(auto cached=backdrop(s,dc))
        BitBlt(dc,dirty.left,dirty.top,dirty.right-dirty.left,dirty.bottom-dirty.top,cached,dirty.left,dirty.top,SRCCOPY);
    else ::fill(dc,dirty,Theme::bg);
    surface(s,dc,s.searchFrame,Theme::input,GetFocus()==s.filter?Theme::focus:Theme::line,glass::enabled()?24:10,GetFocus()==s.filter?2:1);
    symbol(s,dc,FILTER,s.searchFrame.left+s.scale(8),s.searchFrame.top+s.scale(6),Theme::muted);
}
inline void childBackdrop(State& s,const DRAWITEMSTRUCT& d){
    RECT r=d.rcItem;
    POINT position{r.left,r.top};
    MapWindowPoints(d.hwndItem,s.win,&position,1);
    if(auto cached=backdrop(s,d.hDC))
        BitBlt(d.hDC,r.left,r.top,r.right-r.left,r.bottom-r.top,cached,position.x,position.y,SRCCOPY);
    else ::fill(d.hDC,r,Theme::bg);
}
inline void drawCaption(State& s,const DRAWITEMSTRUCT& d){
    childBackdrop(s,d);
    const bool hint=d.hwndItem==s.practiceHint, path=d.hwndItem==s.path;
    const auto old=SelectObject(d.hDC,reinterpret_cast<HFONT>(SendMessageW(d.hwndItem,WM_GETFONT,0,0)));
    SetTextColor(d.hDC,hint||path?Theme::muted:Theme::text);
    SetBkMode(d.hDC,TRANSPARENT);
    RECT bounds=d.rcItem;
    const auto text=textOf(d.hwndItem);
    DrawTextW(d.hDC,text.c_str(),static_cast<int>(text.size()),&bounds,DT_NOPREFIX|
        (hint?DT_WORDBREAK:DT_SINGLELINE|DT_VCENTER|(path?DT_PATH_ELLIPSIS:DT_END_ELLIPSIS)));
    SelectObject(d.hDC,old);
}
inline void chevron(const State& s,HDC dc,int x,int y,COLORREF color){
    HPEN pen=CreatePen(PS_SOLID,std::max(1,s.scale(1)),color);
    auto old=SelectObject(dc,pen);
    MoveToEx(dc,x-s.scale(4),y-s.scale(2),nullptr);
    LineTo(dc,x,y+s.scale(2));
    LineTo(dc,x+s.scale(4),y-s.scale(2));
    SelectObject(dc,old);DeleteObject(pen);
}

inline void drawLiveEntry(const State& s,HDC dc,RECT box,const std::wstring& title,COLORREF ink){
    const int centerX=(box.left+box.right)/2,centerY=(box.top+box.bottom)/2;
    const int iconX=s.stacked?centerX:box.left+s.scale(31);
    const int iconY=s.stacked?box.top+s.scale(21):centerY;
    const int half=s.scale(s.stacked?14:18);
    RECT disk{iconX-half,iconY-half,iconX+half,iconY+half};
    surface(s,dc,disk,Theme::accent,Theme::accent,36);
    symbol(s,dc,LIVE_MODE,iconX-s.scale(9),iconY-s.scale(9),Theme::onAccent);

    RECT heading=box;
    if(s.stacked){
        heading.left+=s.scale(5);heading.right-=s.scale(5);
        heading.top=iconY+half+s.scale(1);heading.bottom=heading.top+s.scale(21);
    }else{
        heading.left=disk.right+s.scale(10);heading.right-=s.scale(10);
        heading.top=centerY-s.scale(20);heading.bottom=centerY+s.scale(1);
    }
    const UINT align=s.stacked?DT_CENTER:DT_LEFT;
    const HFONT face=textWidth(dc,title,s.emphasis)<=heading.right-heading.left?s.emphasis:s.small;
    lettering(dc,title,heading,face,ink,align);

    // Keep the game's localized context, with the chord in one keycap.
    // In the narrow layout both fit on separate centered lines.
    const auto context=live_setup::inGameLabel();
    const auto keys=live_setup::panelShortcut;
    RECT line=heading;
    line.top=heading.bottom;line.bottom=line.top+s.scale(20);
    const int keyWidth=textWidth(dc,keys,s.small)+s.scale(12);
    const int contextWidth=textWidth(dc,context,s.small);
    RECT contextBox=line;
    if(s.stacked)contextBox.bottom=contextBox.top+s.scale(16);
    else contextBox.right=contextBox.left+contextWidth;
    lettering(dc,context,contextBox,s.small,Theme::muted,align);
    const int x=s.stacked?centerX-keyWidth/2:contextBox.right+s.scale(6);
    const int y=s.stacked?contextBox.bottom+s.scale(1):line.top+s.scale(1);
    RECT key{x,y,x+keyWidth,y+s.scale(18)};
    const auto keyFace=glass::mix(Theme::panel,Theme::accent,.06);
    surface(s,dc,key,keyFace,glass::mix(keyFace,Theme::accent,.28),9);
    lettering(dc,keys,key,s.small,Theme::accentInk,DT_CENTER);
}
inline void drawButton(State& s,const DRAWITEMSTRUCT& d){
    const int id=d.CtlID;
    const bool primary=id==IDOK,live=id==LIVE_MODE;
    const bool demo=id==DEMO_MAIN||id==DEMO_RIZIA;
    const bool disabled=(d.itemState&ODS_DISABLED)!=0,pressed=(d.itemState&ODS_SELECTED)!=0;
    const bool hover=GetPropW(d.hwndItem,L"RiziaHover")!=nullptr;
    const bool focus=(d.itemState&ODS_FOCUS)!=0;
    COLORREF bg=primary?Theme::accent:Theme::panel;
    COLORREF edge=primary?Theme::accent:Theme::line;
    COLORREF fg=primary?Theme::onAccent:Theme::text;
    if(disabled){bg=Theme::head;edge=Theme::line;fg=Theme::dim;}
    else if(pressed||hover){bg=primary?Theme::accentHover:Theme::head;edge=primary?bg:Theme::accent;}
    RECT box=d.rcItem;
    childBackdrop(s,d);
    --box.right;--box.bottom;
    const bool material=glass::enabled();
    const bool inGlassGroup=material && (id==APPEARANCE||id==BROWSE);
    const bool quiet=id==IDCANCEL||id==RESCAN||id==ADD_FOLDER||id==SUPPORT||demo||live||inGlassGroup;
    const bool plain=quiet&&!hover&&!focus&&!pressed;
    if(id==SUPPORT&&plain)fg=Theme::muted;
    RECT segment=box;
    if(inGlassGroup||live||demo)InflateRect(&segment,-s.scale(3),-s.scale(3));
    if(!plain){
        if(material && (inGlassGroup||live||demo) && !disabled){
            glass::tintSegment(d.hDC,segment,s.scale(live?20:13),Theme::accent,pressed?52:30);
        }else if(material && primary && !disabled){
            glass::surface(d.hDC,box,bg,s.scale(17),true);
        }else if(material && !disabled){
            glass::surface(d.hDC,box,(hover||pressed||focus)?Theme::select:Theme::sidebar,s.scale(14),pressed||hover);
        }else if((live||demo)&&!disabled){
            surface(s,d.hDC,segment,Theme::hover,focus?Theme::focus:Theme::line,24);
        }else surface(s,d.hDC,box,bg,focus?Theme::focus:edge,material?28:11,focus?2:1);
    }
    if(material && hover && !disabled && !demo){
        const int radius=live?(s.stacked?20:24):inGlassGroup?13:primary?17:14;
        glass::glint(d.hDC,live?box:segment,s.scale(radius),glassPointer(d.hwndItem),pressed);
    }
    const int width=box.right-box.left;
    int hmid=(box.top+box.bottom)/2;
    auto label=textOf(d.hwndItem);
    RECT textBox=box;
    HFONT font=primary||demo||live?s.emphasis:s.font;
    int labelWidth=textWidth(d.hDC,label,font);
    if(width<labelWidth+s.scale(8)){
        font=s.small;
        labelWidth=textWidth(d.hDC,label,font);
    }
    const bool hasIcon=id!=IDCANCEL&&id!=SUPPORT&&width>=labelWidth+s.scale(35);
    if(id==APPEARANCE){
        const int x=box.left+s.scale(12),r=s.scale(5);
        RECT swatch{x,hmid-r,x+2*r,hmid+r};
        surface(s,d.hDC,swatch,Theme::accent,Theme::accent,10);
        textBox.left=x+2*r+s.scale(8);textBox.right-=s.scale(24);
        if(labelWidth>textBox.right-textBox.left)label=Theme::name(Theme::index());
        lettering(d.hDC,label,textBox,font,fg,DT_LEFT);
        chevron(s,d.hDC,box.right-s.scale(13),hmid,Theme::muted);
    }else if(live){
        drawLiveEntry(s,d.hDC,box,label,fg);
    }else if(demo&&!s.stacked){
        const int tileX=box.left+s.scale(9),tileSize=s.scale(32);
        RECT tile{tileX,hmid-tileSize/2,tileX+tileSize,hmid+(tileSize+1)/2};
        const COLORREF badgeInk=campaignInk(id);
        const auto tint=glass::mix(Theme::panel,badgeInk,hover||pressed?.18:.10);
        surface(s,d.hDC,tile,tint,tint,18);
        symbol(s,d.hDC,id,(tile.left+tile.right)/2-s.scale(9),hmid-s.scale(8),badgeInk);
        textBox.left=tile.right+s.scale(10);textBox.right-=s.scale(20);
        lettering(d.hDC,label,textBox,font,fg,DT_LEFT);
        HPEN pen=CreatePen(PS_SOLID,std::max(1,s.scale(1)),Theme::muted);
        auto old=SelectObject(d.hDC,pen);
        const int end=box.right-s.scale(11);
        MoveToEx(d.hDC,end-s.scale(3),hmid-s.scale(4),nullptr);
        LineTo(d.hDC,end+s.scale(1),hmid);LineTo(d.hDC,end-s.scale(3),hmid+s.scale(4));
        SelectObject(d.hDC,old);DeleteObject(pen);
    }else if(primary && hasIcon){
        const int start=box.left+(width-labelWidth-s.scale(24))/2;
        textBox.left=start;textBox.right=start+labelWidth;
        lettering(d.hDC,label,textBox,font,fg,DT_LEFT);
        symbol(s,d.hDC,id,textBox.right+s.scale(7),hmid-s.scale(8),fg);
    }else if(hasIcon){
        const int start=box.left+(width-labelWidth-s.scale(24))/2;
        symbol(s,d.hDC,id,start,hmid-s.scale(8),primary?fg:Theme::accentInk);
        textBox.left=start+s.scale(23);textBox.right-=s.scale(5);
        lettering(d.hDC,label,textBox,font,fg,DT_LEFT);
    }else lettering(d.hDC,label,textBox,font,fg,DT_CENTER);
    if(focus&&!disabled){
        RECT ring=box;InflateRect(&ring,-s.scale(4),-s.scale(4));
        focusRing(s,d.hDC,ring,primary?Theme::onAccent:Theme::focus,21);
    }
}
inline void drawEmpty(const State& s,const DRAWITEMSTRUCT& d){
    RECT bounds=d.rcItem;
    ::fill(d.hDC,bounds,Theme::panel);
    const int midX=(bounds.left+bounds.right)/2,midY=(bounds.top+bounds.bottom)/2;
    if(bounds.bottom-bounds.top>=s.scale(100)){
        const int half=s.scale(21),cy=midY-s.scale(13);
        RECT tile{midX-half,cy-half,midX+half,cy+half};
        surface(s,d.hDC,tile,Theme::head,Theme::head,18);
        const int icon=s.busy?RESCAN:s.result.saves.empty()?BROWSE:FILTER;
        symbol(s,d.hDC,icon,midX-s.scale(8),cy-s.scale(8),Theme::accentInk);
        bounds.top=midY+s.scale(12);bounds.bottom=bounds.top+s.scale(28);
    }
    bounds.left+=s.scale(8);bounds.right-=s.scale(8);
    lettering(d.hDC,textOf(d.hwndItem),bounds,s.font,Theme::muted,DT_CENTER);
}
inline void paintCampaignFace(const State& s,HWND h,HDC dc){
    RECT box{};GetClientRect(h,&box);
    ::fill(dc,box,Theme::panel);
    --box.right;--box.bottom;
    const bool enabled=IsWindowEnabled(h)!=FALSE;
    const bool focus=GetFocus()==h;
    surface(s,dc,box,enabled?Theme::input:Theme::head,focus?Theme::focus:Theme::line,9,focus?2:1);
    RECT caption=box;caption.left+=s.scale(9);caption.right-=s.scale(23);
    const auto label=textOf(h);
    const auto font=textWidth(dc,label,s.font)>caption.right-caption.left?s.small:s.font;
    lettering(dc,label,caption,font,enabled?Theme::text:Theme::dim,DT_LEFT);
    chevron(s,dc,box.right-s.scale(13),(box.top+box.bottom)/2,Theme::muted);
}
inline LRESULT CALLBACK campaignFaceProc(HWND h,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
    auto& s=*reinterpret_cast<State*>(data);
    if(m==WM_PAINT){
        PAINTSTRUCT ps{};auto dc=BeginPaint(h,&ps);paintCampaignFace(s,h,dc);EndPaint(h,&ps);return 0;
    }
    if(m==WM_PRINTCLIENT){paintCampaignFace(s,h,(HDC)wp);return 0;}
    if(m==WM_ERASEBKGND)return 1;
    const auto result=DefSubclassProc(h,m,wp,lp);
    if(m==CB_SETCURSEL||m==WM_SETFOCUS||m==WM_KILLFOCUS||m==WM_ENABLE)InvalidateRect(h,nullptr,FALSE);
    return result;
}
inline void drawCampaignItem(const State& s,const DRAWITEMSTRUCT& d){
    if(d.itemState&ODS_COMBOBOXEDIT){paintCampaignFace(s,d.hwndItem,d.hDC);return;}
    const bool selected=(d.itemState&ODS_SELECTED)!=0;
    ::fill(d.hDC,d.rcItem,selected?Theme::select:Theme::input);
    const auto length=SendMessageW(d.hwndItem,CB_GETLBTEXTLEN,d.itemID,0);
    if(length==CB_ERR)return;
    std::wstring label((size_t)length+1,L'\0');
    SendMessageW(d.hwndItem,CB_GETLBTEXT,d.itemID,(LPARAM)label.data());label.resize((size_t)length);
    RECT caption=d.rcItem;caption.left+=s.scale(9);caption.right-=s.scale(5);
    lettering(d.hDC,label,caption,s.font,selected?Theme::accentInk:Theme::text,DT_LEFT);
    if(d.itemState&ODS_FOCUS)DrawFocusRect(d.hDC,&d.rcItem);
}
inline LRESULT CALLBACK header(HWND h,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
    auto&s=*reinterpret_cast<State*>(data);
    if(m==WM_ERASEBKGND)return 1;
    if(m==WM_PAINT){
        PAINTSTRUCT ps{};auto dc=BeginPaint(h,&ps);
        RECT all{};GetClientRect(h,&all);::fill(dc,all,Theme::panel);
        const auto headerFace=glass::mix(Theme::panel,Theme::head,.77);
        surface(s,dc,all,headerFace,headerFace,12);
        for(int i=0;i<Header_GetItemCount(h);++i){
            RECT cell{};Header_GetItemRect(h,i,&cell);
            if(cell.right<=cell.left)continue;
            wchar_t name[96]{};HDITEMW item{};item.mask=HDI_TEXT;item.pszText=name;item.cchTextMax=96;Header_GetItem(h,i,&item);
            cell.left+=s.scale(9)+(i==0?fileIconInset(s):0);cell.right-=s.scale(3);
            lettering(dc,name,cell,s.small,Theme::muted,DT_LEFT);
        }
        EndPaint(h,&ps);return 0;
    }
    return DefSubclassProc(h,m,wp,lp);
}
inline LRESULT drawRow(State& s,NMLVCUSTOMDRAW& d){
    if(d.nmcd.dwDrawStage==CDDS_PREPAINT)return CDRF_NOTIFYITEMDRAW|CDRF_NOTIFYPOSTPAINT;
    if(d.nmcd.dwDrawStage==CDDS_POSTPAINT&&!s.rows.empty()){
        RECT last{},space{};
        if(ListView_GetItemRect(s.list,static_cast<int>(s.rows.size())-1,&last,LVIR_BOUNDS)){
            GetClientRect(s.list,&space);
            space.top=last.bottom;
            if(space.top<space.bottom)::fill(d.nmcd.hdc,space,Theme::panel);
        }
        return CDRF_DODEFAULT;
    }
    if(d.nmcd.dwDrawStage!=CDDS_ITEMPREPAINT)return CDRF_DODEFAULT;
    int row=(int)d.nmcd.dwItemSpec;
    if(row<0||row>=(int)s.rows.size()||s.busy)return CDRF_DODEFAULT;
    RECT rowBox{}, viewport{};ListView_GetItemRect(s.list,row,&rowBox,LVIR_BOUNDS);
    GetClientRect(s.list,&viewport);rowBox.right=viewport.right;
    const bool selected=(ListView_GetItemState(s.list,row,LVIS_SELECTED)&LVIS_SELECTED)!=0;
    const int iconInset=fileIconInset(s);
    ::fill(d.nmcd.hdc,rowBox,Theme::panel);
    if(selected){
        RECT shape=rowBox;InflateRect(&shape,-s.scale(1),-s.scale(2));
        surface(s,d.nmcd.hdc,shape,Theme::select,glass::mix(Theme::select,Theme::accent,.22),16);
        if(!iconInset){
            const int mid=(shape.top+shape.bottom)/2;
            RECT bar{shape.left+s.scale(2),mid-s.scale(8),shape.left+s.scale(4),mid+s.scale(8)};
            surface(s,d.nmcd.hdc,bar,Theme::accent,Theme::accent,2);
        }
    }else if(row+1<(int)s.rows.size() && !(ListView_GetItemState(s.list,row+1,LVIS_SELECTED)&LVIS_SELECTED)){
        RECT divider{rowBox.left+s.scale(9)+iconInset,rowBox.bottom-1,rowBox.right-s.scale(3),rowBox.bottom};
        ::fill(d.nmcd.hdc,divider,glass::mix(Theme::panel,Theme::line,.63));
    }
    for(int column=0;column<5;++column){
        RECT cell{};Header_GetItemRect(ListView_GetHeader(s.list),column,&cell);
        if(cell.right<=cell.left)continue;
        MapWindowPoints(ListView_GetHeader(s.list),s.list,reinterpret_cast<POINT*>(&cell),2);
        cell.top=rowBox.top;cell.bottom=rowBox.bottom;cell.left+=s.scale(9);cell.right-=s.scale(3);
        wchar_t value[256]{};
        ListView_GetItemText(s.list,row,column,value,256);
        std::wstring caption=value;
        if(column==0){
            const int midY=(cell.top+cell.bottom)/2;
            if(iconInset){
                const auto& story=s.result.saves[s.rows[row]].story;
                const int icon=story=="StoryPack_Main"?DEMO_MAIN:story=="StoryPack_Rizia"?DEMO_RIZIA:LIST;
                const auto ink=campaignInk(icon);
                const auto tint=selected?Theme::accent:glass::mix(Theme::panel,ink,.11);
                RECT badge{cell.left,midY-s.scale(14),cell.left+s.scale(28),midY+s.scale(14)};
                surface(s,d.nmcd.hdc,badge,tint,tint,16);
                symbol(s,d.nmcd.hdc,icon,cell.left+s.scale(5),midY-s.scale(9),selected?Theme::onAccent:ink);
                cell.left+=iconInset;
            }
            wchar_t secondary[512]{};
            ListView_GetItemText(s.list,row,2,secondary,512);
            if(secondary[0]){
                RECT detail=cell;detail.top=midY;
                lettering(d.nmcd.hdc,secondary,detail,s.small,Theme::muted,DT_LEFT);
                cell.bottom=midY+s.scale(3);
            }
        }
        if(column==4&&caption.size()==20&&caption.compare(10,2,L"  ")==0){
            const int midY=(cell.top+cell.bottom)/2;
            RECT clock=cell;clock.top=midY;
            lettering(d.nmcd.hdc,caption.substr(12),clock,s.small,Theme::muted,DT_LEFT);
            caption.resize(10);
            cell.bottom=midY+s.scale(3);
        }
        const auto ink=column==0 ? (selected?Theme::accentInk:Theme::text) : Theme::muted;
        HFONT face=column>=3?s.number:column==0&&selected?s.emphasis:s.font;
        lettering(d.nmcd.hdc,caption,cell,face,ink,DT_LEFT);
    }
    if(selected&&GetFocus()==s.list){RECT ring=rowBox;InflateRect(&ring,-s.scale(2),-s.scale(2));focusRing(s,d.nmcd.hdc,ring,Theme::focus,14);}
    return CDRF_SKIPDEFAULT;
}
