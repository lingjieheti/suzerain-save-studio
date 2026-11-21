// Exercise the real list, edit control, preview and history in a hidden window.
// The two inputs are synthetic demos; this test performs no disk writes.
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
        testReview(fs::u8path(argv[2]));
        DestroyWindow(window);
        std::cout<<"PASS: "<<checks<<" native editor checks\n";
        return 0;
    }catch(const std::exception& error){
        std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';
        return 1;
    }
}
