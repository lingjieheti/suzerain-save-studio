// Exercise the real list, edit control, preview and history in a hidden window.
// Both inputs are read-only synthetic demos. Appearance tests use an isolated registry key.
#define wWinMain UnusedEditorGuiEntry
#include "../src/main.cpp"
#undef wWinMain
#include <iostream>

namespace {
int checks=0;
void check(bool ok,const char* description){
    ++checks;
    if(!ok)throw std::runtime_error(description);
}

void selectRow(const std::string& key){
    const auto row=std::find(g.rows.begin(),g.rows.end(),key);
    check(row!=g.rows.end(),"Requested row is visible");
    const int index=(int)(row-g.rows.begin());
    ListView_SetItemState(g.grid,index,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    check(g.selected==key,"Native row selection reaches the editor");
}

void testSaveQueries() {
    rizia::discovery::Save snapshot;
    snapshot.story="StoryPack_Rizia";snapshot.turn="3";snapshot.campaignName="North Palace";
    snapshot.path=L"Autosave_13-10-2025.json";
    auto matches=[&](const wchar_t* input){
        return save_picker::matchesSave(snapshot,save_picker::parseSaveQuery(input),save_picker::saveSlot(snapshot.path.filename().wstring()));
    };
    check(matches(L"pALAcE　Rizia north") && matches(L"13-10-2025"),"Combine save metadata words and preserve a dated filename");
    check(matches(L"里奇亚 自动 回合 3") && matches(L"Rizia turn3") && matches(L"3"),"Find an autosave at the requested turn");
    check(matches(L"第3回合") && matches(L"里奇亚 第 3 回合") && !matches(L"第13回合"),"Recognize Chinese ordinal turn phrases");
    check(!matches(L"Sordland turn 3") && matches(L"13") && !matches(L"turn 13"),"Only explicit turn queries ignore filename digits");
    snapshot.turn="13";snapshot.path=L"ManualSave_03-10-2025.json";
    check(matches(L"手动 回合13") && !matches(L"自动") && !matches(L"turn 3") && !matches(L"3"),"Manual save and exact turn remain distinct");
    const int language=i18n::language;
    for(int i=0;i<11;++i) {
        i18n::setLanguage(i);
        auto localized=tr(L"手动存档")+L" "+tr(L"回合")+L" 13";
        check(matches(localized.c_str()),"Localized turn and manual save type match in each interface language");
    }
    i18n::setLanguage(language);
    snapshot.path=L"My-Autosave.json";
    check(!matches(L"自动"),"A renamed file is not classified as a game autosave");
    snapshot.path=L"TurnSave_20241121.json";
    check(matches(L"回合存档 2024") && matches(L"20241121") && !matches(L"3"),"Compact filename dates and turn slot aliases remain searchable");
    snapshot.path=L"Active_20241121.json";
    check(matches(L"当前") && !matches(L"自动") && !matches(L"回合存档"),"Active slots remain distinct from auto and turn saves");
    for(const auto* filename:{L"AUTOSAVE_2024.json",L"ManualSave.json",L"TurnSave_01.json",L"FinalSave_01.json",L"Active_01.json"}){
        const auto* kind=save_picker::saveSlot(filename);
        check(kind && save_picker::saveSubtitle(kind,L"") == tr(kind->label) &&
              save_picker::saveSubtitle(kind,L"Palace") == tr(kind->label)+L" · Palace","Known save slots retain type and optional campaign name");
    }
    for(const auto* custom:{L"My-Autosave.json",L"Auto-save_01.json",L"Autosave.custom.json"}) {
        const auto* kind=save_picker::saveSlot(custom);
        check(!kind && save_picker::saveSubtitle(kind,L"Palace")==L"Palace" &&
              save_picker::saveSubtitle(kind,L"").empty(),"Custom save filenames preserve their original subtitle");
    }
}

void testSearchControls(const fs::path& sample) {
    loadDocument(sample,false);
    navigate(4);
    auto shown=[](HWND control){return (GetWindowLongPtrW(control,GWL_STYLE)&WS_VISIBLE)!=0;};
    auto clear=ctl(CLEAR_FIELD_SEARCH);
    check(!shown(clear),"An empty search leaves the full input usable");
    const auto allFields=g.rows.size();
    SetWindowTextW(g.search,L"GovernmentBudget");
    check(shown(clear) && !g.rows.empty() && g.rows.size()<allFields,"Typing exposes the clear action and filters the grid");
    RECT input{},button{};
    GetWindowRect(g.search,&input);GetWindowRect(clear,&button);
    check(input.right<=button.left && button.right-button.left>=px(24),"Clear button has a separate pointer target");
    SendMessageW(clear,BM_CLICK,0,0);
    MSG click{};
    check(PeekMessageW(&click,g.win,WM_APP+1,WM_APP+1,PM_REMOVE)!=0,"Native clear button posts its action");
    DispatchMessageW(&click);
    check(!shown(clear) && textOf(g.search).empty() && g.rows.size()==allFields,"Clear button resets the query and restores all fields");
    SetWindowTextW(g.search,L"PersonalWealth");
    MSG key{};key.hwnd=g.search;key.message=WM_KEYDOWN;key.wParam=VK_ESCAPE;
    check((SendMessageW(g.search,WM_GETDLGCODE,key.wParam,(LPARAM)&key)&DLGC_WANTMESSAGE)!=0,"A filled search handles Escape before dialog navigation");
    SendMessageW(g.search,WM_KEYDOWN,VK_ESCAPE,0);
    check(!shown(clear) && textOf(g.search).empty(),"Escape clears a search without leaving the editor");
    navigate(0);
}

void testInspectorLayout(const fs::path& sample){
    loadDocument(sample,false);
    navigate(4);
    auto bounds=[](HWND child){
        RECT r{};GetWindowRect(child,&r);
        MapWindowPoints(nullptr,g.win,reinterpret_cast<POINT*>(&r),2);
        return r;
    };
    auto settle=[](){
        MSG msg{};int dispatched=0;
        while(dispatched<16 && PeekMessageW(&msg,g.desc,WM_APP,0xBFFF,PM_REMOVE)){
            DispatchMessageW(&msg);++dispatched;
        }
        check(dispatched<16,"Inspector text wrapping settles after resize");
    };
    SetWindowPos(g.win,nullptr,0,0,px(1160),px(770),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    SetWindowTextW(g.desc,L"One line of context.");
    settle();
    RECT shortNote=bounds(g.desc);
    check(shortNote.bottom-shortNote.top<px(60) && (GetWindowLongPtrW(g.desc,GWL_STYLE)&WS_VSCROLL)==0,
          "Brief field descriptions use a compact inspector");
    SetWindowPos(g.win,nullptr,0,0,px(680),px(485),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    std::wstring longNote;
    for(int i=0;i<40;++i)longNote+=L"Field context and the raw variable name.\r\n";
    SetWindowTextW(g.desc,longNote.c_str());
    settle();
    const auto area=workspaceGeometry();
    const RECT longBounds=bounds(g.desc),apply=bounds(ctl(APPLY)),search=bounds(g.search);
    check(longBounds.bottom<=px(area.bottom) && longBounds.bottom-longBounds.top>=px(12) &&
          (GetWindowLongPtrW(g.desc,GWL_STYLE)&WS_VSCROLL)!=0,"Long notes scroll above the compact footer");
    check(search.right>search.left && apply.bottom<=longBounds.top && apply.right<=px(area.width),
          "Compact search and inspector actions remain visible above long notes");
    SetWindowTextW(g.desc,L"Short.");
    settle();
    check((GetWindowLongPtrW(g.desc,GWL_STYLE)&WS_VSCROLL)==0 &&
          bounds(g.desc).bottom< longBounds.bottom,"Shortening a description releases the scroll area");
    int previousLanguage=i18n::language;
    for(int language=0;language<11;++language) {
        i18n::setLanguage(language);fonts();layout();
        auto dc=GetDC(g.win);auto old=SelectObject(dc,g.font);
        for(const auto& item: {std::pair<int,const wchar_t*>{FILTER_CHANGED,L"只看待保存"},
                                {FILTER_FAVORITES,L"只看收藏"},{PATCH_IMPORT,L"导入方案"}}) {
            auto caption=tr(item.second);SIZE size{};
            GetTextExtentPoint32W(dc,caption.c_str(),static_cast<int>(caption.size()),&size);
            RECT button=bounds(ctl(item.first));
            check(button.right-button.left>=size.cx+px(item.first==FILTER_CHANGED?25:6) &&
                  button.top>=bounds(g.search).bottom && button.bottom<=bounds(g.grid).top,
                  "Compact advanced filter fits its localized label below the search field");
        }
        SelectObject(dc,old);ReleaseDC(g.win,dc);
    }
    i18n::setLanguage(previousLanguage);fonts();layout();
}

void CALLBACK dismissValidationWarning(HWND,UINT,UINT_PTR,DWORD){
    EnumThreadWindows(GetCurrentThreadId(),[](HWND popup,LPARAM)->BOOL{
        wchar_t kind[16]{};
        GetClassNameW(popup,kind,16);
        if(std::wstring(kind)==L"#32770")EndDialog(popup,IDOK);
        return TRUE;
    },0);
}

// Keep the application code on its normal registry path while isolating every test run.
class AppearanceTestRegistry {
    std::wstring path_ = L"Software\\SuzerainSaveStudio-Tests-" + writeId();
    HKEY root_ = nullptr;
public:
    AppearanceTestRegistry(){
        if(RegCreateKeyExW(HKEY_CURRENT_USER,path_.c_str(),0,nullptr,REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,nullptr,&root_,nullptr)!=ERROR_SUCCESS)
            throw std::runtime_error("Create temporary appearance test key");
        if(RegOverridePredefKey(HKEY_CURRENT_USER,root_)!=ERROR_SUCCESS){
            RegCloseKey(root_);
            RegDeleteKeyW(HKEY_CURRENT_USER,path_.c_str());
            throw std::runtime_error("Isolate appearance preferences");
        }
    }
    ~AppearanceTestRegistry(){
        RegOverridePredefKey(HKEY_CURRENT_USER,nullptr);
        RegCloseKey(root_);
        RegDeleteTreeW(HKEY_CURRENT_USER,path_.c_str());
    }
    AppearanceTestRegistry(const AppearanceTestRegistry&)=delete;
    AppearanceTestRegistry& operator=(const AppearanceTestRegistry&)=delete;
};

double textContrast(COLORREF ink, COLORREF surface){
    auto linear = [](BYTE channel){
        const double value = channel / 255.0;
        return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    auto luminance = [&](COLORREF color){
        return 0.2126 * linear(GetRValue(color)) + 0.7152 * linear(GetGValue(color)) +
               0.0722 * linear(GetBValue(color));
    };
    const double a = luminance(ink), b = luminance(surface);
    return (std::max(a,b) + 0.05) / (std::min(a,b) + 0.05);
}

void testAppearance(const fs::path& path){
    AppearanceTestRegistry registry;
    Theme::load();
    check(Theme::index()==2,"A new user starts with Fjord");
    const int languageBefore=i18n::language;
    for(int lang=0;lang<static_cast<int>(std::size(i18n::names));++lang){
        i18n::setLanguage(lang);
        std::set<std::wstring> names;
        for(int index=0;index<Theme::count;++index)names.insert(Theme::name(index));
        check(*Theme::label() && names.size()==Theme::count && !names.count(L""),
              "Each supported language has four distinct named appearances");
    }
    i18n::setLanguage(languageBefore);
    loadDocumentBytes(readFile(path),path,0);
    SetWindowTextW(g.search,L"GovernmentBudget");
    const std::string budget="BaseGame.GovernmentBudget";
    selectRow(budget);
    SetWindowTextW(g.edit,L"43");
    check(applyInput(),"Stage a budget edit before changing the appearance");
    SetWindowTextW(g.edit,L"unfinished");
    auto* session=g.session.get();
    for(int index:{1,2,3,0}){
        applyAppearance(index);
        check(Theme::index()==index,"The selected appearance applies immediately");
        check(textContrast(Theme::muted,Theme::head)>=4.5,"Column headings stay readable");
        check(textContrast(Theme::accentInk,Theme::select)>=4.5,"Selected rows stay readable");
        check(textContrast(Theme::sideText,Theme::sideHover)>=4.5,"Hovered sidebar labels stay readable");
        check(textContrast(Theme::onAccent,Theme::accent)>=4.5 &&
              textContrast(Theme::onAccent,Theme::accentHover)>=4.5,"Primary button text stays readable");
        check(g.session.get()==session && pending().at(budget)=="43" && g.session->canUndo(),
              "Switching appearance preserves edits and undo history");
        check(g.selected==budget && textOf(g.search)==L"GovernmentBudget" &&
              textOf(g.edit)==L"unfinished" && g.editingDirty,
              "Switching appearance preserves the search, selection and uncommitted draft");
        DWORD saved=99,size=sizeof(saved);
        check(RegGetValueW(HKEY_CURRENT_USER,L"Software\\SuzerainSaveStudio",L"Appearance",
                           RRF_RT_REG_DWORD,nullptr,&saved,&size)==ERROR_SUCCESS && saved==DWORD(index),
              "The desktop and game panel share the appearance preference");
        Theme::use((index+1)%Theme::count);
        Theme::load();
        check(Theme::index()==index,"Reopening uses the saved appearance");
    }
    const DWORD invalid=99;
    RegSetKeyValueW(HKEY_CURRENT_USER,L"Software\\SuzerainSaveStudio",L"Appearance",REG_DWORD,&invalid,sizeof(invalid));
    Theme::load();
    check(Theme::index()==2,"An unsupported saved theme falls back to Fjord");
    const wchar_t wrongType[]=L"3";
    RegSetKeyValueW(HKEY_CURRENT_USER,L"Software\\SuzerainSaveStudio",L"Appearance",
                    REG_SZ,wrongType,sizeof(wrongType));
    Theme::load();
    check(Theme::index()==2,"An invalid preference type falls back to Fjord");
}

void testFavorites(const fs::path& mainPath,const fs::path& riziaPath){
    AppearanceTestRegistry registry;
    const std::string budget="BaseGame.GovernmentBudget", event="BaseGame.BranchChoice";
    const std::string newer=R"({"currentStoryPack":"StoryPack_Main","variables":"Variable={[\"BaseGame.GovernmentBudget\"]=7,[\"BaseGame.BranchChoice\"]=true}"})";
    loadDocumentBytes(newer,{},0);
    navigate(0);
    selectRow(budget);
    SetWindowTextW(g.edit,L"12");
    command(FIELD_FAVORITE,0);
    check(textOf(g.edit)==L"12"&&g.editingDirty&&pending().empty(),"Favoriting preserves a live unsaved draft");
    command(UNDOONE,0);
    navigate(4);
    command(FILTER_FAVORITES,0);
    check(g.rows==std::vector<std::string>{budget}&&textOf(ctl(FIELD_FAVORITE))==tr(L"取消收藏"),
          "A curated favorite appears in advanced fields with its current state");
    command(FILTER_FAVORITES,0);
    selectRow(event);command(FIELD_FAVORITE,0);
    check(pending().empty()&&!g.session->canUndo(),"An advanced boolean favorite is separate from edit history");
    loadDocumentBytes(readFile(mainPath),mainPath,0);
    command(FILTER_FAVORITES,0);
    check(g.rows==std::vector<std::string>{budget}&&g.favorites.count(event),"Older save hides missing fields without forgetting them");
    SetWindowTextW(g.search,L"no match");
    check(g.rows.empty(),"Favorite filter intersects with field search");
    SetWindowTextW(g.search,L"");
    command(FIELD_FAVORITE,0);
    check(g.rows.empty()&&!IsWindowEnabled(ctl(FIELD_FAVORITE)),"Removing the last visible favorite updates selection and edit controls");
    loadDocumentBytes(readFile(riziaPath),riziaPath,0);
    command(FILTER_FAVORITES,0);
    check(g.rows.empty()&&g.favorites.empty(),"Another campaign starts with its own favorites");
    loadDocumentBytes(newer,{},0);
    command(FILTER_FAVORITES,0);
    check(g.rows==std::vector<std::string>{event},"Reopened campaign recovers the advanced-only favorite");
    command(FIELD_FAVORITE,0);
    check(g.rows.empty()&&pending().empty()&&!g.session->canUndo(),"Advanced-only favorite can be removed without changing the save");
}

void testSearch(const fs::path& path){
    loadDocumentBytes(readFile(path),path,0);
    const std::string budget="BaseGame.GovernmentBudget";
    selectRow(budget);
    SetWindowTextW(g.edit,L"43");
    SetWindowTextW(g.search,L"government 预算");
    check(pending().at(budget)=="43","Filtering safely stages an active numeric edit");
    check(g.selected==budget,"Matching selected field remains selected");
    SetWindowTextW(g.search,L"MAX　预算");
    check(g.rows.size()==1&&g.selected=="BaseGame.Sordland_HUDStat_GovernmentBudget_Max",
          "Reordered raw-key and Chinese tokens match on a named page");
    const auto economySelection=g.selected;
    navigate(1);
    check(textOf(g.search).empty()&&!g.rows.empty(),"Different pages have independent filters");
    navigate(0);
    check(textOf(g.search)==L"MAX　预算"&&g.selected==economySelection,"Returning restores the query and row");
    SetWindowTextW(g.search,L"zzzzzz");
    check(g.rows.empty()&&g.selected.empty()&&!IsWindowEnabled(g.edit),"Empty search results disable editing");
    SetWindowTextW(g.search,L"");
    check(g.selected==budget,"Clearing a filter restores the preferred selection");
    SetWindowTextW(g.edit,L"invalid");
    const auto timer=SetTimer(nullptr,0,50,dismissValidationWarning);
    check(timer!=0,"Install validation-dialog timer for invalid input test");
    SetWindowTextW(g.search,L"trade");
    KillTimer(nullptr,timer);
    check(g.selected==budget&&textOf(g.edit)==L"invalid"&&g.editingDirty,"Rejected draft remains in the active editor");
    check(textOf(g.search).empty()&&g.pages[0].query.empty()&&pending().at(budget)=="43",
          "Rejected draft restores both the text and results of the previous query");
    SetWindowTextW(g.edit,L"44");

    navigate(4);
    SetWindowTextW(g.search,L"budget 预算 MAX");
    check(g.rows.size()==1&&g.selected==economySelection,"Advanced search accepts mixed unordered tokens");
    command(FILTER_CHANGED,0);
    check(g.rows.empty(),"Pending-only and text filters intersect");
    navigate(0);
    check(!g.changedOnly&&!g.rows.empty(),"Pending-only filtering is scoped to its page");
    navigate(4);
    check(g.changedOnly&&g.rows.empty(),"Returning restores the pending-only filter");
    command(FILTER_CHANGED,0);
    i18n::setLanguage(2);g.captions.clear();refresh();
    check(g.rows.size()==1&&g.selected==economySelection,"Chinese search works in English UI");
    const auto beforePaste = pending();
    for (auto query : {LR"(["BaseGame.GovernmentBudget"]=)", LR"([\"BaseGame.GovernmentBudget\"]=-5,)",
                       L"  BaseGame.GovernmentBudget = 500  "}) {
        SetWindowTextW(g.search,query);
        check(g.rows.size()==1 && g.selected==budget,"A copied guide assignment finds only its exact key");
    }
    check(pending()==beforePaste,"The value in a pasted search never edits the save");
    SetWindowTextW(g.search,LR"(["BaseGame.GovernmentBud"]=)");
    check(g.rows.empty(),"A wrapped incomplete key does not select a similarly named field");
    SetWindowTextW(g.search,L"GovernmentBud");
    check(g.rows.size()>1,"An ordinary keyword keeps matching several fields");
    i18n::setLanguage(0);
}

void testResourceSearch(const fs::path& path){
    i18n::setLanguage(0);
    loadDocumentBytes(readFile(path),path,0);
    navigate(0);
    auto findOne = [&](const wchar_t* query, const char* key) {
        SetWindowTextW(g.search,query);
        const auto description = "Resource query " + utf8(query) + " should select " + key +
            " (matches=" + std::to_string(g.rows.size()) + ", selection=" + g.selected + ")";
        check(g.rows.size()==1 && g.selected==key, description.c_str());
    };
    findOne(L"现有权威",riziaResources[1].stock);
    findOne(L"最大预算",riziaResources[0].cap);
    findOne(L"每回合能源收入",riziaResources[2].turnTotal);
    check(textOf(g.desc).find(L"可能重算")!=std::wstring::npos &&
          textOf(g.desc).find(L"未列出")!=std::wstring::npos,
          "A save without individual income modifiers explains their absence");
    i18n::setLanguage(2);g.captions.clear();refresh();
    findOne(L"per turn energy",riziaResources[2].turnTotal);
    check(textOf(g.desc).find(L"no individual modifiers")!=std::wstring::npos,
          "English inspector explains missing modifier data");
    i18n::setLanguage(1);g.captions.clear();refresh();
    findOne(L"預算餘額",riziaResources[0].stock);
    i18n::setLanguage(0);

    const std::string example = R"({"currentStoryPack":"StoryPack_Rizia","variables":"Variable={[\"RiziaDLC.Energy_Modifier_AureusGasField\"]=3,[\"RiziaDLC.Resources_EnergyModifiersTotal\"]=2,[\"RiziaDLC.Resources_Energy\"]=4}"})";
    loadDocumentBytes(example,{},0);
    navigate(4);
    SetWindowTextW(g.search,L"能源");
    check(g.rows.size()==3 && g.rows.front()==riziaResources[2].stock &&
          std::set<std::string>(g.rows.begin(),g.rows.end()).size()==3,
          "A broad search lists the current stock first and keeps each matching modifier once");
    SetWindowTextW(g.search,L"");
    check(std::is_sorted(g.rows.begin(),g.rows.end()),"Clearing a search restores alphabetical browsing");
    findOne(L"能源来源","RiziaDLC.Energy_Modifier_AureusGasField");
    findOne(L"能源净变化",riziaResources[2].turnTotal);
    check(textOf(g.desc).find(L"RiziaDLC.Energy_Modifier_")!=std::wstring::npos,
          "A save with income modifiers displays a searchable prefix");
    check(turnDelta(L"3")==L"+3" && turnDelta(L"-2")==L"-2" && turnDelta(L"0")==L"0" &&
          turnDelta(L"0.00e+9")==L"0.00e+9" && turnDelta(L"1.2e-2")==L"+1.2e-2" &&
          turnDelta(L"")==L"—" && turnDelta(L"—")==L"—", "Signed turn summaries preserve zeros and missing data");
    check(g.doc->render({})==example && pending().empty(), "Resource inspection preserves save bytes and edits");
}

void testReview(const fs::path& path){
    loadDocumentBytes(readFile(path),path,0);
    const std::string stock="RiziaDLC.Resources_Budget";
    const std::string cap="RiziaDLC.Rizia_HUDStat_Budget_Max";
    preset(99);
    check(pending().size()==3&&!pending().count(cap),"Resource preset stages stock only");
    check(textOf(ctl(NAV_BASE+5))==L"修改预览  6","Editing-page count includes all automatic cap rows");
    navigate(5);
    check(g.review.size()==6,"Review includes all derived caps");
    check(textOf(ctl(NAV_BASE+5))==L"修改预览  6","Review navigation count matches its actual row count");
    selectRow(cap);
    check(reviewRevertKey()==stock,"Derived cap identifies its stock source");
    check(textOf(ctl(REVIEW_REVERT))==L"撤回关联库存","Derived row identifies the linked action");
    command(REVIEW_REVERT,0);
    check(!g.review.count(cap)&&!g.review.count(stock)&&g.review.size()==4,"Linked reversion affects only one resource");
    check(textOf(ctl(NAV_BASE+5))==L"修改预览  4","Navigation count updates after linked reversion");
    command(HISTORY_UNDO,0);
    check(g.review.size()==6&&g.review.count(cap),"Undo restores the linked pair");
    command(HISTORY_REDO,0);
    check(g.review.size()==4&&!g.review.count(stock),"Redo reapplies the reversion");
    selectRow("RiziaDLC.Resources_Energy");
    command(REVIEW_REVERT,0);
    check(g.review.size()==2,"Explicit stock reversion recalculates the preview");

    g.session->reset();
    g.autoCap=false;
    preset(99);navigate(5);
    check(g.review.size()==3&&!g.reviewError.empty(),"Disabled cap sync leaves the explicit stock edits reviewable");
    check(!IsWindowEnabled(ctl(CONFIRM_SAVE))&&!IsWindowEnabled(ctl(SAVEAS)),"Invalid review disables both write actions");
    for(int i=0;i<3;++i)command(REVIEW_REVERT,0);
    check(g.rows.empty()&&g.reviewError.empty()&&pending().empty()&&!IsWindowEnabled(ctl(REVIEW_REVERT)),"Every conflicting edit can be removed in review");
    g.autoCap=true;
    g.session->applyBatch({{stock,"99"},{cap,"60"}});
    navigate(5);
    check(!g.reviewError.empty()&&!IsWindowEnabled(ctl(CONFIRM_SAVE)),"Explicit insufficient caps are reported");
    selectRow(cap);command(REVIEW_REVERT,0);
    check(g.reviewError.empty()&&g.review.at(cap)=="99"&&IsWindowEnabled(ctl(CONFIRM_SAVE)),
          "Removing an explicit cap override restores normal automatic adjustment");
}
}

int main(int argc,char** argv){
    if(argc!=3)return 2;
    try{
        INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES};
        InitCommonControlsEx(&controls);
        g.bg=CreateSolidBrush(Theme::bg);g.panel=CreateSolidBrush(Theme::panel);g.input=CreateSolidBrush(Theme::input);
        fonts();
        WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"SuzerainEditorRegression";
        check(RegisterClassW(&wc)!=0,"Register test editor");
        const HWND window=CreateWindowW(wc.lpszClassName,L"",WS_OVERLAPPEDWINDOW,0,0,1120,740,nullptr,nullptr,wc.hInstance,nullptr);
        check(window!=nullptr&&g.grid,"Create hidden editor");
        testSaveQueries();
        testSearchControls(fs::u8path(argv[1]));
        testSearch(fs::u8path(argv[1]));
        testResourceSearch(fs::u8path(argv[2]));
        testReview(fs::u8path(argv[2]));
        testAppearance(fs::u8path(argv[1]));
        testFavorites(fs::u8path(argv[1]),fs::u8path(argv[2]));
        testInspectorLayout(fs::u8path(argv[2]));
        DestroyWindow(window);
        std::cout<<"PASS: "<<checks<<" native editor checks\n";
        return 0;
    }catch(const std::exception& error){
        std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';
        return 1;
    }
}
