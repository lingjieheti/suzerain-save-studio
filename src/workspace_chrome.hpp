#pragma once

// All layout measurements are device-independent pixels.
struct WorkspaceGeometry {
    int width, height, rail, gap, left, right, inspector, content, list;
    int headerBottom, metricHeight, presetsY, toolsY, listY, bottom, footer;
    bool compactRail, shortScreen;
};

WorkspaceGeometry workspaceGeometry() {
    RECT bounds{};
    GetClientRect(g.win, &bounds);
    WorkspaceGeometry a{};
    a.width = MulDiv(bounds.right, 96, g.dpi);
    a.height = MulDiv(bounds.bottom, 96, g.dpi);
    a.shortScreen = a.height < 610;
    a.compactRail = a.width < 1000 || a.height < 435;
    a.rail = a.compactRail ? 56 : (a.width < 1140 ? 174 : 194);
    const bool spacious = a.width >= 1140 && a.height >= 710;
    a.gap = a.width < 800 ? 9 : spacious ? 24 : 18;
    int margin = a.width < 900 ? 12 : spacious ? 32 : 26;
    a.left = a.rail + margin;
    a.content = a.width - a.left - margin;
    a.inspector = std::clamp(a.content / 3, 180, 286);
    a.right = a.width - margin - a.inspector;
    a.list = a.right - a.left - a.gap;
    a.headerBottom = a.shortScreen ? 78 : 102;
    if (g.tab == 0 && a.height >= 540)
        a.metricHeight = a.height < 710 ? 54 : 76;
    a.presetsY = a.headerBottom + (a.metricHeight ? a.metricHeight + 9 : 0);
    a.toolsY = g.tab == 0 ? a.presetsY + 39 : a.headerBottom;
    a.listY = a.toolsY + (spacious ? 46 : 40);
    a.footer = a.height - (a.shortScreen ? 46 : 54);
    a.bottom = a.footer - 10;
    return a;
}

void fill(HDC dc, RECT bounds, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &bounds, brush);
    DeleteObject(brush);
}

// Monochrome navigation symbols also identify pages when the sidebar collapses.
void navSymbol(HDC dc, int id, int x, int y, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, std::max(1, px(2)), color);
    auto oldPen = SelectObject(dc, pen);
    auto oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    auto stroke = [&](std::initializer_list<POINT> points) {
        bool first = true;
        for (auto p : points) {
            if (first) MoveToEx(dc, x + px(p.x), y + px(p.y), nullptr);
            else LineTo(dc, x + px(p.x), y + px(p.y));
            first = false;
        }
    };
    auto oval = [&](int l,int t,int r,int b) { Ellipse(dc,x+px(l),y+px(t),x+px(r),y+px(b)); };
    switch (id) {
        case 0:
            stroke({{1,16},{18,16}}); stroke({{4,13},{4,9}}); stroke({{9,13},{9,6}}); stroke({{14,13},{14,2}}); break;
        case 1:
            stroke({{1,6},{9,2},{17,6},{1,6}}); stroke({{4,8},{4,14}}); stroke({{9,8},{9,14}}); stroke({{14,8},{14,14}}); stroke({{1,16},{17,16}}); break;
        case 2:
            oval(4,1,11,8); stroke({{1,16},{2,12},{5,10},{9,10},{12,12},{13,16}}); stroke({{13,3},{15,4},{15,7},{13,8}}); stroke({{14,10},{17,12},{18,16}}); break;
        case 3:
            oval(1,1,18,18); oval(6,1,13,18); stroke({{2,9},{17,9}}); break;
        case 4:
            stroke({{1,4},{17,4}}); stroke({{1,9},{17,9}}); stroke({{1,14},{17,14}}); stroke({{5,1},{5,7}}); stroke({{13,6},{13,12}}); stroke({{7,11},{7,17}}); break;
        case 5:
            stroke({{9,16},{3,16},{3,2},{14,2},{14,8}}); stroke({{5,6},{11,6}}); stroke({{8,12},{11,15},{17,9}}); break;
        case 6:
            stroke({{1,2},{17,2},{17,6},{1,6},{1,2}}); stroke({{3,7},{3,16},{15,16},{15,7}}); stroke({{7,10},{12,10}}); break;
        default:
            stroke({{1,3},{5,3},{9,5},{13,3},{17,3},{17,15},{13,15},{9,17},{5,15},{1,15},{1,3}}); stroke({{9,5},{9,17}}); break;
    }
    SelectObject(dc, oldPen); SelectObject(dc, oldBrush); DeleteObject(pen);
}

void drawButton(const DRAWITEMSTRUCT* d) {
    int id = d->CtlID;
    bool disabled = d->itemState & ODS_DISABLED;
    bool hover = GetPropW(d->hwndItem, L"RiziaHover") != nullptr;
    bool down = d->itemState & ODS_SELECTED;
    bool nav = id >= NAV_BASE && id < NAV_BASE + 8;
    bool activeNav = nav && id - NAV_BASE == g.tab;
    bool check = id == AUTOCAP || id == FILTER_CHANGED;
    bool checked = id == AUTOCAP ? g.autoCap : g.changedOnly;
    bool toggle = (id == BOOL_FALSE && !g.boolValue) || (id == BOOL_TRUE && g.boolValue);
    bool primary = (id == LATEST && !g.doc) || id == APPLY || (id == CONFIRM_SAVE && !g.demoResource) ||
                   (id == SAVEAS && g.demoResource) || id == BACKUP_RESTORE ||
                   (id == SAVE && (!pending().empty() || g.editingDirty));
    bool quiet = id==FOLDER || id==RELOAD || id==HISTORY_UNDO || id==HISTORY_REDO;
    bool inspector = id==APPLY || id==UNDOONE || id==BOOL_FALSE || id==BOOL_TRUE ||
                     id==SAVEAS || id==CONFIRM_SAVE || id==PATCH_EXPORT || id==REVIEW_REVERT ||
                     id==UNDOALL || id==BACKUP_RESTORE || id==BACKUP_CANCEL || id==COPY_KEY || id==EXPORT_CSV;
    COLORREF surface = nav ? Theme::sidebar : inspector ? Theme::panel : Theme::bg;
    COLORREF bg = quiet ? surface : Theme::input, border = quiet ? surface : Theme::line, fg = Theme::text;
    if (nav) {
        bg = activeNav ? Theme::select : hover ? Theme::sideHover : Theme::sidebar;
        border = bg; fg = activeNav ? Theme::accentInk : Theme::sideText;
    } else if (primary) {
        bg = border = hover || down ? Theme::accentHover : Theme::accent; fg = Theme::onAccent;
    } else if (toggle || hover || down) {
        bg = toggle ? Theme::select : Theme::hover; border = toggle ? Theme::accent : Theme::line;
    } else if (check) {
        bg = border = Theme::bg;
    }
    if (disabled) { bg = quiet ? surface : Theme::stripe; fg = Theme::dim; border = quiet ? surface : Theme::line; }
    RECT r = d->rcItem;
    fill(d->hDC, r, surface);
    rounded(d->hDC, r, bg, border, nav ? 14 : 11);
    RECT tx = r;
    if (nav) {
        bool compact = r.right - r.left < px(64);
        int glyphX = compact ? (r.right - px(18)) / 2 : px(13);
        navSymbol(d->hDC, id - NAV_BASE, glyphX, (r.bottom - px(18)) / 2,
                  activeNav ? Theme::accentInk : Theme::sideMuted);
        if (!compact) {
            tx.left += px(42); tx.right -= px(5);
            drawText(d->hDC, textOf(d->hwndItem), tx, g.font, fg);
        }
    } else if(id==APPEARANCE) {
        const bool compact=workspaceGeometry().compactRail;
        const int cy=(r.top+r.bottom)/2;
        RECT mark{px(9),cy-px(5),px(19),cy+px(5)};
        rounded(d->hDC,mark,Theme::accent,Theme::accent,5);
        tx.left=px(26);tx.right-=px(17);
        auto caption=compact?std::wstring(Theme::name(Theme::index())):textOf(d->hwndItem);
        drawText(d->hDC,caption,tx,g.small,fg);
        int x=r.right-px(11);
        auto pen=CreatePen(PS_SOLID,std::max(1,px(1)),Theme::muted);auto old=SelectObject(d->hDC,pen);
        MoveToEx(d->hDC,x-px(3),cy-px(2),nullptr);LineTo(d->hDC,x,cy+px(1));LineTo(d->hDC,x+px(3),cy-px(2));
        SelectObject(d->hDC,old);DeleteObject(pen);
    } else {
        if (check) {
            int y = (r.bottom - px(15)) / 2;
            RECT box{px(5), y, px(20), y + px(15)};
            rounded(d->hDC, box, checked ? Theme::accent : Theme::input, checked ? Theme::accent : Theme::dim, 5);
            if (checked) {
                auto pen = CreatePen(PS_SOLID, px(2), Theme::onAccent);
                auto old = SelectObject(d->hDC, pen);
                MoveToEx(d->hDC, px(8), y+px(8), nullptr); LineTo(d->hDC, px(11), y+px(11));
                LineTo(d->hDC, px(17), y+px(4)); SelectObject(d->hDC, old); DeleteObject(pen);
            }
            tx.left += px(25);
        }
        drawText(d->hDC, textOf(d->hwndItem), tx, g.font, fg,
                 (check ? DT_LEFT : DT_CENTER) | DT_VCENTER | DT_SINGLELINE);
    }
    if ((d->itemState & ODS_FOCUS) && !(d->itemState & ODS_NOFOCUSRECT) && !disabled) {
        RECT focus = r; InflateRect(&focus, -px(4), -px(4));
        auto pen=CreatePen(PS_DOT,1,primary?Theme::onAccent:Theme::focus);
        auto oldPen=SelectObject(d->hDC,pen);auto oldBrush=SelectObject(d->hDC,GetStockObject(HOLLOW_BRUSH));
        RoundRect(d->hDC,focus.left,focus.top,focus.right,focus.bottom,px(5),px(5));
        SelectObject(d->hDC,oldBrush);SelectObject(d->hDC,oldPen);DeleteObject(pen);
    }
}

void literalText(HDC dc,const wchar_t* value,RECT r,HFONT font,COLORREF color,UINT align=DT_LEFT) {
    auto old=SelectObject(dc,font);
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,color);
    DrawTextW(dc,value,-1,&r,align|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    SelectObject(dc,old);
}

UINT gridAlignment(int col) {
    if(col==0 || (g.tab==6 && col==2)) return DT_LEFT;
    if(g.tab<5 && col==3) return DT_CENTER;
    return DT_RIGHT;
}

// Paint the visible cells from the native ListView. Selection, keyboard search,
// accessibility and scrolling still use the common control's own row model.
void drawWorkspaceRow(NMLVCUSTOMDRAW& item) {
    auto dc=item.nmcd.hdc;
    int row=static_cast<int>(item.nmcd.dwItemSpec);
    RECT r{}; if(!ListView_GetItemRect(g.grid,row,&r,LVIR_BOUNDS)) return;
    RECT view{};GetClientRect(g.grid,&view);r.left=0;r.right=view.right;
    auto state=ListView_GetItemState(g.grid,row,LVIS_SELECTED|LVIS_FOCUSED);
    bool selected=(state&LVIS_SELECTED)!=0;
    bool changed=g.tab==5 || (g.tab<5 && row>=0 && row<(int)g.rows.size() && pending().count(g.rows[row]));
    fill(dc,r,selected?Theme::select:Theme::panel);
    if(selected || changed) {
        RECT mark{r.left,r.top+px(5),r.left+px(selected?3:2),r.bottom-px(5)};
        fill(dc,mark,selected?Theme::accent:Theme::focus);
    }
    auto header=ListView_GetHeader(g.grid);
    for(int col=0;col<Header_GetItemCount(header);++col) {
        RECT cell{};Header_GetItemRect(header,col,&cell);
        if(cell.right<=cell.left)continue;
        MapWindowPoints(header,g.grid,reinterpret_cast<POINT*>(&cell),2);
        cell.top=r.top;cell.bottom=r.bottom;
        cell.left+=px(col==0?12:5);cell.right-=px(col==3?4:11);
        wchar_t value[2048]{};ListView_GetItemText(g.grid,row,col,value,2048);
        COLORREF ink=col==0?Theme::text:Theme::muted;
        if((col==2 && changed) || (col==0 && selected))ink=Theme::accentInk;
        literalText(dc,value,cell,col==3?g.small:g.font,ink,gridAlignment(col));
    }
    RECT rule{0,r.bottom-1,r.right,r.bottom};fill(dc,rule,Theme::stripe);
    if((state&LVIS_FOCUSED) && GetFocus()==g.grid) {
        auto pen=CreatePen(PS_DOT,1,Theme::focus);auto old=SelectObject(dc,pen);
        auto brush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc,r.left+1,r.top+1,r.right-1,r.bottom-1);
        SelectObject(dc,brush);SelectObject(dc,old);DeleteObject(pen);
    }
}

void fillGridRemainder(HDC dc) {
    RECT area{};GetClientRect(g.grid,&area);
    RECT last{};
    int count=ListView_GetItemCount(g.grid);
    if(count && ListView_GetItemRect(g.grid,count-1,&last,LVIR_BOUNDS)) area.top=std::max<LONG>(0,last.bottom);
    else {
        auto header=ListView_GetHeader(g.grid);GetWindowRect(header,&last);
        MapWindowPoints(nullptr,g.grid,reinterpret_cast<POINT*>(&last),2);area.top=last.bottom;
    }
    if(area.bottom>area.top)fill(dc,area,Theme::panel);
}

void paintWorkspaceHeader(HWND h,HDC dc) {
    RECT all{};GetClientRect(h,&all);fill(dc,all,Theme::head);
    for(int col=0;col<Header_GetItemCount(h);++col) {
        RECT cell{};Header_GetItemRect(h,col,&cell);
        if(cell.right<=cell.left)continue;
        wchar_t title[128]{};
        HDITEMW item{};item.mask=HDI_TEXT;item.pszText=title;item.cchTextMax=128;
        Header_GetItem(h,col,&item);
        cell.left+=px(col==0?12:5);cell.right-=px(col==3?4:11);
        literalText(dc,title,cell,g.small,Theme::muted,gridAlignment(col));
        if(col==0 && g.doc && cell.right-cell.left>px(120)) {
            auto old=SelectObject(dc,g.small);
            SIZE extent{};GetTextExtentPoint32W(dc,title,lstrlenW(title),&extent);SelectObject(dc,old);
            auto count=std::to_wstring(g.tab==6?g.backups.size():g.rows.size());
            cell.left+=extent.cx+px(8);
            if(cell.right-cell.left>=px((int)count.size()*8+12))
                literalText(dc,count.c_str(),cell,g.small,Theme::accentInk);
        }
    }
    all.top=all.bottom-1;fill(dc,all,Theme::line);
}

void paintLanguageFace(HWND h,HDC dc) {
    RECT r{};GetClientRect(h,&r);
    auto a=workspaceGeometry();
    fill(dc,r,a.compactRail?Theme::bg:Theme::sidebar);
    rounded(dc,r,Theme::input,GetFocus()==h?Theme::focus:Theme::line,9);
    RECT tx=r;tx.left+=px(9);tx.right-=px(22);
    literalText(dc,textOf(h).c_str(),tx,g.font,Theme::text);
    int x=r.right-px(14),y=r.bottom/2;
    auto pen=CreatePen(PS_SOLID,std::max(1,px(1)),Theme::muted);auto old=SelectObject(dc,pen);
    MoveToEx(dc,x-px(4),y-px(2),nullptr);LineTo(dc,x,y+px(2));LineTo(dc,x+px(4),y-px(2));
    SelectObject(dc,old);DeleteObject(pen);
}

LRESULT CALLBACK languageFaceProc(HWND h,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR) {
    if(m==WM_PAINT){PAINTSTRUCT ps{};auto dc=BeginPaint(h,&ps);paintLanguageFace(h,dc);EndPaint(h,&ps);return 0;}
    if(m==WM_PRINTCLIENT){paintLanguageFace(h,(HDC)wp);return 0;}
    auto result=DefSubclassProc(h,m,wp,lp);
    if(m==CB_SETCURSEL||m==WM_SETFOCUS||m==WM_KILLFOCUS)InvalidateRect(h,nullptr,FALSE);
    return result;
}

inline HWND workspaceTips{};

LRESULT CALLBACK detailProc(HWND h, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    // A text change and a resize can arrive together during navigation. Re-evaluate
    // wrapping after the edit control has processed both.
    constexpr UINT updateScrollbar = WM_APP + 26;
    if (msg == WM_SETTEXT || msg == WM_SIZE || msg == WM_SETFONT) {
        auto result = DefSubclassProc(h, msg, wp, lp);
        if (!GetPropW(h,L"DetailLayoutPending")) {
            SetPropW(h,L"DetailLayoutPending",reinterpret_cast<HANDLE>(1));
            PostMessageW(h,updateScrollbar,0,0);
        }
        return result;
    }
    if (msg == updateScrollbar) {
        RemovePropW(h,L"DetailLayoutPending");
        RECT content{}; SendMessageW(h, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&content));
        LONG lines = static_cast<LONG>(SendMessageW(h, EM_GETLINECOUNT, 0, 0));
        auto dc=GetDC(h);
        auto font=reinterpret_cast<HFONT>(SendMessageW(h,WM_GETFONT,0,0));
        auto old=SelectObject(dc,font);
        TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);SelectObject(dc,old);ReleaseDC(h,dc);
        bool needed=GetWindowTextLengthW(h)>0 && lines*metrics.tmHeight > content.bottom-content.top;
        if (needed != ((GetWindowLongPtrW(h,GWL_STYLE)&WS_VSCROLL)!=0))
            ShowScrollBar(h,SB_VERT,needed);
        return 0;
    }
    return DefSubclassProc(h,msg,wp,lp);
}

void enableHints() {
    workspaceTips = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                    CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,g.win,nullptr,GetModuleHandleW(nullptr),nullptr);
    auto add = [&](HWND control) {
        TOOLINFOW t{}; t.cbSize = sizeof(t); t.hwnd = g.win; t.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        t.uId = reinterpret_cast<UINT_PTR>(control); t.lpszText = LPSTR_TEXTCALLBACKW;
        SendMessageW(workspaceTips, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&t));
    };
    for (const auto& entry : g.controls) {
        wchar_t cls[16]{}; GetClassNameW(entry.second,cls,16);
        if (lstrcmpiW(cls,L"BUTTON")==0) add(entry.second);
    }
    for(HWND label : {g.path,g.meta,g.name,g.oldValue}) add(label);
}
