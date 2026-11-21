#pragma once

int navStart(const WorkspaceGeometry& a) { return a.compactRail ? 65 : (a.shortScreen ? 87 : 138); }
int navStep(const WorkspaceGeometry& a) { return a.shortScreen ? 33 : 43; }
int navGap(const WorkspaceGeometry& a) { return a.shortScreen || a.compactRail ? 12 : 30; }
int navY(const WorkspaceGeometry& a, int index) {
    return navStart(a) + index * navStep(a) + (index >= 5 ? navGap(a) : 0);
}
bool compactFieldTools(const WorkspaceGeometry& a) { return a.list < (g.tab==4 ? 560 : 450); }
int searchWidth(const WorkspaceGeometry& a) {
    bool compact=compactFieldTools(a);
    return a.list - (compact ? 92 : 132) - 6 - (g.tab==4 ? (compact ? 153 : 213) : 0);
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
    int themeWidth=a.compactRail?116:145;
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
    place(g.search, a.left+29, a.toolsY+6, sw-37, 22);
    bool compactTools=compactFieldTools(a);
    place(ctl(FILTER_CHANGED), a.left+sw+5, a.toolsY, compactTools ? 92 : 132, 32);
    int favoritesX=a.left+sw+10+(compactTools?92:132);
    place(ctl(FILTER_FAVORITES),favoritesX,a.toolsY,compactTools?70:100,32);
    place(ctl(PATCH_IMPORT), a.right-a.gap-(compactTools?74:104), a.toolsY, compactTools?74:104, 32);
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
    int nameH = edit ? (a.shortScreen ? 25 : 34) : (a.shortScreen ? 32 : 43);
    place(g.name,rx,y,rw,nameH);
    y += nameH + 3;
    int favoriteW=std::clamp(rw/2,70,100);
    place(g.oldValue,rx,y,edit?rw-favoriteW-5:rw,20);
    if(edit)place(ctl(FIELD_FAVORITE),rx+rw-favoriteW,y-3,favoriteW,25);
    if (edit) {
        y += a.shortScreen ? 27 : 34;
        place(g.valueLabel,rx,y,rw,18);
        y += 20;
        place(g.edit,rx+8,y+4,rw-16,27);
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
        if (backup) {
            widths[0]=std::min(156,avail*40/100); widths[1]=std::min(72,avail/5); widths[2]=avail-widths[0]-widths[1];
        } else if (review) {
            widths[1]=widths[2]=std::clamp(avail/5,51,108); widths[0]=avail-widths[1]-widths[2];
        } else {
            auto dc=GetDC(g.grid);auto old=SelectObject(dc,g.small);
            int typeW=0;
            for(const wchar_t* caption : {L"类型",L"数值",L"开关"}) {
                auto s=tr(caption);SIZE size{};GetTextExtentPoint32W(dc,s.c_str(),(int)s.size(),&size);
                typeW=std::max(typeW,MulDiv(size.cx,96,g.dpi)+16);
            }
            SelectObject(dc,old);ReleaseDC(g.grid,dc);
            widths[3]=std::clamp(std::max(avail/9,typeW),44,94); widths[1]=widths[2]=std::clamp(avail/6,48,92);
            widths[0]=avail-widths[1]-widths[2]-widths[3];
        }
        for (int i=0;i<4;++i) ListView_SetColumnWidth(g.grid,i,px(std::max(0,widths[i])));
        if (auto header=ListView_GetHeader(g.grid)) InvalidateRect(header,nullptr,FALSE);
    }
    InvalidateRect(g.win,nullptr,FALSE);
}

void paintRiziaMetric(HDC dc, const RiziaResource& resource, const rizia::ChangeMap& linkedCaps,
                     int x, int y, int width, int height) {
    bool tall = height >= 70;
    int pad = tall ? 15 : 10;
    int right = x + width - pad;
    int inner = width - 2 * pad;
    auto balance = val(resource.stock);
    auto numberFont = tall ? g.large : g.bold;
    auto old = SelectObject(dc, numberFont);
    SIZE valueSize{};
    GetTextExtentPoint32W(dc, balance.c_str(), static_cast<int>(balance.size()), &valueSize);
    SelectObject(dc, old);
    int valueW = std::min(inner / 2, std::max(28, MulDiv(valueSize.cx, 96, g.dpi) + 3));
    int rowY = y + (tall ? 7 : 2);
    int rowH = tall ? 38 : 28;
    drawText(dc, resource.balanceLabel, rect(x+pad,rowY,inner-valueW-4,rowH),
             tall ? g.font : g.small, Theme::muted);
    drawText(dc, balance, rect(right-valueW,rowY,valueW,rowH), numberFont,
             pending().count(resource.stock) ? Theme::accentInk : Theme::text, DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

    const auto linked = linkedCaps.find(resource.cap);
    const auto cap = linked == linkedCaps.end() ? val(resource.cap) : wide(linked->second);
    const auto capText = tr(L"上限 ") + cap;
    const auto turnText = tr(L"每回合") + L" " + turnDelta(val(resource.turnTotal));
    auto oldSmall = SelectObject(dc, g.small);
    SIZE capSize{};
    GetTextExtentPoint32W(dc, capText.c_str(), static_cast<int>(capText.size()), &capSize);
    SelectObject(dc, oldSmall);
    int capW = std::min(inner / 2, MulDiv(capSize.cx, 96, g.dpi) + 3);
    int bottomRow = y + height - (tall ? 27 : 22);
    drawText(dc, turnText, rect(x+pad,bottomRow,inner-capW-5,19), g.small,
             pending().count(resource.turnTotal) ? Theme::accentInk : Theme::muted);
    drawText(dc, capText, rect(right-capW,bottomRow,capW,19), g.small,
             linked != linkedCaps.end() || pending().count(resource.cap) ? Theme::accentInk : Theme::muted,
             DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
}

void paintMetrics(HDC dc,const WorkspaceGeometry& a) {
    const char* mainKeys[]={"BaseGame.GovernmentBudget","BaseGame.PersonalWealth","BaseGame.Economy"};
    const wchar_t* labels[]={L"预算",L"个人财富",L"经济水平"};
    rounded(dc,rect(a.left,a.headerBottom,a.content,a.metricHeight),Theme::head,Theme::line,14);
    int cw=a.content/3;
    rizia::ChangeMap linkedCaps;
    if (!isBase()) {
        try { linkedCaps = linkedCapChanges(); }
        catch (const std::exception&) { /* Conflicting edits are shown as entered until corrected in Review. */ }
    }
    for(int i=0;i<3;++i) {
        int x=a.left+i*cw, y=a.headerBottom, width=i==2 ? a.left+a.content-x : cw;
        if(i>0)fill(dc,rect(x,y+12,1,a.metricHeight-24),Theme::line);
        if (!isBase()) {
            paintRiziaMetric(dc, riziaResources[i], linkedCaps, x, y, width, a.metricHeight);
            continue;
        }
        const auto key = mainKeys[i];
        auto value=effective(key); bool tall=a.metricHeight>=70;
        int valueWidth=std::max(48,width/2-9);
        if (tall) {
            drawText(dc,labels[i],rect(x+17,y+7,width-32,19),g.small,Theme::muted);
            drawText(dc,value.empty()?L"—":wide(value),rect(x+16,y+28,width-32,39),g.large,pending().count(key)?Theme::accentInk:Theme::text);
        } else {
            drawText(dc,labels[i],rect(x+12,y+5,width-valueWidth-20,a.metricHeight-10),g.font,Theme::muted);
            drawText(dc,value.empty()?L"—":wide(value),rect(x+width-valueWidth-9,y+4,valueWidth,a.metricHeight-8),g.large,pending().count(key)?Theme::goldDark:Theme::text,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
        }
    }
}

void paint() {
    PAINTSTRUCT ps{}; auto dc=BeginPaint(g.win,&ps);
    RECT client{};GetClientRect(g.win,&client);FillRect(dc,&client,g.bg);
    auto a=workspaceGeometry(); fill(dc,rect(0,0,a.rail,a.height),Theme::sidebar);
    int logoX=a.compactRail?13:16,logoY=a.shortScreen?13:20;
    fill(dc,rect(a.rail-1,0,1,a.height),Theme::sideLine);
    rounded(dc,rect(logoX,logoY,33,33),Theme::accent,Theme::accent,12);
    navSymbol(dc,7,px(logoX+7),px(logoY+7),Theme::onAccent);
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
        rounded(dc,rect(a.right,a.toolsY,a.inspector,a.bottom-a.toolsY),Theme::panel,Theme::line,14);
        if(!a.shortScreen){
            RECT notes{};GetWindowRect(g.desc,&notes);MapWindowPoints(nullptr,g.win,(POINT*)&notes,2);
            RECT divider{notes.left,notes.top-px(7),notes.right,notes.top-px(7)+1};
            fill(dc,divider,Theme::line);
        }
        if(g.tab<5) {
            rounded(dc,rect(a.left,a.toolsY,searchWidth(a),32),Theme::input,GetFocus()==g.search?Theme::accent:Theme::line,8);
            auto pen=CreatePen(PS_SOLID,std::max(1,px(2)),Theme::dim);auto old=SelectObject(dc,pen);auto ob=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
            Ellipse(dc,px(a.left+9),px(a.toolsY+9),px(a.left+19),px(a.toolsY+19));
            MoveToEx(dc,px(a.left+17),px(a.toolsY+17),nullptr);LineTo(dc,px(a.left+22),px(a.toolsY+22));
            SelectObject(dc,ob);SelectObject(dc,old);DeleteObject(pen);
            if(IsWindowVisible(g.edit)) {
                RECT box{};GetWindowRect(g.edit,&box);MapWindowPoints(nullptr,g.win,(POINT*)&box,2);InflateRect(&box,px(8),px(4));
                rounded(dc,box,Theme::input,GetFocus()==g.edit?Theme::focus:Theme::line,9);
            }
        }
    } else rounded(dc,rect(a.left,a.listY,a.content,a.bottom-a.listY),Theme::panel,Theme::line,8);
    fill(dc,rect(a.left,a.footer,a.content,1),Theme::line);
    rounded(dc,rect(a.left+1,a.footer+19,7,7),g.editingDirty||!pending().empty()?Theme::goldDark:Theme::green,
            g.editingDirty||!pending().empty()?Theme::goldDark:Theme::green,7);
    EndPaint(g.win,&ps);
}
