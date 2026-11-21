#pragma once

inline void clearSearchField(HWND field) {
    SetFocus(field);
    SetWindowTextW(field,L"");
}

// Both search fields expose a real, named button to Tab and accessibility tools.
inline void drawClearSearch(const DRAWITEMSTRUCT& item,UINT dpi) {
    const auto scale=[&](int n){return MulDiv(n,dpi,96);};
    const bool focused=(item.itemState&ODS_FOCUS) && !(item.itemState&ODS_NOFOCUSRECT);
    const bool active=(item.itemState&ODS_SELECTED) || GetPropW(item.hwndItem,L"RiziaHover");
    auto dc=item.hDC;
    const RECT box=item.rcItem;
    fill(dc,box,Theme::input);
    auto brush=CreateSolidBrush(active?Theme::select:Theme::input);
    auto ring=CreatePen(PS_SOLID,scale(1),focused?Theme::focus:active?Theme::select:Theme::input);
    auto oldBrush=SelectObject(dc,brush),oldPen=SelectObject(dc,ring);
    Ellipse(dc,box.left+scale(2),box.top+scale(2),box.right-scale(2),box.bottom-scale(2));
    auto stroke=CreatePen(PS_SOLID,std::max(1,scale(2)),active?Theme::accentInk:Theme::muted);
    SelectObject(dc,stroke);
    const int x=(box.left+box.right)/2,y=(box.top+box.bottom)/2,r=scale(4);
    MoveToEx(dc,x-r,y-r,nullptr);LineTo(dc,x+r+1,y+r+1);
    MoveToEx(dc,x-r,y+r,nullptr);LineTo(dc,x+r+1,y-r-1);
    SelectObject(dc,oldPen);SelectObject(dc,oldBrush);
    DeleteObject(stroke);DeleteObject(ring);DeleteObject(brush);
}

inline LRESULT CALLBACK searchFieldProc(HWND h,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto results=reinterpret_cast<HWND>(data);
    if(m==WM_IME_STARTCOMPOSITION)SetPropW(h,L"SearchComposition",reinterpret_cast<HANDLE>(1));
    if(m==WM_IME_ENDCOMPOSITION)RemovePropW(h,L"SearchComposition");
    const bool composing=GetPropW(h,L"SearchComposition")!=nullptr;
    const bool escape=wp==VK_ESCAPE && (composing || GetWindowTextLengthW(h)>0 || h==g.search);
    if(m==WM_GETDLGCODE && lp && (escape || wp==VK_DOWN))
        return DefSubclassProc(h,m,wp,lp)|DLGC_WANTMESSAGE;
    if(m==WM_KEYDOWN && !composing) {
        if(escape) {
            if(GetWindowTextLengthW(h)>0)clearSearchField(h);
            else SetFocus(results);
            return 0;
        }
        if(wp==VK_DOWN) {
            if(ListView_GetItemCount(results)>0)SetFocus(results);
            return 0;
        }
    }
    if(m==WM_CHAR && wp==VK_ESCAPE && !composing)return 0;
    if((m==WM_PAINT || m==WM_PRINTCLIENT) && !composing && GetWindowTextLengthW(h)==0) {
        PAINTSTRUCT ps{};
        auto dc=m==WM_PAINT?BeginPaint(h,&ps):reinterpret_cast<HDC>(wp);
        RECT bounds{},line{};
        GetClientRect(h,&bounds);SendMessageW(h,EM_GETRECT,0,(LPARAM)&line);
        fill(dc,bounds,Theme::input);
        wchar_t cue[192]{};
        SendMessageW(h,EM_GETCUEBANNER,(WPARAM)cue,192);
        auto font=reinterpret_cast<HFONT>(SendMessageW(h,WM_GETFONT,0,0));
        literalText(dc,cue,line,font,Theme::muted);
        if(m==WM_PAINT)EndPaint(h,&ps);
        return 0;
    }
    const auto result=DefSubclassProc(h,m,wp,lp);
    if(m==WM_SETTEXT || m==EM_SETCUEBANNER || m==WM_KILLFOCUS || m==WM_SETFOCUS || m==WM_IME_ENDCOMPOSITION)
        InvalidateRect(h,nullptr,FALSE);
    return result;
}

inline void configureSearchField(HWND field,HWND results,const wchar_t* hint) {
    SendMessageW(field,EM_SETLIMITTEXT,160,0);
    SendMessageW(field,EM_SETCUEBANNER,TRUE,(LPARAM)tr(hint).c_str());
    SetWindowSubclass(field,searchFieldProc,1,reinterpret_cast<DWORD_PTR>(results));
}
