#pragma once

// TrackPopupMenu keeps the native keyboard, mouse and dismissal behavior.
constexpr int fontHeadingCommand=Theme::menuBase+Theme::count;
constexpr int firstFontCommand=fontHeadingCommand+1;
struct AppearanceMenuMetrics {
    int dpi;
    HFONT font;
    std::array<std::wstring,typography::fontCount> fontNames{};
};

bool appearanceMenuItem(UINT id) {
    return (id>=glass::firstFinishCommand && id<=glass::headingCommand) ||
           (id>=Theme::menuBase && id<firstFontCommand+typography::fontCount);
}

bool themeMenuMessage(UINT message,WPARAM,LPARAM param) {
    if(message!=WM_MEASUREITEM && message!=WM_DRAWITEM)return false;
    if(!param)return false;
    if(message==WM_MEASUREITEM) {
        auto& item=*reinterpret_cast<MEASUREITEMSTRUCT*>(param);
        if(item.CtlType!=ODT_MENU || !appearanceMenuItem(item.itemID))return false;
        const auto& metrics=*reinterpret_cast<const AppearanceMenuMetrics*>(item.itemData);
        item.itemWidth=MulDiv(typography::isChinese(i18n::language)?220:168,metrics.dpi,96);
        const bool heading=item.itemID==fontHeadingCommand || item.itemID==glass::headingCommand;
        item.itemHeight=MulDiv(heading?27:36,metrics.dpi,96);
        return true;
    }
    auto& item=*reinterpret_cast<DRAWITEMSTRUCT*>(param);
    if(item.CtlType!=ODT_MENU || !appearanceMenuItem(item.itemID))return false;
    const auto& metrics=*reinterpret_cast<const AppearanceMenuMetrics*>(item.itemData);
    auto measure=[&](int n){return MulDiv(n,metrics.dpi,96);};
    const bool active=(item.itemState&ODS_SELECTED)!=0;
    auto dc=item.hDC;
    auto bounds=item.rcItem;
    fill(dc,bounds,active?Theme::select:Theme::panel);
    int x=bounds.left+measure(12), y=(bounds.top+bounds.bottom)/2;
    RECT caption=bounds;caption.left=x;caption.right-=measure(25);
    if(item.itemID==fontHeadingCommand || item.itemID==glass::headingCommand) {
        RECT line{bounds.left,bounds.top,bounds.right,bounds.top+1};
        fill(dc,line,Theme::line);
        const auto heading=item.itemID==glass::headingCommand?glass::label():(i18n::language==0?L"中文字体":L"中文字體");
        literalText(dc,heading,caption,metrics.font,Theme::muted);
        return true;
    }
    const wchar_t* label;
    if(item.itemID<fontHeadingCommand) {
        const auto id=item.itemID-Theme::menuBase;
        const auto& sample=Theme::palettes[id];
        RECT swatch{x,y-measure(8),x+measure(25),y+measure(8)};
        fill(dc,swatch,Theme::line);
        InflateRect(&swatch,-measure(1),-measure(1));fill(dc,swatch,sample.bg);
        swatch.right=swatch.left+measure(8);fill(dc,swatch,sample.accent);
        caption.left+=measure(37);
        label=Theme::name(id);
    }else if(item.itemID>=glass::firstFinishCommand){
        const auto finish=static_cast<glass::Finish>(item.itemID-glass::firstFinishCommand);
        // The same band under each lens makes their relative transmission visible.
        // Paint the sample without changing the user's current selection.
        RECT lens{x,y-measure(8),x+measure(29),y+measure(8)};
        POINT band[]={{x+measure(6),y+measure(7)}, {x+measure(14),y-measure(7)},
                      {x+measure(22),y-measure(7)}, {x+measure(14),y+measure(7)}};
        const int saved=SaveDC(dc);
        SelectObject(dc,GetStockObject(NULL_PEN));
        SelectObject(dc,GetStockObject(DC_BRUSH));
        SetDCBrushColor(dc,Theme::accent);
        Polygon(dc,band,static_cast<int>(std::size(band)));
        RestoreDC(dc,saved);
        glass::surface(dc,lens,Theme::sidebar,measure(8),false,finish);
        caption.left+=measure(37);
        label=glass::finishLabel(finish);
    }else{
        label=metrics.fontNames[item.itemID-firstFontCommand].c_str();
    }
    auto ink=(item.itemState&ODS_DISABLED)?Theme::muted:(active?Theme::accentInk:Theme::text);
    literalText(dc,label,caption,metrics.font,ink);
    if(item.itemState&ODS_CHECKED) {
        auto pen=CreatePen(PS_SOLID,std::max(1,measure(2)),Theme::accentInk);
        auto before=SelectObject(dc,pen);
        x=bounds.right-measure(24);
        MoveToEx(dc,x,y,nullptr);LineTo(dc,x+measure(4),y+measure(4));LineTo(dc,x+measure(12),y-measure(5));
        SelectObject(dc,before);DeleteObject(pen);
    }
    return true;
}

void populateAppearanceMenu(HMENU menu,AppearanceMenuMetrics& metrics) {
    auto append=[&](int command,const wchar_t* label,UINT state) {
        MENUITEMINFOW item{sizeof(item)};
        item.fMask=MIIM_ID|MIIM_FTYPE|MIIM_STRING|MIIM_STATE|MIIM_DATA;
        item.fType=MFT_OWNERDRAW;item.fState=state;
        item.dwTypeData=const_cast<wchar_t*>(label);
        item.dwItemData=reinterpret_cast<ULONG_PTR>(&metrics);
        item.wID=command;
        InsertMenuItemW(menu,GetMenuItemCount(menu),TRUE,&item);
    };
    for(int i=0;i<Theme::count;++i)
        append(Theme::menuBase+i,Theme::name(i),i==Theme::index()?MFS_CHECKED:MFS_ENABLED);
    append(glass::headingCommand,glass::label(),MFS_DISABLED);
    for(auto finish:{glass::Clear,glass::Frosted,glass::Solid}) {
        UINT state=glass::systemAllows?MFS_ENABLED:MFS_DISABLED;
        if(finish==glass::effectiveFinish())state|=MFS_CHECKED;
        append(glass::firstFinishCommand+finish,glass::finishLabel(finish),state);
    }
    if(!typography::isChinese(i18n::language))return;

    const bool simplified=i18n::language==0;
    const auto families=typography::chineseFamilies(i18n::language);
    const auto automatic=typography::effectiveFont(families,typography::Automatic);
    const auto checked=families[typography::selected].text?typography::selected:typography::Automatic;
    metrics.fontNames={simplified?L"自动":L"自動",simplified?L"微软雅黑 UI":L"微軟正黑體 UI",
                       simplified?L"Noto Sans SC":L"Noto Sans TC",simplified?L"苹方 SC":L"蘋方 TC"};
    const auto current=automatic==typography::Automatic?(simplified?L"系统默认":L"系統預設"):metrics.fontNames[automatic];
    metrics.fontNames[typography::Automatic]+=L"（"+current+L"）";
    append(fontHeadingCommand,simplified?L"中文字体":L"中文字體",MFS_DISABLED);
    for(int i=0;i<typography::fontCount;++i) {
        const bool available=i==typography::Automatic || families[i].text;
        if(!available)metrics.fontNames[i]+=simplified?L"（未安装）":L"（未安裝）";
        UINT state=available?MFS_ENABLED:MFS_DISABLED;
        if(i==checked)state|=MFS_CHECKED;
        append(firstFontCommand+i,metrics.fontNames[i].c_str(),state);
    }
}

int pickAppearance(HWND owner,HWND anchor) {
    auto menu=CreatePopupMenu();
    if(!menu)return 0;
    AppearanceMenuMetrics metrics{static_cast<int>(GetDpiForWindow(owner)),reinterpret_cast<HFONT>(SendMessageW(anchor,WM_GETFONT,0,0))};
    if(!metrics.dpi)metrics.dpi=96;
    auto brush=CreateSolidBrush(Theme::panel);
    MENUINFO style{sizeof(style)};style.fMask=MIM_BACKGROUND;style.hbrBack=brush;SetMenuInfo(menu,&style);
    populateAppearanceMenu(menu,metrics);
    RECT anchorBounds{};GetWindowRect(anchor,&anchorBounds);
    TPMPARAMS pos{sizeof(pos),anchorBounds};
    const int command=TrackPopupMenuEx(menu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,
                                          anchorBounds.left,anchorBounds.bottom,owner,&pos);
    DestroyMenu(menu);DeleteObject(brush);
    return command;
}
