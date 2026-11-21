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
    check(Theme::index()==0,"A new user starts with Paper");
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
    check(Theme::index()==0,"An unsupported saved theme falls back to Paper");
    const wchar_t wrongType[]=L"3";
    RegSetKeyValueW(HKEY_CURRENT_USER,L"Software\\SuzerainSaveStudio",L"Appearance",
                    REG_SZ,wrongType,sizeof(wrongType));
    Theme::load();
    check(Theme::index()==0,"An invalid preference type falls back to Paper");
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
        testSearch(fs::u8path(argv[1]));
        testResourceSearch(fs::u8path(argv[2]));
        testReview(fs::u8path(argv[2]));
        testAppearance(fs::u8path(argv[1]));
        testFavorites(fs::u8path(argv[1]),fs::u8path(argv[2]));
        DestroyWindow(window);
        std::cout<<"PASS: "<<checks<<" native editor checks\n";
        return 0;
    }catch(const std::exception& error){
        std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';
        return 1;
    }
}
