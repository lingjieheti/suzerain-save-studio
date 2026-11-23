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
inline void symbol(const State& s,HDC dc,int id,int x,int y,COLORREF color){
    HPEN pen=CreatePen(PS_SOLID,std::max(1,s.scale(1)),color);
    auto old=SelectObject(dc,pen),brush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
    auto line=[&](std::initializer_list<POINT> points){
        bool start=true;
        for(auto p:points){if(start)MoveToEx(dc,x+s.scale(p.x),y+s.scale(p.y),nullptr);else LineTo(dc,x+s.scale(p.x),y+s.scale(p.y));start=false;}
    };
    auto circle=[&](int l,int t,int r,int b){Ellipse(dc,x+s.scale(l),y+s.scale(t),x+s.scale(r),y+s.scale(b));};
    switch(id){
        case FILTER:circle(2,2,12,12);line({{10,10},{16,16}});break;
        case BROWSE:case ADD_FOLDER:
            line({{1,14},{1,4},{6,4},{8,6},{16,6},{16,9}});
            line({{1,14},{4,8},{18,8},{15,15},{1,15}});break;
        case RESCAN:
            Arc(dc,x+s.scale(2),y+s.scale(2),x+s.scale(16),y+s.scale(16),x+s.scale(14),y+s.scale(6),x+s.scale(15),y+s.scale(9));
            line({{11,6},{16,6},{16,1}});break;
        case DEMO_MAIN:
            line({{1,6},{9,2},{17,6},{1,6}});line({{4,8},{4,13}});line({{9,8},{9,13}});line({{14,8},{14,13}});line({{2,15},{16,15}});break;
        case DEMO_RIZIA:
            line({{1,5},{5,8},{9,3},{13,8},{17,5},{15,13},{3,13},{1,5}});line({{4,16},{14,16}});break;
        case LIVE_MODE:
            line({{0,10},{4,10},{7,3},{10,15},{13,8},{17,8}});break;
        case IDOK:line({{2,9},{15,9}});line({{10,4},{15,9},{10,14}});break;
        case LIST:
            line({{3,1},{10,1},{15,6},{15,17},{3,17},{3,1}});
            line({{10,1},{10,6},{15,6}});line({{6,11},{12,11}});break;
        default:
            line({{2,4},{8,4},{10,6},{16,6},{16,15},{2,15},{2,4}});
            line({{4,2},{10,2},{12,4},{17,4}});break;
    }
    SelectObject(dc,brush);SelectObject(dc,old);DeleteObject(pen);
}
// Child windows sample this same canvas so their corners reveal the material beneath them.
inline void drawBackdrop(const State& s,HDC dc,const RECT& client){
    const bool material=glass::enabled();
    if(material)glass::wash(dc,client,Theme::bg);
    else ::fill(dc,client,Theme::bg);
    surface(s,dc,s.library,Theme::panel,Theme::line,14);
    if(material){
        glass::surface(dc,s.headerTools,Theme::sidebar,s.scale(20));
        glass::surface(dc,s.practice,Theme::sidebar,s.scale(18));
        if(!s.stacked){
            RECT live{};GetWindowRect(s.live,&live);
            MapWindowPoints(nullptr,s.win,reinterpret_cast<POINT*>(&live),2);
            glass::surface(dc,live,Theme::select,s.scale(17));
        }
    }else if(s.stacked)surface(s,dc,s.practice,Theme::panel,Theme::line,14);
    else {
        RECT rule{s.practice.left+s.scale(11),s.practice.bottom-1,s.practice.right-s.scale(11),s.practice.bottom};
        ::fill(dc,rule,Theme::line);
    }
    if(!IsRectEmpty(&s.headingIcon)){
        if(material)glass::surface(dc,s.headingIcon,Theme::select,s.scale(13));
        else surface(s,dc,s.headingIcon,Theme::select,Theme::select,12);
        symbol(s,dc,0,s.headingIcon.left+s.scale(8),s.headingIcon.top+s.scale(9),Theme::accentInk);
    }
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
    surface(s,dc,s.searchFrame,Theme::input,GetFocus()==s.filter?Theme::focus:Theme::line,glass::enabled()?24:10);
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
    const bool railItem=(demo||live)&&!s.stacked;
    const bool material=glass::enabled();
    const bool inToolbar=material && (id==APPEARANCE||id==BROWSE);
    bool plain=(id==IDCANCEL||id==RESCAN||id==ADD_FOLDER||id==SUPPORT||railItem||inToolbar)&&!hover&&!focus&&!pressed;
    if(id==SUPPORT&&plain)fg=Theme::muted;
    if(!plain){
        if(material && !primary && !disabled){
            glass::surface(d.hDC,box,(hover||pressed||focus)?Theme::select:Theme::sidebar,
                           s.scale(live&&s.stacked?16:14),pressed||hover);
        }else surface(s,d.hDC,box,bg,focus?Theme::focus:edge,material?28:11,focus?2:1);
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
    }else if(live&&s.stacked){
        const int center=(box.left+box.right)/2;
        symbol(s,d.hDC,id,center-s.scale(9),hmid-s.scale(28),Theme::accentInk);
        RECT title=box,hint=box;
        title.top=hmid-s.scale(9);title.bottom=hmid+s.scale(13);
        hint.top=title.bottom;hint.bottom=box.bottom-s.scale(2);
        lettering(d.hDC,label,title,font,fg,DT_CENTER);
        lettering(d.hDC,live_setup::inGameShortcut(),hint,s.small,Theme::muted,DT_CENTER);
    }else if(railItem){
        const int tileX=box.left+s.scale(8), tileSize=s.scale(31);
        RECT tile{tileX,hmid-tileSize/2,tileX+tileSize,hmid+(tileSize+1)/2};
        const auto tint=live?Theme::select:Theme::head;
        surface(s,d.hDC,tile,tint,tint,12);
        symbol(s,d.hDC,id,tile.left+s.scale(7),hmid-s.scale(8),live?Theme::accentInk:Theme::muted);
        textBox.left=tile.right+s.scale(10);textBox.right-=s.scale(20);
        if(live){
            RECT top=textBox,bottom=textBox;
            top.bottom=hmid+s.scale(2);bottom.top=hmid+s.scale(1);
            lettering(d.hDC,label,top,font,fg,DT_LEFT);
            lettering(d.hDC,live_setup::inGameShortcut(),bottom,s.small,Theme::muted,DT_LEFT);
        }else lettering(d.hDC,label,textBox,font,fg,DT_LEFT);
        HPEN pen=CreatePen(PS_SOLID,std::max(1,s.scale(1)),Theme::muted);
        auto old=SelectObject(d.hDC,pen);
        const int end=box.right-s.scale(11);
        MoveToEx(d.hDC,end-s.scale(3),hmid-s.scale(4),nullptr);
        LineTo(d.hDC,end+s.scale(1),hmid);LineTo(d.hDC,end-s.scale(3),hmid+s.scale(4));
        SelectObject(d.hDC,old);DeleteObject(pen);
    }else if(hasIcon){
        const int start=box.left+(width-labelWidth-s.scale(24))/2;
        symbol(s,d.hDC,id,start,hmid-s.scale(8),primary?fg:Theme::accentInk);
        textBox.left=start+s.scale(23);textBox.right-=s.scale(5);
        lettering(d.hDC,label,textBox,font,fg,DT_LEFT);
    }else lettering(d.hDC,label,textBox,font,fg,DT_CENTER);
    if(focus&&!disabled){
        RECT ring=box;InflateRect(&ring,-s.scale(4),-s.scale(4));
        DrawFocusRect(d.hDC,&ring);
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
        symbol(s,d.hDC,FILTER,midX-s.scale(8),cy-s.scale(8),Theme::muted);
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
        RECT all{};GetClientRect(h,&all);::fill(dc,all,Theme::head);
        for(int i=0;i<Header_GetItemCount(h);++i){
            RECT cell{};Header_GetItemRect(h,i,&cell);
            if(cell.right<=cell.left)continue;
            wchar_t name[96]{};HDITEMW item{};item.mask=HDI_TEXT;item.pszText=name;item.cchTextMax=96;Header_GetItem(h,i,&item);
            cell.left+=s.scale(9);cell.right-=s.scale(3);
            lettering(dc,name,cell,s.small,Theme::muted,DT_LEFT);
        }
        RECT rule{0,all.bottom-1,all.right,all.bottom};::fill(dc,rule,Theme::line);
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
    RECT rowBox{};ListView_GetItemRect(s.list,row,&rowBox,LVIR_BOUNDS);
    const bool selected=(ListView_GetItemState(s.list,row,LVIS_SELECTED)&LVIS_SELECTED)!=0;
    ::fill(d.nmcd.hdc,rowBox,Theme::panel);
    if(selected){
        RECT shape=rowBox;InflateRect(&shape,-s.scale(1),-s.scale(2));
        surface(s,d.nmcd.hdc,shape,Theme::select,Theme::select,13);
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
            if(cell.right-cell.left>=s.scale(174)){
                symbol(s,d.nmcd.hdc,LIST,cell.left,midY-s.scale(9),selected?Theme::accentInk:Theme::muted);
                cell.left+=s.scale(26);
            }
            wchar_t secondary[512]{};
            ListView_GetItemText(s.list,row,2,secondary,512);
            if(secondary[0]){
                RECT detail=cell;detail.top=midY;
                lettering(d.nmcd.hdc,secondary,detail,s.small,Theme::muted,DT_LEFT);
                cell.bottom=midY+s.scale(3);
            }
        }
        if(column==4&&caption.size()==20&&caption[10]==L' '){
            auto old=SelectObject(d.nmcd.hdc,s.number);SIZE size{};
            GetTextExtentPoint32W(d.nmcd.hdc,caption.c_str(),(int)caption.size(),&size);
            if(size.cx>cell.right-cell.left){
                caption=caption.substr(0,10)+L" "+caption.substr(12,5);
                GetTextExtentPoint32W(d.nmcd.hdc,caption.c_str(),(int)caption.size(),&size);
                if(size.cx>cell.right-cell.left)caption.resize(10);
            }
            SelectObject(d.nmcd.hdc,old);
        }
        const auto ink=column==0 ? (selected?Theme::accentInk:Theme::text) : Theme::muted;
        HFONT face=column>=3?s.number:column==0?s.emphasis:s.font;
        lettering(d.nmcd.hdc,caption,cell,face,ink,DT_LEFT);
    }
    if(selected&&GetFocus()==s.list){RECT ring=rowBox;InflateRect(&ring,-s.scale(2),-s.scale(2));DrawFocusRect(d.nmcd.hdc,&ring);}
    return CDRF_SKIPDEFAULT;
}
