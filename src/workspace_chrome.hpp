#pragma once

// All layout measurements are device-independent pixels.
struct WorkspaceGeometry {
    int width, height, rail, gap, left, right, inspector, content, list;
    int headerBottom, metricHeight, presetsY, toolsY, listY, bottom, footer;
    bool compactRail, shortScreen;
};

bool stackAdvancedFilters(const WorkspaceGeometry& a) { return g.tab==4 && a.list<560; }

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
    a.headerBottom = a.shortScreen ? 78 : 106;
    if (g.tab == 0 && a.height >= 540)
        a.metricHeight = a.height < 710 ? 58 : 90;
    a.presetsY = a.headerBottom + (a.metricHeight ? a.metricHeight + 11 : 0);
    a.toolsY = g.tab == 0 ? a.presetsY + 39 : a.headerBottom;
    a.listY = a.toolsY + (spacious ? 46 : 40);
    if(stackAdvancedFilters(a))a.listY+=36;
    a.footer = a.height - (a.shortScreen ? 46 : 54);
    a.bottom = a.footer - 10;
    return a;
}

void fill(HDC dc, RECT bounds, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &bounds, brush);
    DeleteObject(brush);
}

int fileToolbarWidth(const WorkspaceGeometry& a) { return std::min(465,a.content*62/100); }
int footerHistoryWidth(const WorkspaceGeometry& a) { return a.width<820?58:80; }
int footerSaveWidth(const WorkspaceGeometry& a) { return a.width<820?107:155; }

void drawWorkspaceBackdrop(HDC dc, const RECT& client, const WorkspaceGeometry& a) {
    glass::wash(dc,client,Theme::bg);
    const int inset=a.compactRail?4:7;
    glass::surface(dc,rect(inset,6,a.rail-2*inset,a.height-12),Theme::sidebar,px(a.compactRail?21:24));
    const int end=a.left+a.content;
    const int toolbarWidth=fileToolbarWidth(a);
    const int toolbarY=a.shortScreen?7:14;
    glass::surface(dc,rect(end-toolbarWidth-4,toolbarY-4,toolbarWidth+8,40),Theme::panel,px(20));
    const int fileGroupEnd=end-toolbarWidth+4*((toolbarWidth-18)/10)+9;
    fill(dc,rect(fileGroupEnd,toolbarY+8,1,16),Theme::line);
    if(g.tab!=7) {
        const int width=2*footerHistoryWidth(a)+5+(g.tab==5?0:footerSaveWidth(a)+7);
        glass::surface(dc,rect(end-width-4,a.footer+3,width+8,40),Theme::panel,px(20));
    }
}

// The parent and its child HWNDs sample the same background at their native
// coordinates. This keeps a floating toolbar continuous through button repaints.
void paintWorkspaceChildBackdrop(HDC dc, HWND child) {
    struct Backdrop {
        HDC dc{};
        HBITMAP bitmap{};
        SIZE size{};
        UINT dpi{};
        int theme=-1,page=-1;
        bool glassEnabled{};
        ~Backdrop() { if(dc)DeleteDC(dc); if(bitmap)DeleteObject(bitmap); }
    };
    static Backdrop cache;
    RECT area{},part{};
    GetClientRect(g.win,&area);
    if(area.right<=0 || area.bottom<=0)return;
    const bool enabled=glass::enabled();
    const int page=g.tab==7?7:g.tab==5?5:0;
    if(cache.size.cx!=area.right || cache.size.cy!=area.bottom || !cache.bitmap) {
        if(!cache.dc)cache.dc=CreateCompatibleDC(dc);
        auto bitmap=cache.dc?CreateCompatibleBitmap(dc,area.right,area.bottom):nullptr;
        if(!bitmap){
            GetClientRect(child,&part); fill(dc,part,Theme::bg); return;
        }
        SelectObject(cache.dc,bitmap);
        if(cache.bitmap)DeleteObject(cache.bitmap);
        cache.bitmap=bitmap; cache.size={area.right,area.bottom}; cache.theme=-1;
    }
    if(cache.theme!=Theme::index() || cache.page!=page || cache.dpi!=g.dpi || cache.glassEnabled!=enabled) {
        drawWorkspaceBackdrop(cache.dc,area,workspaceGeometry());
        cache.theme=Theme::index();cache.page=page;cache.dpi=g.dpi;cache.glassEnabled=enabled;
    }
    POINT origin{};
    MapWindowPoints(child,g.win,&origin,1);
    GetClientRect(child,&part);
    BitBlt(dc,0,0,part.right,part.bottom,cache.dc,origin.x,origin.y,SRCCOPY);
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
        case 8: {
            POINT curve[]={{9,16},{7,14},{1,10},{1,6},{1,1},{6,0},{9,5},
                           {12,0},{17,1},{17,6},{17,10},{11,14},{9,16}};
            for(auto& point:curve){point.x=x+px(point.x);point.y=y+px(point.y);}
            PolyBezier(dc,curve,static_cast<DWORD>(std::size(curve)));
            break;
        }
        case RELOAD:
            Arc(dc,x+px(2),y+px(2),x+px(16),y+px(16),x+px(15),y+px(6),x+px(16),y+px(11));
            stroke({{11,6},{16,6},{16,1}}); break;
        case FOLDER:
            stroke({{1,15},{1,4},{7,4},{9,6},{16,6},{16,15},{1,15}}); break;
        case OPEN:
            stroke({{4,15},{2,15},{2,2},{10,2},{14,6},{14,8}});
            stroke({{10,2},{10,6},{14,6}}); stroke({{7,13},{17,13},{13,9}});
            stroke({{17,13},{13,17}}); break;
        case LATEST:
            stroke({{2,3},{16,3},{16,15},{2,15},{2,3}}); stroke({{6,7},{13,7}});
            stroke({{6,11},{11,11}}); break;
        case HISTORY_UNDO:
        case HISTORY_REDO: {
            POINT points[]={{16,14},{16,8},{12,6},{4,6}};
            const bool redo=id==HISTORY_REDO;
            for(auto& point:points){
                point.x=x+px(redo?18-point.x:point.x);point.y=y+px(point.y);
            }
            PolyBezier(dc,points,4);
            if(redo)stroke({{10,2},{15,6},{10,10}});
            else stroke({{8,2},{3,6},{8,10}});
            break;
        }
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
    bool nav = (id >= NAV_BASE && id < NAV_BASE + 8) || id==SUPPORT_AUTHOR;
    bool activeNav = id==NAV_BASE+g.tab;
    bool check = id == AUTOCAP || id == FILTER_CHANGED;
    bool checked = id == AUTOCAP ? g.autoCap : g.changedOnly;
    bool toggle = (id == BOOL_FALSE && !g.boolValue) || (id == BOOL_TRUE && g.boolValue) ||
                  (id == FILTER_FAVORITES && g.favoritesOnly) ||
                  (id == FIELD_FAVORITE && g.favorites.count(g.selected));
    const bool emphasizeApply=g.editingDirty;
    bool primary = (id == LATEST && !g.doc) || (id == APPLY && emphasizeApply) || (id == CONFIRM_SAVE && !g.demoResource) ||
                   (id == SAVEAS && g.demoResource) || id == BACKUP_RESTORE ||
                   (id == SAVE && !g.editingDirty && !pending().empty());
    const bool fileAction=id==FOLDER || id==RELOAD || id==OPEN || id==LATEST;
    const bool history=id==HISTORY_UNDO || id==HISTORY_REDO;
    bool quiet = fileAction || id==UNDOONE || id==FIELD_FAVORITE || history;
    bool inspector = id==APPLY || id==UNDOONE || id==BOOL_FALSE || id==BOOL_TRUE ||
                     id==SAVEAS || id==CONFIRM_SAVE || id==PATCH_EXPORT || id==REVIEW_REVERT ||
                     id==UNDOALL || id==BACKUP_RESTORE || id==BACKUP_CANCEL || id==COPY_KEY || id==EXPORT_CSV || id==FIELD_FAVORITE;
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
    if(inspector)fill(d->hDC,r,Theme::panel);
    else paintWorkspaceChildBackdrop(d->hDC,d->hwndItem);
    const bool grouped=nav || fileAction || history || id==SAVE;
    if(nav && (activeNav || hover || down)) {
        glass::surface(d->hDC,r,activeNav?Theme::select:Theme::sideHover,px(13),activeNav);
    } else if(grouped) {
        if(primary || (!disabled && (hover || down)))
            rounded(d->hDC,r,bg,border,30);
    } else if(id==APPEARANCE) {
        glass::surface(d->hDC,r,hover?Theme::select:Theme::input,px(14));
    } else if(!check && (!quiet || toggle || hover || down || primary)) {
        rounded(d->hDC,r,bg,border,15);
    }
    if(fileAction && id==LATEST && !primary && !disabled)fg=Theme::accentInk;
    RECT tx = r;
    if (nav) {
        bool compact = r.right - r.left < px(64);
        const bool longSupport=id==SUPPORT_AUTHOR && !compact &&
            typography::width(d->hDC,g.font,textOf(d->hwndItem))>r.right-r.left-px(47);
        int glyphX = compact ? (r.right - px(18)) / 2 : px(longSupport?6:13);
        navSymbol(d->hDC, id==SUPPORT_AUTHOR?8:id-NAV_BASE, glyphX, (r.bottom - px(18)) / 2,
                  activeNav ? Theme::accentInk : Theme::sideMuted);
        if (!compact) {
            tx.left += px(longSupport?27:42); tx.right -= px(5);
            auto title=textOf(d->hwndItem);
            if(id==NAV_BASE+5 && g.previewCount) {
                auto count=std::to_wstring(g.previewCount);
                int badgeWidth=px(12+8*static_cast<int>(count.size()));
                RECT badge{r.right-px(8)-badgeWidth,r.bottom/2-px(9),r.right-px(8),r.bottom/2+px(9)};
                rounded(d->hDC,badge,Theme::select,Theme::select,15);
                drawText(d->hDC,count,badge,g.small,Theme::accentInk,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                tx.right=badge.left-px(4);
                title=tr(L"修改预览");
            }
            drawText(d->hDC, title, tx, longSupport?g.small:activeNav?g.emphasis:g.font, fg);
        }
    } else if(id==APPEARANCE) {
        const bool compact=workspaceGeometry().compactRail;
        const int cy=(r.top+r.bottom)/2;
        RECT mark{px(9),cy-px(5),px(19),cy+px(5)};
        rounded(d->hDC,mark,Theme::accent,Theme::accent,5);
        tx.left=px(26);tx.right-=px(17);
        auto caption=textOf(d->hwndItem);
        if(compact || typography::width(d->hDC,g.small,caption)>tx.right-tx.left)
            caption=Theme::name(Theme::index());
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
        const auto caption=textOf(d->hwndItem);
        const auto font=primary?g.emphasis:g.font;
        if(fileAction || history) {
            const int textWidth=typography::width(d->hDC,font,caption);
            const int groupWidth=px(22)+textWidth;
            if(groupWidth+px(10)<=r.right-r.left) {
                const int iconX=(r.right-r.left-groupWidth)/2;
                navSymbol(d->hDC,id,iconX,(r.bottom-px(18))/2,fg);
                tx.left=iconX+px(22);
                tx.right=tx.left+textWidth+px(2);
            }
        }
        drawText(d->hDC, caption, tx, font, fg,
                 (check ? DT_LEFT : DT_CENTER) | DT_VCENTER | DT_SINGLELINE);
    }
    if ((d->itemState & ODS_FOCUS) && !(d->itemState & ODS_NOFOCUSRECT) && !disabled) {
        RECT focus = r; InflateRect(&focus, -px(4), -px(4));
        auto pen=CreatePen(PS_SOLID,std::max(1,px(2)),primary?Theme::onAccent:Theme::focus);
        auto oldPen=SelectObject(d->hDC,pen);auto oldBrush=SelectObject(d->hDC,GetStockObject(HOLLOW_BRUSH));
        RoundRect(d->hDC,focus.left,focus.top,focus.right,focus.bottom,px(12),px(12));
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
    fill(dc,r,Theme::panel);
    if(selected) {
        RECT band=r;
        InflateRect(&band,-px(3),-px(2));
        rounded(dc,band,Theme::select,Theme::select,11);
    }
    if(selected || changed) {
        RECT mark{r.left+px(4),r.top+px(8),r.left+px(selected?7:6),r.bottom-px(8)};
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
        HFONT face=col==3?g.small:g.font;
        if(col==0 && selected)face=g.emphasis;
        if(g.tab<=5 && (col==1 || col==2) && ((*value>=L'0' && *value<=L'9') || *value==L'-' || *value==L'+'))
            face=g.tableNumber;
        literalText(dc,value,cell,face,ink,gridAlignment(col));
    }
    if(!selected) {
        RECT rule{px(13),r.bottom-1,r.right-px(9),r.bottom};fill(dc,rule,Theme::stripe);
    }
    if((state&LVIS_FOCUSED) && GetFocus()==g.grid) {
        auto pen=CreatePen(PS_SOLID,std::max(1,px(1)),Theme::focus);auto old=SelectObject(dc,pen);
        auto brush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
        RoundRect(dc,r.left+px(3),r.top+px(2),r.right-px(3),r.bottom-px(2),px(10),px(10));
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
    paintWorkspaceChildBackdrop(dc,h);
    glass::surface(dc,r,Theme::input,px(12));
    if(GetFocus()==h){
        auto pen=CreatePen(PS_SOLID,std::max(1,px(2)),Theme::focus);
        auto oldPen=SelectObject(dc,pen),oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
        RoundRect(dc,r.left+1,r.top+1,r.right-1,r.bottom-1,px(21),px(21));
        SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);
    }
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
        if(g.tab<5 && h==g.desc) {
            RECT bounds{};GetWindowRect(h,&bounds);
            MapWindowPoints(nullptr,g.win,reinterpret_cast<POINT*>(&bounds),2);
            const auto area=workspaceGeometry();
            const int inset=area.shortScreen?10:16;
            int room=std::max(px(12),px(area.bottom-inset)-static_cast<int>(bounds.top));
            int height=std::min(room,std::max(px(32),static_cast<int>(lines*metrics.tmHeight)+px(8)));
            if(bounds.bottom-bounds.top!=height) {
                SetWindowPos(h,nullptr,0,0,bounds.right-bounds.left,height,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
                InvalidateRect(g.win,nullptr,FALSE);
                return 0;
            }
        }
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
