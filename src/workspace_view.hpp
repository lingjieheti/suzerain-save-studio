#pragma once

int navStart(const WorkspaceGeometry& a) { return a.compactRail ? 65 : (a.shortScreen ? 87 : 138); }
int navStep(const WorkspaceGeometry& a) { return a.shortScreen ? 33 : 43; }
int navGap(const WorkspaceGeometry& a) { return a.shortScreen || a.compactRail ? 12 : 30; }
int navY(const WorkspaceGeometry& a, int index) {
    return navStart(a) + index * navStep(a) + (index >= 5 ? navGap(a) : 0);
}
bool compactFieldTools(const WorkspaceGeometry& a) { return a.list < (g.tab==4 ? 560 : 450); }
int pendingFilterWidth() {
    const auto text=tr(L"只看待保存");
    auto dc=GetDC(g.win);auto old=SelectObject(dc,g.font);
    SIZE extent{};GetTextExtentPoint32W(dc,text.c_str(),static_cast<int>(text.size()),&extent);
    SelectObject(dc,old);ReleaseDC(g.win,dc);
    return 33+MulDiv(extent.cx,96,g.dpi);
}
int searchWidth(const WorkspaceGeometry& a) {
    if(stackAdvancedFilters(a))return a.list;
    bool compact=compactFieldTools(a);
    return a.list - pendingFilterWidth() - 6 - (g.tab==4 ? (compact ? 153 : 213) : 0);
}
int inspectorPad(const WorkspaceGeometry& a) { return a.inspector < 218 ? 9 : 18; }

void layout() {
    if (!g.grid) return;
    auto a = workspaceGeometry();
    bool edit = g.tab < 5, review = g.tab == 5, backup = g.tab == 6, help = g.tab == 7;
    g.gridTop = a.listY; g.tableW = a.list; g.rightX = a.right;
    const int toolbarEnd = a.left + a.content;
    int toolsWidth = std::min(465, a.content * 62 / 100);
    int slot = (toolsWidth-18) / 10;
    int tx = toolbarEnd - toolsWidth;
    for (auto item : {std::pair<int,int>{RELOAD,2}, {FOLDER,2}, {OPEN,3}, {LATEST,3}}) {
        int bw = item.first == LATEST ? toolbarEnd - tx : slot * item.second;
        place(ctl(item.first), tx, a.shortScreen ? 7 : 14, bw, 32);
        tx += bw + 6;
    }
    int themeWidth=116;
    if(!a.compactRail) {
        auto dc=GetDC(g.win);
        const int caption=typography::width(dc,g.small,textOf(ctl(APPEARANCE)));
        ReleaseDC(g.win,dc);
        themeWidth=std::clamp(MulDiv(caption,96,g.dpi)+46,145,200);
    }
    int themeRight=toolbarEnd-(a.compactRail?121:0);
    place(ctl(APPEARANCE),themeRight-themeWidth,a.shortScreen?44:55,themeWidth,29);
    int pathWidth=themeRight-themeWidth-a.left-8;
    place(g.meta, a.left, a.shortScreen ? 39 : 54, pathWidth, 20);
    place(g.path, a.left, a.shortScreen ? 57 : 75, pathWidth, 19);
    if (a.compactRail) place(ctl(LANGUAGE), toolbarEnd-116, a.shortScreen ? 43 : 57,116,300);
    else place(ctl(LANGUAGE), 15, a.height - (a.shortScreen ? 39 : 73), a.rail-30, 300);
    for (int i=0;i<8;++i) place(ctl(NAV_BASE+i), a.compactRail ? 6 : 10, navY(a,i),
                                  a.rail - (a.compactRail ? 12 : 20), navStep(a)-4);

    int presetW = a.content < 610 ? 104 : 138;
    place(ctl(PRESET99), toolbarEnd-2*presetW-7, a.presetsY, presetW, 31);
    place(ctl(PRESET999), toolbarEnd-presetW, a.presetsY, presetW, 31);
    place(ctl(AUTOCAP), a.left, a.presetsY, std::min(270,a.content-2*presetW-14), 31);
    int sw = searchWidth(a);
    const bool showClear=edit && GetWindowTextLengthW(g.search)>0;
    place(g.search, a.left+29, a.toolsY+6, sw-37-(showClear?24:0), 22);
    place(ctl(CLEAR_FIELD_SEARCH),a.left+sw-29,a.toolsY+3,26,26);
    visible(CLEAR_FIELD_SEARCH,showClear);
    bool compactTools=compactFieldTools(a), stacked=stackAdvancedFilters(a);
    int filtersY=a.toolsY+(stacked?37:0);
    int changedW=stacked?std::max(a.list*40/100,pendingFilterWidth()):pendingFilterWidth();
    int favoritesW=stacked?a.list*26/100:compactTools?70:100;
    int patchW=stacked?a.list-changedW-favoritesW-10:compactTools?74:104;
    int filtersX=stacked?a.left:a.left+sw+5;
    place(ctl(FILTER_CHANGED),filtersX,filtersY,changedW,32);
    place(ctl(FILTER_FAVORITES),filtersX+changedW+5,filtersY,favoritesW,32);
    place(ctl(PATCH_IMPORT),a.right-a.gap-patchW,filtersY,patchW,32);
    place(ctl(BACKUP_REFRESH), a.left, a.toolsY, 96, 32);
    place(ctl(BACKUP_FOLDER), a.left+102, a.toolsY, std::min(165,a.list-102), 32);
    place(ctl(HELP_USAGE), a.left, a.toolsY, 120, 32);
    place(ctl(HELP_LICENSES), a.left+126, a.toolsY, 120, 32);
    int listY = review ? a.toolsY : a.listY;
    place(g.grid, a.left+1, listY+1, a.list-2, a.bottom-listY-2);
    place(g.empty, a.left+12, listY+std::max(35,(a.bottom-listY)/2-14), a.list-24, 44);
    place(g.helpText, a.left+14,a.listY+13,a.content-28,a.bottom-a.listY-26);

    const int pad = inspectorPad(a), rx = a.right+pad, rw = a.inspector-2*pad;
    int y = a.toolsY+(a.shortScreen?12:19);
    int nameH = edit ? (a.inspector<235?47:a.shortScreen?25:34) : (a.shortScreen ? 32 : 43);
    place(g.name,rx,y,rw,nameH);
    y += nameH + 3;
    int favoriteW=std::clamp(rw/2,70,100);
    place(g.oldValue,rx,y,edit?rw-favoriteW-5:rw,20);
    if(edit)place(ctl(FIELD_FAVORITE),rx+rw-favoriteW,y-3,favoriteW,25);
    if (edit) {
        y += a.shortScreen ? 27 : 34;
        place(g.valueLabel,rx,y,rw,20);
        y += 20;
        place(g.edit,rx+8,y+3,rw-16,29);
        place(ctl(BOOL_FALSE),rx,y,(rw-6)/2,35);
        place(ctl(BOOL_TRUE),rx+(rw-6)/2+6,y,rw-(rw-6)/2-6,35);
        y += 42;
        int first = (rw-6)*54/100;
        place(ctl(APPLY),rx,y,first,33); place(ctl(UNDOONE),rx+first+6,y,rw-first-6,33);
        y += a.shortScreen ? 40 : 47;
        if (g.tab==4) {
            place(ctl(COPY_KEY),rx,y,(rw-6)/2,30);
            place(ctl(EXPORT_CSV),rx+(rw-6)/2+6,y,rw-(rw-6)/2-6,30);
            y += 37;
        }
    } else if (review) {
        // A computed row can refer to its resource. Its caption is supplied by the review model.
        y += 24;
        place(ctl(CONFIRM_SAVE),rx,y,rw,33); y+=39;
        if (a.bottom-y>=205) {
            for (int id : std::initializer_list<int>{SAVEAS,PATCH_EXPORT,REVIEW_REVERT,UNDOALL}) {
                place(ctl(id),rx,y,rw,30); y+=35;
            }
        } else {
            int half=(rw-6)/2;
            place(ctl(SAVEAS),rx,y,half,30); place(ctl(PATCH_EXPORT),rx+half+6,y,rw-half-6,30); y+=36;
            place(ctl(REVIEW_REVERT),rx,y,half,30); place(ctl(UNDOALL),rx+half+6,y,rw-half-6,30); y+=38;
        }
    } else if (backup) {
        y += 26;
        place(ctl(BACKUP_RESTORE),rx,y,rw,33); y+=40;
        place(ctl(BACKUP_CANCEL),rx,y,rw,30);
        y+=36;
    }
    if (!a.shortScreen) y+=9;
    place(g.desc,rx,y,rw,std::max(12,a.bottom-y-10));
    int saveW = a.width < 820 ? 107 : 155, historyW = a.width < 820 ? 58 : 80;
    int footerRight = toolbarEnd;
    if (!review && !help) { footerRight -= saveW; place(ctl(SAVE),footerRight,a.footer+7,saveW,32); footerRight-=7; }
    footerRight-=historyW; place(ctl(HISTORY_REDO),footerRight,a.footer+7,historyW,32);
    footerRight-=historyW+5; place(ctl(HISTORY_UNDO),footerRight,a.footer+7,historyW,32);
    place(g.notice,a.left+15,a.footer+13,(help?toolbarEnd:footerRight)-a.left-20,24);

    visible(g.grid,!help); visible(g.search,edit); visible(FILTER_CHANGED,edit);
    visible(FILTER_FAVORITES,g.tab==4);EnableWindow(ctl(FILTER_FAVORITES),g.doc!=nullptr);
    visible(FIELD_FAVORITE,edit);
    visible(g.name,!help); visible(g.oldValue,!help); visible(g.desc,!help);
    visible(g.valueLabel,edit); visible(APPLY,edit); visible(UNDOONE,edit);
    visible(COPY_KEY,g.tab==4); visible(EXPORT_CSV,g.tab==4); EnableWindow(ctl(EXPORT_CSV),g.doc!=nullptr);
    auto* f = g.doc ? g.doc->find(g.selected) : nullptr;
    visible(g.edit,edit&&(!f||!f->is_bool)); visible(BOOL_FALSE,edit&&f&&f->is_bool); visible(BOOL_TRUE,edit&&f&&f->is_bool);
    visible(PRESET99,g.tab==0); visible(PRESET999,g.tab==0); visible(AUTOCAP,g.tab==0&&!isBase());
    visible(PATCH_IMPORT,g.tab==4); visible(PATCH_EXPORT,review); visible(CONFIRM_SAVE,review);
    visible(SAVEAS,review); visible(UNDOALL,review); visible(REVIEW_REVERT,review);
    visible(BACKUP_REFRESH,backup); visible(BACKUP_FOLDER,backup);
    visible(BACKUP_RESTORE,backup); visible(BACKUP_CANCEL,backup&&g.restoreArmed);
    visible(g.helpText,help); visible(HELP_USAGE,help); visible(HELP_LICENSES,help);
    visible(SAVE,!review&&!help); visible(HISTORY_UNDO,!help); visible(HISTORY_REDO,!help);
    bool empty=!help && (backup?g.backups.empty():g.rows.empty()); visible(g.empty,empty);
    if (empty) {
        bool emptyFavorites=g.tab==4&&g.favoritesOnly&&!g.changedOnly&&g.pages[4].query.empty();
        setText(g.empty,!g.doc?L"选择存档，或拖入 JSON 文件":backup?L"暂无备份":review?L"暂无修改":
                emptyFavorites?L"当前存档没有已收藏的字段。关闭筛选后可选择字段收藏。":L"无匹配项目");
    }
    if (!help) {
        RECT client{}; GetClientRect(g.grid,&client);
        int avail=MulDiv(client.right,96,g.dpi)-3;
        int widths[4]{};
        auto dc=GetDC(g.grid);
        auto columnWidth=[&](const wchar_t* caption,int insets) {
            return MulDiv(typography::width(dc,g.small,tr(caption)),96,g.dpi)+insets;
        };
        if (backup) {
            widths[0]=std::min(156,avail*40/100); widths[1]=std::min(72,avail/5); widths[2]=avail-widths[0]-widths[1];
        } else if (review) {
            const int budget=std::clamp(avail/5,51,108);
            widths[1]=std::max(budget,columnWidth(L"原值",18));
            widths[2]=std::max(budget,columnWidth(L"保存为",18));
            widths[0]=avail-widths[1]-widths[2];
        } else {
            int typeW=0;
            for(const wchar_t* caption : {L"类型",L"数值",L"开关"})
                typeW=std::max(typeW,columnWidth(caption,16));
            widths[3]=std::clamp(std::max(avail/9,typeW),44,94);
            const int budget=std::clamp(avail/6,48,92);
            widths[1]=std::max(budget,columnWidth(L"原值",18));
            widths[2]=std::max(budget,columnWidth(L"待保存",18));
            widths[0]=avail-widths[1]-widths[2]-widths[3];
        }
        ReleaseDC(g.grid,dc);
        for (int i=0;i<4;++i) ListView_SetColumnWidth(g.grid,i,px(std::max(0,widths[i])));
        if (auto header=ListView_GetHeader(g.grid)) InvalidateRect(header,nullptr,FALSE);
    }
    InvalidateRect(g.win,nullptr,FALSE);
}

void resourceSymbol(HDC dc,int kind,bool base,int x,int y,COLORREF ink) {
    auto pen=CreatePen(PS_SOLID,std::max(1,px(1)),ink);
    auto oldPen=SelectObject(dc,pen),oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
    auto line=[&](std::initializer_list<POINT> points) {
        bool first=true;
        for(auto p:points) {
            if(first)MoveToEx(dc,px(x+p.x),px(y+p.y),nullptr);
            else LineTo(dc,px(x+p.x),px(y+p.y));
            first=false;
        }
    };
    if(kind==0) {
        Ellipse(dc,px(x+1),px(y+2),px(x+15),px(y+8));
        line({{1,5},{1,10},{3,12},{7,13},{11,13},{14,11},{14,5}});
        line({{1,8},{4,10},{9,11},{13,9}});
    } else if(kind==1 && base) {
        RoundRect(dc,px(x+1),px(y+3),px(x+16),px(y+14),px(4),px(4));
        line({{11,7},{15,7},{15,10},{11,10},{11,7}});
    } else if(kind==1) {
        line({{1,4},{5,7},{8,2},{11,7},{15,4},{13,12},{3,12},{1,4}});
        line({{3,15},{13,15}});
    } else if(base) {
        line({{1,14},{1,3}});line({{1,14},{15,14}});
        line({{3,10},{7,7},{9,9},{15,3}});
    } else {
        line({{9,1},{2,9},{7,9},{6,15},{14,6},{9,6},{9,1}});
    }
    SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);
}

void paintMetrics(HDC dc,const WorkspaceGeometry& a) {
    const char* keys[]={"BaseGame.GovernmentBudget","BaseGame.PersonalWealth","BaseGame.Economy"};
    const wchar_t* labels[]={L"预算",L"个人财富",L"经济水平"};
    const COLORREF inks[]={Theme::accentInk,Theme::green,Theme::blue};
    const bool base=isBase(), tall=a.metricHeight>=70;
    const bool sharedSurface=Theme::index()==Theme::fjordIndex;
    const int gap=sharedSurface?0:(a.content>=780?10:6);
    const int unit=(a.content-2*gap)/3;
    rizia::ChangeMap linkedCaps;
    if(!base) {
        try { linkedCaps=linkedCapChanges(); }
        catch(const std::exception&) { /* Keep conflicting explicit values visible until Review. */ }
    }
    if(sharedSurface)
        fill(dc,rect(a.left,a.headerBottom,a.content,a.metricHeight),Theme::panel);
    for(int i=0;i<3;++i) {
        int x=a.left+i*(unit+gap), y=a.headerBottom;
        int width=i==2?a.left+a.content-x:unit;
        int pad=tall?14:9, inner=width-2*pad;
        const auto& resource=riziaResources[i];
        const char* key=base?keys[i]:resource.stock;
        const auto balance=val(key);
        if(sharedSurface) {
            if(i>0) {
                const int inset=tall?18:11;
                fill(dc,rect(x,y+inset,1,a.metricHeight-2*inset),Theme::line);
            }
        } else {
            rounded(dc,rect(x,y,width,a.metricHeight),Theme::panel,Theme::line,21);
        }

        bool showSymbol=!sharedSurface && (tall || width>=220);
        int labelX=x+pad+(showSymbol?23:0);
        int labelY=y+(tall?11:base?18:3);
        if(showSymbol)resourceSymbol(dc,i,base,x+pad,labelY+2,inks[i]);
        int available=inner-(showSymbol?23:0);
        int labelW=tall?available:available*3/5;
          HFONT numberFont=tall?g.large:g.compactNumber;
          auto old=SelectObject(dc,numberFont);SIZE extent{};
          GetTextExtentPoint32W(dc,balance.c_str(),static_cast<int>(balance.size()),&extent);
          SelectObject(dc,old);
          if(sharedSurface && !tall)
              labelW=std::max(labelW,available-MulDiv(extent.cx,96,g.dpi)-8);
        drawText(dc,base?labels[i]:resource.balanceLabel,rect(labelX,labelY,labelW,20),g.small,Theme::muted);

        // Reserve a second column for income and cap only on the tall DLC card.
        int numberX=tall?x+pad:labelX+labelW+3;
        int numberW=tall?(base?inner:inner/2):x+width-pad-numberX;
        if(extent.cx>px(numberW))numberFont=tall?g.compactNumber:g.tableNumber;
        drawText(dc,balance,rect(numberX,tall?y+33:base?y+15:y+1,numberW,tall?45:26),numberFont,
                 pending().count(key)?Theme::accentInk:Theme::text,
                 (tall?DT_LEFT:DT_RIGHT)|DT_VCENTER|DT_SINGLELINE);
        if(base)continue;

        const auto linked=linkedCaps.find(resource.cap);
        const auto capText=tr(L"上限 ")+(linked==linkedCaps.end()?val(resource.cap):wide(linked->second));
        const auto turnText=tr(L"每回合")+L" "+turnDelta(val(resource.turnTotal));
        // In short windows, keep both secondary values on a single baseline.
        RECT turnBox=tall?rect(x+pad+numberW,y+33,inner-numberW,20):rect(x+pad,y+30,inner*55/100,21);
        RECT capBox=tall?rect(x+pad+numberW,y+55,inner-numberW,20):rect(x+pad+inner*55/100,y+30,inner-inner*55/100,21);
        drawText(dc,turnText,turnBox,g.small,pending().count(resource.turnTotal)?Theme::accentInk:Theme::muted,
                 (tall?DT_RIGHT:DT_LEFT)|DT_VCENTER|DT_SINGLELINE);
        drawText(dc,capText,capBox,g.small,linked!=linkedCaps.end()||pending().count(resource.cap)?Theme::accentInk:Theme::muted,
                 DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
    }
}

void paintInputFrame(HDC dc,RECT box,HWND input,int radius) {
    const bool focused=GetFocus()==input;
    rounded(dc,box,focused?Theme::focus:Theme::input,focused?Theme::focus:Theme::line,radius);
    if(focused) {
        InflateRect(&box,-px(2),-px(2));
        rounded(dc,box,Theme::input,Theme::input,radius-2);
    }
}

void paint() {
    PAINTSTRUCT ps{}; auto dc=BeginPaint(g.win,&ps);
    RECT client{};GetClientRect(g.win,&client);FillRect(dc,&client,g.bg);
    auto a=workspaceGeometry(); fill(dc,rect(0,0,a.rail,a.height),Theme::sidebar);
    int logoX=a.compactRail?13:16,logoY=a.shortScreen?13:20;
    fill(dc,rect(a.rail-1,0,1,a.height),Theme::sideLine);
    const bool fjord=Theme::index()==Theme::fjordIndex;
    const auto logoFill=fjord?Theme::select:Theme::accent;
    rounded(dc,rect(logoX,logoY,33,33),logoFill,logoFill,12);
    navSymbol(dc,7,px(logoX+7),px(logoY+7),fjord?Theme::accentInk:Theme::onAccent);
    if (!a.compactRail) {
        drawText(dc,L"Suzerain",rect(59,logoY-7,a.rail-64,28),g.brand,Theme::sideText);
        drawText(dc,L"Save Studio",rect(60,logoY+17,a.rail-64,19),g.small,Theme::sideMuted);
        if (!a.shortScreen) {
            fill(dc,rect(16,84,a.rail-32,1),Theme::sideLine);
            drawText(dc,g.doc?campaignLabel(activeStory):tr(L"战役"),rect(19,101,a.rail-32,26),g.font,Theme::sideMuted);
            drawText(dc,L"工作区",rect(19,navY(a,5)-27,a.rail-32,22),g.small,Theme::sideMuted);
            drawText(dc,std::wstring(L"v")+APP_VERSION,rect(16,a.height-37,a.rail-32,22),g.small,Theme::sideMuted);
        } else {
            drawText(dc,g.doc?campaignLabel(activeStory):tr(L"战役"),rect(17,55,a.rail-30,23),g.small,Theme::sideMuted);
        }
    }
    if (a.shortScreen || a.compactRail)
        fill(dc,rect(11,navY(a,5)-8,a.rail-22,1),Theme::sideLine);
    int titleWidth=a.content-std::min(465,a.content*62/100)-8;
    drawText(dc,pageTitle(g.tab),rect(a.left,a.shortScreen?5:10,titleWidth,37),g.title,Theme::text);
    if(a.metricHeight) paintMetrics(dc,a);
    if(g.tab<7) {
        int listY=g.tab==5?a.toolsY:a.listY;
        rounded(dc,rect(a.left,listY,a.list,a.bottom-listY),Theme::panel,Theme::line,9);
        RECT inspector=rect(a.right,a.toolsY,a.inspector,a.bottom-a.toolsY);
        RECT notes{};GetWindowRect(g.desc,&notes);MapWindowPoints(nullptr,g.win,(POINT*)&notes,2);
        if(g.tab<5)inspector.bottom=std::min<LONG>(inspector.bottom,notes.bottom+px(a.shortScreen?10:16));
        rounded(dc,inspector,Theme::panel,Theme::line,19);
        if(!a.shortScreen){
            RECT divider{notes.left,notes.top-px(7),notes.right,notes.top-px(7)+1};
            fill(dc,divider,Theme::line);
        }
        if(g.tab<5) {
            paintInputFrame(dc,rect(a.left,a.toolsY,searchWidth(a),32),g.search,10);
            auto pen=CreatePen(PS_SOLID,std::max(1,px(2)),Theme::dim);auto old=SelectObject(dc,pen);auto ob=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
            Ellipse(dc,px(a.left+9),px(a.toolsY+9),px(a.left+19),px(a.toolsY+19));
            MoveToEx(dc,px(a.left+17),px(a.toolsY+17),nullptr);LineTo(dc,px(a.left+22),px(a.toolsY+22));
            SelectObject(dc,ob);SelectObject(dc,old);DeleteObject(pen);
            if(IsWindowVisible(g.edit)) {
                RECT box{};GetWindowRect(g.edit,&box);MapWindowPoints(nullptr,g.win,(POINT*)&box,2);InflateRect(&box,px(8),px(4));
                paintInputFrame(dc,box,g.edit,10);
            }
        }
    } else rounded(dc,rect(a.left,a.listY,a.content,a.bottom-a.listY),Theme::panel,Theme::line,8);
    fill(dc,rect(a.left,a.footer,a.content,1),Theme::line);
    rounded(dc,rect(a.left+1,a.footer+19,7,7),g.editingDirty||!pending().empty()?Theme::goldDark:Theme::green,
            g.editingDirty||!pending().empty()?Theme::goldDark:Theme::green,7);
    EndPaint(g.win,&ps);
}
