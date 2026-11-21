#pragma once

// TrackPopupMenu keeps the native keyboard, mouse and dismissal behavior.
struct AppearanceMenuMetrics { int dpi; HFONT font; };

bool themeMenuMessage(UINT message,WPARAM,LPARAM param) {
    if(message!=WM_MEASUREITEM && message!=WM_DRAWITEM)return false;
    if(!param)return false;
    if(message==WM_MEASUREITEM) {
        auto& item=*reinterpret_cast<MEASUREITEMSTRUCT*>(param);
        if(item.CtlType!=ODT_MENU || item.itemID<Theme::menuBase || item.itemID>=Theme::menuBase+Theme::count)return false;
        const auto& metrics=*reinterpret_cast<const AppearanceMenuMetrics*>(item.itemData);
        item.itemWidth=MulDiv(168,metrics.dpi,96);
        item.itemHeight=MulDiv(38,metrics.dpi,96);
        return true;
    }
    auto& item=*reinterpret_cast<DRAWITEMSTRUCT*>(param);
    if(item.CtlType!=ODT_MENU || item.itemID<Theme::menuBase || item.itemID>=Theme::menuBase+Theme::count)return false;
    const auto& metrics=*reinterpret_cast<const AppearanceMenuMetrics*>(item.itemData);
    auto measure=[&](int n){return MulDiv(n,metrics.dpi,96);};
    const int id=item.itemID-Theme::menuBase;
    const auto& sample=Theme::palettes[id];
    const bool active=(item.itemState&ODS_SELECTED)!=0;
    auto dc=item.hDC;
    auto bounds=item.rcItem;
    fill(dc,bounds,active?Theme::select:Theme::panel);
    int x=bounds.left+measure(12), y=(bounds.top+bounds.bottom)/2;
    RECT swatch{x,y-measure(8),x+measure(25),y+measure(8)};
    fill(dc,swatch,Theme::line);
    InflateRect(&swatch,-measure(1),-measure(1));fill(dc,swatch,sample.bg);
    swatch.right=swatch.left+measure(8);fill(dc,swatch,sample.accent);
    RECT caption=bounds;caption.left=x+measure(37);caption.right-=measure(26);
    literalText(dc,Theme::name(id),caption,metrics.font,active?Theme::accentInk:Theme::text);
    if(id==Theme::index()) {
        auto pen=CreatePen(PS_SOLID,std::max(1,measure(2)),Theme::accentInk);
        auto before=SelectObject(dc,pen);
        x=bounds.right-measure(24);
        MoveToEx(dc,x,y,nullptr);LineTo(dc,x+measure(4),y+measure(4));LineTo(dc,x+measure(12),y-measure(5));
        SelectObject(dc,before);DeleteObject(pen);
    }
    return true;
}

int pickAppearance(HWND owner,HWND anchor) {
    auto menu=CreatePopupMenu();
    if(!menu)return -1;
    AppearanceMenuMetrics metrics{static_cast<int>(GetDpiForWindow(owner)),reinterpret_cast<HFONT>(SendMessageW(anchor,WM_GETFONT,0,0))};
    if(!metrics.dpi)metrics.dpi=96;
    auto brush=CreateSolidBrush(Theme::panel);
    MENUINFO style{sizeof(style)};style.fMask=MIM_BACKGROUND;style.hbrBack=brush;SetMenuInfo(menu,&style);
    for(int i=0;i<Theme::count;++i) {
        MENUITEMINFOW item{sizeof(item)};
        item.fMask=MIIM_ID|MIIM_FTYPE|MIIM_STRING|MIIM_STATE|MIIM_DATA;
        item.fType=MFT_OWNERDRAW;
        item.fState=i==Theme::index()?MFS_CHECKED:MFS_ENABLED;
        item.dwTypeData=const_cast<wchar_t*>(Theme::name(i));
        item.dwItemData=reinterpret_cast<ULONG_PTR>(&metrics);
        item.wID=Theme::menuBase+i;
        InsertMenuItemW(menu,i,TRUE,&item);
    }
    RECT anchorBounds{};GetWindowRect(anchor,&anchorBounds);
    TPMPARAMS pos{sizeof(pos),anchorBounds};
    const int command=TrackPopupMenuEx(menu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,
                                          anchorBounds.left,anchorBounds.bottom,owner,&pos);
    DestroyMenu(menu);DeleteObject(brush);
    return command?command-Theme::menuBase:-1;
}
