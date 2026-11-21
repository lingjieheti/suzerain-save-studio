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
    a.gap = a.width < 800 ? 9 : 14;
    int margin = a.width < 900 ? 12 : 22;
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
    a.listY = a.toolsY + 40;
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
    bool primary = id == LATEST || id == APPLY || (id == CONFIRM_SAVE && !g.demoResource) ||
                   (id == SAVEAS && g.demoResource) || id == BACKUP_RESTORE ||
                   (id == SAVE && (!pending().empty() || g.editingDirty));
    COLORREF bg = Theme::panel, border = Theme::line, fg = Theme::text;
    if (nav) {
        bg = activeNav ? Theme::sideLine : Theme::sidebar;
        if (hover && !activeNav) bg = RGB(43, 68, 58);
        border = bg; fg = activeNav ? Theme::sideText : Theme::sideMuted;
    } else if (primary) {
        bg = border = hover ? RGB(26, 70, 51) : Theme::accent; fg = Theme::sideText;
    } else if (toggle || hover || down) {
        bg = Theme::select; border = toggle ? Theme::accent : Theme::line;
    } else if (check) {
        bg = border = Theme::bg;
    }
    if (disabled) { bg = Theme::stripe; fg = Theme::dim; border = Theme::line; }
    RECT r = d->rcItem;
    fill(d->hDC, r, nav ? Theme::sidebar : Theme::bg);
    rounded(d->hDC, r, bg, border, nav ? 6 : 8);
    RECT tx = r;
    if (nav) {
        bool compact = r.right - r.left < px(64);
        int glyphX = compact ? (r.right - px(18)) / 2 : px(13);
        navSymbol(d->hDC, id - NAV_BASE, glyphX, (r.bottom - px(18)) / 2,
                  activeNav ? Theme::gold : Theme::sideMuted);
        if (activeNav) {
            RECT strip{0, px(7), px(3), r.bottom - px(7)}; fill(d->hDC, strip, Theme::gold);
        }
        if (!compact) {
            tx.left += px(42); tx.right -= px(5);
            drawText(d->hDC, textOf(d->hwndItem), tx, g.font, fg);
        }
    } else {
        if (check) {
            int y = (r.bottom - px(15)) / 2;
            RECT box{px(5), y, px(20), y + px(15)};
            rounded(d->hDC, box, checked ? Theme::accent : Theme::input, checked ? Theme::accent : Theme::dim, 5);
            if (checked) {
                auto pen = CreatePen(PS_SOLID, px(2), Theme::sideText);
                auto old = SelectObject(d->hDC, pen);
                MoveToEx(d->hDC, px(8), y+px(8), nullptr); LineTo(d->hDC, px(11), y+px(11));
                LineTo(d->hDC, px(17), y+px(4)); SelectObject(d->hDC, old); DeleteObject(pen);
            }
            tx.left += px(25);
        }
        drawText(d->hDC, textOf(d->hwndItem), tx, g.font, fg,
                 (check ? DT_LEFT : DT_CENTER) | DT_VCENTER | DT_SINGLELINE);
    }
    if ((d->itemState & ODS_FOCUS) && !disabled) {
        RECT focus = r; InflateRect(&focus, -px(4), -px(4)); DrawFocusRect(d->hDC, &focus);
    }
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
