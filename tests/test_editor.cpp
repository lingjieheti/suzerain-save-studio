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

void testInterfaceIcons(){
    using icons::Symbol;
    for(const int dpi : {96,144}){
        const int size=MulDiv(20,dpi,96), margin=4;
        glass::Dib image;
        check(image.size(nullptr,size+2*margin,size+2*margin),"Create a 32-bit icon test surface");
        const auto count=image.width*image.height;
        for(auto symbol : {Symbol::Economy,Symbol::Inventory,Symbol::People,Symbol::Globe,Symbol::Sliders,
                           Symbol::Review,Symbol::Archive,Symbol::Book,Symbol::Heart,Symbol::Reload,
                           Symbol::Folder,Symbol::Open,Symbol::Saves,Symbol::Undo,Symbol::Redo,Symbol::Search,
                           Symbol::Crown,Symbol::Pulse,Symbol::Arrow,Symbol::Document}){
            std::fill_n(image.pixels,count,0x00ffffffu);
            icons::draw(image.dc,symbol,margin,margin,size,RGB(0,0,0));
            GdiFlush();
            int dark=0,partial=0,outside=0,upperLeft=0,upperRight=0;
            for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x){
                const int intensity=image.pixels[y*image.width+x]&255;
                if(intensity==255)continue;
                if(x<margin || y<margin || x>=margin+size || y>=margin+size)++outside;
                if(intensity>0 && intensity<255)++partial;
                if(intensity<96){
                    ++dark;
                    if(y<margin+size/4){
                        if(x<margin+size/2)++upperLeft;else ++upperRight;
                    }
                }
            }
            if(!dark || !partial || outside)
                std::cerr<<"Icon "<<static_cast<int>(symbol)<<" at "<<dpi<<" DPI: "<<dark<<" dark, "
                         <<partial<<" antialiased, "<<outside<<" pixels outside\n";
            check(dark>0 && partial>0 && outside==0,"Each scaled icon has solid ink and antialiased edges within its bounds");
            if(symbol==Symbol::Undo)check(upperLeft>0 && upperRight==0,"Undo's upper arrowhead points left");
            if(symbol==Symbol::Redo)check(upperRight>0 && upperLeft==0,"Redo mirrors the undo arrowhead");
        }
    }
}

void selectRow(const std::string& key){
    const auto row=std::find(g.rows.begin(),g.rows.end(),key);
    check(row!=g.rows.end(),"Requested row is visible");
    const int index=(int)(row-g.rows.begin());
    ListView_SetItemState(g.grid,index,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    check(g.selected==key,"Native row selection reaches the editor");
}

bool hostedClipboardAvailable(){
    // Only the disposable GitHub-hosted Windows VM owns a test clipboard.
    const char* environment=std::getenv("RUNNER_ENVIRONMENT");
    const char* actions=std::getenv("GITHUB_ACTIONS");
    return environment && std::string(environment)=="github-hosted" && actions && std::string(actions)=="true";
}

void testSponsorDialog() {
    using sponsor_text::Key;
    const int originalLanguage=i18n::language;
    struct alignas(DWORD) Template { DLGTEMPLATE dialog{}; WORD menu=0,cls=0,title=0; } form;
    form.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;
    auto shown=[](HWND control){ return (GetWindowLongPtrW(control,GWL_STYLE)&WS_VISIBLE)!=0; };
    for(int language=0;language<static_cast<int>(std::size(i18n::names));++language) {
        i18n::setLanguage(language);
        sponsor::State state{};
        HWND dialog=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),&form.dialog,g.win,
                                               sponsor::proc,reinterpret_cast<LPARAM>(&state));
        check(dialog!=nullptr,"Create the support dialog for each app language");
        auto control=[&](int id){return GetDlgItem(dialog,id);};
        auto fits=[](HWND control,bool wrap) {
            RECT bounds{};GetClientRect(control,&bounds);
            HDC dc=GetDC(control);
            auto prev=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(control,WM_GETFONT,0,0)));
            RECT text={0,0,bounds.right,bounds.bottom};
            auto caption=textOf(control);
            DrawTextW(dc,caption.c_str(),-1,&text,DT_CALCRECT|DT_NOPREFIX|(wrap?DT_WORDBREAK:DT_SINGLELINE));
            SelectObject(dc,prev);ReleaseDC(control,dc);
            if(text.right>bounds.right || text.bottom>bounds.bottom)
                std::cerr<<"Support layout: language "<<i18n::language<<", control "<<GetDlgCtrlID(control)
                         <<", text "<<text.right<<"x"<<text.bottom<<", area "
                         <<bounds.right<<"x"<<bounds.bottom<<'\n';
            return text.right<=bounds.right && text.bottom<=bounds.bottom;
        };
        check(textOf(dialog)==sponsor_text::get(Key::Title) && fits(control(sponsor::Heading),false),
              "The support window has a localized title that fits");
        check(fits(control(sponsor::Introduction),true),"The introduction fits the support dialog");
        check(SendMessageW(control(sponsor::FirstMethod),BM_GETCHECK,0,0)==BST_CHECKED,
              "The WeChat appreciation code is selected on first open");
        constexpr Key names[]={Key::WeChat,Key::PayPal,Key::USDT};
        constexpr Key hints[]={Key::ScanWeChat,Key::ScanPayPal,Key::ScanUSDT};
        constexpr int bitmapIds[]={204,206,207};
        constexpr int count=static_cast<int>(std::size(names));
        static_assert(count==sponsor::methodCount);
        for(int method=0;method<count;++method) {
            const auto& details=sponsor::methods[method];
            const bool scanOnly=names[method]==Key::WeChat, hasAddress=names[method]==Key::USDT;
            auto radio=control(sponsor::FirstMethod+method);
            check(GetNextDlgGroupItem(dialog,radio,FALSE)==control(sponsor::FirstMethod+(method+1)%count) &&
                  GetNextDlgGroupItem(dialog,radio,TRUE)==control(sponsor::FirstMethod+(method+count-1)%count),
                  "The keyboard radio group cycles through all three support methods");
            if(method)SendMessageW(radio,BM_CLICK,0,0);
            check(SendMessageW(radio,BM_GETCHECK,0,0)==BST_CHECKED &&
                  textOf(radio)==sponsor_text::get(names[method]) && fits(radio,false),
                  "A support method selects its correctly localized tab");
            check(textOf(control(sponsor::Hint))==sponsor_text::get(hints[method]) && fits(control(sponsor::Hint),true),
                  "The selected method shows its matching scan instructions");
            BITMAP qr{};
            check(GetObjectW(state.bitmap,sizeof(qr),&qr)==sizeof(qr) && qr.bmWidth>0 && qr.bmWidth==qr.bmHeight &&
                  details.bitmapId==bitmapIds[method] && (!scanOnly || qr.bmBitsPixel>=24),
                  "Each method loads its matching square bitmap, including the full-color appreciation code");
            check(shown(control(sponsor::Action))==!scanOnly && shown(control(sponsor::Address))==hasAddress,
                  "Only the current method's controls are available");
            const auto expectedPage=names[method]==Key::PayPal?sponsor::paypalUrl:nullptr;
            check(details.page==expectedPage,"Each web action targets its own payment page");
            const auto caption=scanOnly?L"":sponsor_text::get(hasAddress?Key::CopyAddress:Key::OpenPage);
            check(textOf(control(sponsor::Action))==caption && (scanOnly||fits(control(sponsor::Action),false)),
                  "The correct action fits in the current language");

            RECT frame{},client{};
            GetWindowRect(dialog,&frame);GetClientRect(dialog,&client);
            const int frameWidth=frame.right-frame.left-client.right;
            const int frameHeight=frame.bottom-frame.top-client.bottom;
            for(const SIZE workArea: {SIZE{430,315},SIZE{384,265}}){
                SetWindowPos(dialog,nullptr,0,0,state.scale(workArea.cx)+frameWidth,
                             state.scale(workArea.cy)+frameHeight,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
                RECT viewport{};GetClientRect(dialog,&viewport);
                MapWindowPoints(dialog,nullptr,reinterpret_cast<POINT*>(&viewport),2);
                std::vector<RECT> visible;
                bool separate=true;
                for(HWND child=GetWindow(dialog,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){
                    if(!shown(child))continue;
                    RECT area{};GetWindowRect(child,&area);
                    separate=separate && area.right>area.left && area.bottom>area.top &&
                             area.left>=viewport.left && area.top>=viewport.top &&
                             area.right<=viewport.right && area.bottom<=viewport.bottom;
                    for(const auto& prior:visible){
                        RECT overlap{};if(IntersectRect(&overlap,&area,&prior))separate=false;
                    }
                    visible.push_back(area);
                }
                check(separate && fits(radio,false) && fits(control(sponsor::Hint),true) &&
                      (scanOnly||fits(control(sponsor::Action),false)),
                      "Short work areas keep the instructions and every visible control distinct and in bounds");
            }
            SetWindowPos(dialog,nullptr,0,0,frame.right-frame.left,frame.bottom-frame.top,
                         SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
            check(shown(control(sponsor::MethodName)),"The method heading returns when the dialog regains height");
        }
        const auto hint=textOf(control(sponsor::Hint));
        check(textOf(control(sponsor::Address))==L"TXE2VuxUSFsenJvmz3Z8oSm7dqPLAUWfRE" &&
              (GetWindowLongPtrW(control(sponsor::Address),GWL_STYLE)&ES_READONLY) &&
              hint.find(L"USDT")!=hint.npos && hint.find(L"TRC20")!=hint.npos && hint.find(L"TRON")!=hint.npos,
              "USDT always presents a read-only TRON address with the correct network");
        RECT client{},frame{};
        GetClientRect(dialog,&client);GetWindowRect(dialog,&frame);
        SetWindowPos(dialog,nullptr,0,0,state.scale(460)+frame.right-frame.left-client.right,
                     state.scale(325)+frame.bottom-frame.top-client.bottom,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        check(fits(control(sponsor::Heading),false) && fits(control(sponsor::Introduction),true) &&
              fits(control(sponsor::Hint),true) && fits(control(sponsor::Action),false),
              "Compact support dialog fits its title, introduction, network and copy action");
        HWND address=control(sponsor::Address);
        RECT addressBounds{};GetClientRect(address,&addressBounds);
        LRESULT last=SendMessageW(address,EM_POSFROMCHAR,GetWindowTextLengthW(address)-1,0);
        check(last!=-1 && LOWORD(last)<addressBounds.right && HIWORD(last)<addressBounds.bottom,
              "The final character of the compact receiving address stays in view");
        if(language==0 && hostedClipboardAvailable()) {
            SendMessageW(control(sponsor::Action),BM_CLICK,0,0);
            check(textOf(control(sponsor::Action))==sponsor_text::get(Key::Copied) && OpenClipboard(dialog),
                  "Copy address writes to the hosted runner clipboard");
            auto data=GetClipboardData(CF_UNICODETEXT);
            const auto* copied=data?static_cast<const wchar_t*>(GlobalLock(data)):nullptr;
            const bool correct=copied && textOf(control(sponsor::Address))==copied;
            if(copied)GlobalUnlock(data);
            CloseClipboard();
            check(correct,"The clipboard exactly matches the displayed TRON address");
            std::cout<<"PASS: support address copied and verified on the hosted Windows clipboard\n";
        }else if(language==0){
            std::cout<<"SKIP: live clipboard check reserved for a GitHub-hosted Windows runner\n";
        }
        SendMessageW(control(sponsor::FirstMethod),BM_CLICK,0,0);
        check(!shown(control(sponsor::Action)) && !shown(control(sponsor::Address)),
              "Returning to the WeChat appreciation code hides the previous address and action");
        DestroyWindow(dialog);
    }
    i18n::setLanguage(originalLanguage);
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
    auto matchesAt=[&](const wchar_t* input,const wchar_t* modified=L"2031-08-24  16:09:32"){
        snapshot.modifiedText=modified;
        return matches(input);
    };
    check(matchesAt(L"2031-08") && matchesAt(L"2031-08-24") && matchesAt(L"16:09") && matchesAt(L"16:09:32"),
          "Search for the displayed modification month, date, minute, or second");
    check(matchesAt(L"2031-08-24  16:09:32") && matchesAt(L"Rizia north turn 3 16:09"),
          "Combine modification time with campaign, file details and turn");
    check(!matchesAt(L"2031-09") && !matchesAt(L"2031-08-25") && !matchesAt(L"09:32") && !matchesAt(L"16:09:33") &&
          !matchesAt(L"Sordland 16:09") && !matchesAt(L"turn 24 16:09"),
          "Reject a different date, clock time, campaign, or turn");
    check(!matchesAt(L"24") && !matchesAt(L"16") && matchesAt(L"2031"),
          "A year finds the timestamp while short numeric queries retain their old meaning");
    check(!matchesAt(L"16:09",L"") && !matchesAt(L"2031-08-24",L"未知时间") &&
          matchesAt(L"13-10-2025",L"") && matchesAt(L"turn 3",L""),
          "Missing timestamps preserve filename and turn searches");
    snapshot.modifiedText.clear();
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

void beginPickerScan(save_picker::State& picker) {
    struct alignas(DWORD) Template { DLGTEMPLATE dialog{}; WORD menu=0,cls=0,title=0; } form;
    form.dialog.style=WS_POPUP|WS_CAPTION|DS_MODALFRAME;
    HWND window=CreateDialogIndirectParamW(GetModuleHandleW(nullptr),&form.dialog,g.win,
                                           save_picker::proc,reinterpret_cast<LPARAM>(&picker));
    check(window && GetFocus()==picker.filter,"The save picker opens ready for typing");
    MSG start{};
    check(PeekMessageW(&start,window,save_picker::START_SCAN,save_picker::START_SCAN,PM_REMOVE),
          "The picker queues its initial scan");
    DispatchMessageW(&start);
}

void finishPickerScan(save_picker::State& picker) {
    // Deliver completion deterministically even when the synthetic directory scans instantly.
    picker.worker.join();
    SendMessageW(picker.win,WM_TIMER,1,0);
}

void testPickerSearchWhileScanning(const fs::path& sample) {
    save_picker::State picker;
    picker.roots={sample.parent_path()};
    beginPickerScan(picker);
    check(picker.busy && IsWindowEnabled(picker.filter) && GetFocus()==picker.filter,
          "Starting the scan keeps the search box enabled and focused");
    for(wchar_t c:sample.stem().wstring())SendMessageW(GetFocus(),WM_CHAR,c,1);
    check(textOf(picker.filter)==sample.stem().wstring() && ListView_GetItemCount(picker.list)==0,
          "Typing during the scan records the query without exposing partial results");
    SetFocus(picker.campaign);
    finishPickerScan(picker);
    check(!picker.busy && GetFocus()==picker.campaign && picker.rows.size()==1 &&
          picker.result.saves[picker.rows.front()].path.filename()==sample.filename(),
          "The first results use the typed query and preserve the new focus");
    DestroyWindow(picker.win);
}

void testPickerCurrentSave(const fs::path& sample) {
    const auto root=fs::temp_directory_path()/(L"suzerain-picker-"+writeId());
    const auto newest=root/L"Slot.json", other=root/L"Other.json", current=root/L"older"/L"Slot.json";
    fs::create_directories(current.parent_path());
    fs::copy_file(sample,newest);fs::copy_file(sample,other);fs::copy_file(sample,current);
    const auto now=fs::file_time_type::clock::now();
    fs::last_write_time(newest,now);
    fs::last_write_time(other,now-std::chrono::hours(1));
    fs::last_write_time(current,now-std::chrono::hours(2));
    const auto contents=readFile(sample);
    auto openPicker=[&](const fs::path& opened,const fs::path& expected,bool refresh=false){
        loadDocumentBytes(contents,opened,opened.empty()?demo::sordland:0);
        save_picker::State picker;
        picker.roots={root};
        beginPickerScan(picker);finishPickerScan(picker);
        check(picker.rows.size()==3 && fs::path(textOf(picker.path))==expected,
              "Reopening selects the current full path, or the newest save if it is absent");
        if(refresh){
            ListView_SetItemState(picker.list,1,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
            SendMessageW(picker.win,WM_COMMAND,save_picker::RESCAN,0);
            finishPickerScan(picker);
            check(fs::path(textOf(picker.path))==other,"Refreshing preserves the player's new selection");
        }
        DestroyWindow(picker.win);
    };
    const auto alternate=current.parent_path()/L"."/L"SLOT.JSON";
    openPicker(fs::exists(alternate)?alternate:current,current,true);
    openPicker(root/L"missing"/L"Slot.json",newest);
    openPicker({},newest);
    for(const auto& path:{current,other,newest,current.parent_path(),root})fs::remove(path);
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

void dialogTab(bool backwards){
    BYTE previous[256]{},state[256]{};
    check(GetKeyboardState(previous)!=0,"Read the test thread's keyboard state");
    std::copy_n(previous,256,state);
    state[VK_SHIFT]=backwards?0x80:0;
    check(SetKeyboardState(state)!=0,"Set the test thread's Tab direction");
    MSG key{};
    key.hwnd=GetFocus();key.message=WM_KEYDOWN;key.wParam=VK_TAB;key.lParam=1;
    const bool consumed=IsDialogMessageW(g.win,&key)!=0;
    SetKeyboardState(previous);
    check(consumed,"The editor's dialog loop handles Tab");
}

void testGuideKeyboard() {
    navigate(7);
    for(int page:{HELP_USAGE,HELP_LICENSES}){
        command(page,BN_CLICKED);
        SetFocus(ctl(HELP_LICENSES));
        dialogTab(false);
        check(GetFocus()==g.helpText && SendMessageW(g.helpText,EM_GETSEL,0,0)==0,
              "Tab reaches the document at the beginning without selecting all text");
        const auto content=textOf(g.helpText);
        check(!content.empty(),"The selected guide or license is present");
        SendMessageW(g.helpText,WM_CHAR,'x',1);
        check(textOf(g.helpText)==content,"The document stays read-only during keyboard navigation");
        if(page==HELP_LICENSES){
            SendMessageW(g.helpText,WM_KEYDOWN,VK_NEXT,1);
            check(SendMessageW(g.helpText,EM_GETFIRSTVISIBLELINE,0,0)>0,"Page Down scrolls the license text");
            SendMessageW(g.helpText,WM_KEYDOWN,VK_PRIOR,1);
            check(SendMessageW(g.helpText,EM_GETFIRSTVISIBLELINE,0,0)==0,"Page Up returns to the first license line");
        }
        dialogTab(false);
        check(GetFocus()!=nullptr && GetFocus()!=g.helpText,"Tab exits the document");
        dialogTab(true);
        check(GetFocus()==g.helpText,"Shift+Tab returns to the document");
        dialogTab(true);
        check(GetFocus()==ctl(HELP_LICENSES),"Shift+Tab exits the document to its page controls");
    }
    command(HELP_USAGE,BN_CLICKED);
    navigate(0);
}

void testFieldDetailKeyboard(const fs::path& sample) {
    const auto original=readFile(sample);
    loadDocumentBytes(original,sample,0);
    for(int page:{0,4}){
        navigate(page);
        const std::string field="BaseGame.GovernmentBudget";
        selectRow(field);
        const auto detail=textOf(g.desc);
        check(detail.size()>=5,"The selected field has a description to read");
        SetFocus(g.grid);
        dialogTab(false);
        check(GetFocus()==g.desc && SendMessageW(g.desc,EM_GETSEL,0,0)==0,
              "Tab reaches the description without automatically selecting it");
        SendMessageW(g.desc,EM_SETSEL,1,5);
        if(page==4 && hostedClipboardAvailable()){
            SendMessageW(g.desc,WM_COPY,0,0);
            check(OpenClipboard(g.win)!=0,"Open the hosted Windows clipboard after copying field details");
            auto data=GetClipboardData(CF_UNICODETEXT);
            auto copied=data?static_cast<const wchar_t*>(GlobalLock(data)):nullptr;
            const bool correct=copied && detail.substr(1,4)==copied;
            if(copied)GlobalUnlock(data);
            CloseClipboard();
            check(correct,"Native copying includes just the selected portion of the description");
        }
        dialogTab(false);
        check(GetFocus()==ctl(FIELD_FAVORITE),"Tab leaves the description for the favorite button");
        dialogTab(true);
        check(GetFocus()==g.desc && SendMessageW(g.desc,EM_GETSEL,0,0)==MAKELONG(1,5),
              "Shift+Tab returns to the existing text selection");
        SendMessageW(g.desc,WM_CHAR,'x',1);
        dialogTab(true);
        check(GetFocus()==g.grid && textOf(g.desc)==detail && g.selected==field && pending().empty(),
              "Returning to the list preserves the read-only text, selected field and clean preview");
    }
    check(readFile(sample)==original,"Navigating descriptions leaves the sample file intact");
}

void testFieldActivation(const fs::path& sample) {
    const auto original=readFile(sample);
    const auto modified=fs::last_write_time(sample);
    loadDocumentBytes(original,sample,0);
    navigate(4);
    const std::string budget="BaseGame.GovernmentBudget";
    selectRow(budget);
    MSG enter{};
    enter.hwnd=g.grid;enter.message=WM_KEYDOWN;enter.wParam=VK_RETURN;
    check((SendMessageW(g.grid,WM_GETDLGCODE,VK_RETURN,reinterpret_cast<LPARAM>(&enter)) & DLGC_WANTMESSAGE)!=0,
          "The focused field list receives Enter from the dialog message loop");
    auto activate=[&](UINT code,int row=-1){
        NMITEMACTIVATE item{};
        item.hdr={g.grid,GRID,code};
        item.iItem=row;
        SetFocus(g.grid);
        SendMessageW(g.win,WM_NOTIFY,GRID,reinterpret_cast<LPARAM>(&item));
    };
    const int budgetRow=ListView_GetNextItem(g.grid,-1,LVNI_SELECTED);
    for(UINT code:{NM_RETURN,NM_DBLCLK}){
        SendMessageW(g.edit,EM_SETSEL,1,1);
        activate(code,budgetRow);
        check(GetFocus()==g.edit && SendMessageW(g.edit,EM_GETSEL,0,0)==MAKELONG(0,2) &&
              textOf(g.edit)==L"10" && pending().empty() && !g.editingDirty,
              "Activating a numeric field selects its full value without staging an edit");
    }
    activate(NM_DBLCLK,-1);
    check(GetFocus()==g.grid,"Double-clicking blank list space keeps focus in the list");

    const std::string flag="BaseGame.Faction_OldGuard_Content";
    selectRow(flag);
    activate(NM_RETURN);
    check(GetFocus()==ctl(BOOL_FALSE) && !g.editingDirty && pending().empty(),
          "Enter focuses the current false value without toggling it");
    command(BOOL_TRUE,BN_CLICKED);
    activate(NM_RETURN);
    check(GetFocus()==ctl(BOOL_TRUE) && g.editingDirty && pending().empty(),
          "Enter focuses a draft true value without applying it");
    check(applyInput(),"Stage the boolean for the review test");
    const rizia::ChangeMap edits{{flag,"true"}};
    SetWindowTextW(g.search,L"NoSuchSyntheticField");
    activate(NM_RETURN);
    check(GetFocus()!=g.edit && g.rows.empty() && pending()==edits,"An empty result cannot activate a field");
    navigate(5);
    selectRow(flag);
    activate(NM_RETURN);
    check(GetFocus()==g.grid && pending()==edits && !g.editingDirty,"Enter in review preserves the pending change");
    check(readFile(sample)==original && fs::last_write_time(sample)==modified,
          "Field and review activation preserve the source file and its timestamp");
}

void testHistoryShortcuts(const fs::path& sample) {
    const auto bytes=readFile(sample);
    loadDocumentBytes(bytes,sample,0);
    navigate(0);
    const std::string budget="BaseGame.GovernmentBudget";
    const rizia::ChangeMap changed{{budget,"41"}};
    SetWindowTextW(g.search,L"GB");
    selectRow(budget);
    SetWindowTextW(g.edit,L"41");
    check(applyInput() && pending()==changed,"Stage a budget edit before testing history shortcuts");
    check(handleEditorShortcut('Z',g.grid) && pending().empty() && g.session->canRedo(),
          "Ctrl+Z in the grid undoes a save edit");

    SendMessageW(g.search,EM_SETSEL,1,2);
    for(auto key:{'Y','Z'})
        check(!handleEditorShortcut(key,g.search),"The search box handles its own text history shortcuts");
    check(pending().empty() && g.session->canRedo() && g.selected==budget &&
          textOf(g.search)==L"GB" && SendMessageW(g.search,EM_GETSEL,0,0)==MAKELONG(1,2),
          "Search shortcuts leave the query, selection, caret and save history intact");

    SendMessageW(g.edit,EM_SETSEL,1,1);
    check(!handleEditorShortcut('Y',g.edit) && pending().empty() && g.session->canRedo(),
          "The value box cannot reapply an undone save edit");
    SetWindowTextW(g.edit,L"-7");
    SendMessageW(g.edit,EM_SETSEL,0,2);
    for(auto key:{'Z','Y'})
        check(!handleEditorShortcut(key,g.edit),"An uncommitted value keeps its text history shortcuts");
    check(g.editingDirty && textOf(g.edit)==L"-7" && SendMessageW(g.edit,EM_GETSEL,0,0)==MAKELONG(0,2) &&
          g.selected==budget && pending().empty() && g.session->canRedo(),
          "Shortcut dispatch preserves the value draft, selection and available save redo");

    command(UNDOONE,0);
    for(int repeat=0;repeat<2;++repeat) {
        check(handleEditorShortcut('Y',g.grid) && pending()==changed && !g.session->canRedo(),
              "Ctrl+Y in the grid reapplies the staged budget");
        check(handleEditorShortcut('Z',g.grid) && pending().empty() && g.session->canRedo(),
              "The redone budget remains undoable in the grid");
    }
    check(g.doc->render({})==bytes && readFile(sample)==bytes,"History shortcuts preserve the synthetic source");
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

void testBackupRefreshAndRestore(const fs::path& sample){
    AppearanceTestRegistry registry;
    const auto root=fs::temp_directory_path()/(L"suzerain-backup-ui-"+writeId());
    const auto target=root/L"Active.json",folder=root/L".rizia-backups";
    fs::create_directories(folder);
    fs::copy_file(sample,target);
    const auto original=readFile(target);
    loadDocumentBytes(original,target,0);
    navigate(6);
    const int count=std::max(1,ListView_GetCountPerPage(g.grid))+5;
    for(int i=0;i<count;++i)
        fs::copy_file(sample,folder/(L"Active.json.slot-"+std::to_wstring(i)+L".bak"));
    command(BACKUP_REFRESH,0);
    check(static_cast<int>(g.backups.size())==count,"Backup history extends beyond one page");
    const int last=count-1;
    ListView_SetItemState(g.grid,last,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    ListView_EnsureVisible(g.grid,last,FALSE);
    const auto selected=g.backups[last].path;
    command(BACKUP_REFRESH,0);
    const int top=ListView_GetTopIndex(g.grid);
    check(g.backupIndex==last && g.backups[last].path==selected &&
          ListView_GetNextItem(g.grid,-1,LVNI_SELECTED)==last &&
          top<=last && last<top+ListView_GetCountPerPage(g.grid),
          "Refreshing keeps the previewed backup selected and in view");

    auto cancelShown=[]{return (GetWindowLongPtrW(ctl(BACKUP_CANCEL),GWL_STYLE)&WS_VISIBLE)!=0;};
    command(BACKUP_RESTORE,0);
    check(g.restoreArmed && cancelShown(),"The first restore click shows its cancel action");
    std::ofstream(selected,std::ios::binary|std::ios::app).put('\n');
    const auto timer=SetTimer(nullptr,0,50,dismissValidationWarning);
    check(timer!=0,"Install a timer to dismiss the changed-backup warning");
    command(BACKUP_RESTORE,0);
    KillTimer(nullptr,timer);
    check(!g.restoreArmed && !cancelShown() && readFile(target)==original,
          "A changed backup clears confirmation and leaves the open save intact");
    for(const auto& backup:g.backups)fs::remove(backup.path);
    fs::remove(target);fs::remove(folder);fs::remove(root);
}

void testBackupSaveAs(){
    const auto root=fs::temp_directory_path()/(L"suzerain-backup-copy-"+writeId());
    const auto archive=root/L".RIZIA-BACKUPS",nested=archive/L"older";
    fs::create_directories(nested);
    const auto backup=nested/(L"Chapter.2.json.20261127-010203-004."+writeId()+L".bak");
    const auto copy=root/L"Chapter.2.recovered.json";
    const std::string original=std::string("\xEF\xBB\xBF")+"{\r\n"+
        R"("currentStoryPack":"StoryPack_Rizia","variables":"Variable={['RiziaDLC.Resources_Budget']=5,['RiziaDLC.Rizia_HUDStat_Budget_Max']=20}")"+"\r\n}\r\n";
    std::ofstream(backup,std::ios::binary)<<original;
    loadDocument(backup,false);
    SendMessageW(g.win,WM_CHANGEUISTATE,MAKEWPARAM(UIS_SET,UISF_HIDEFOCUS),0);
    check(handleEditorShortcut('S',GetFocus()) && !(SendMessageW(ctl(SAVEAS),WM_QUERYUISTATE,0,0)&UISF_HIDEFOCUS),
          "Ctrl+S reveals keyboard focus when opening the backup preview after pointer input");
    check(g.file==backup && g.backupCopyTarget==copy,"Suggest a new JSON beside the outer backup directory using the original slot name");
    check(g.review.empty() && IsWindowEnabled(ctl(SAVEAS)) && !IsWindowEnabled(ctl(CONFIRM_SAVE)) &&
          !IsWindowEnabled(ctl(PATCH_EXPORT)),"A pristine backup permits Save As with no patch or overwrite action");
    check(GetFocus()==ctl(SAVEAS),"An empty backup preview focuses the available Save As action");
    check(textOf(g.desc)==tr(L"请在备份目录外另存一份存档。"),"The preview directs backup readers to Save As");
    RECT saveAs{},overwrite{};
    GetWindowRect(ctl(SAVEAS),&saveAs);GetWindowRect(ctl(CONFIRM_SAVE),&overwrite);
    check(saveAs.bottom<=overwrite.top,"Save As occupies the primary position for an open backup");
    command(CONFIRM_SAVE,BN_CLICKED);
    check(!fs::exists(copy) && readFile(backup)==original,"The overwrite command cannot write through a protected source");
    atomicSave(copy,g.doc->render(preparedChanges()),nullptr);
    check(readFile(copy)==original && readFile(backup)==original,"An unchanged recovery copy retains the BOM, CRLFs and original bytes");
    const std::string budget="RiziaDLC.Resources_Budget", cap="RiziaDLC.Rizia_HUDStat_Budget_Max";
    g.session->applyBatch({{budget,"15"}});refresh();
    check(IsWindowEnabled(ctl(SAVEAS)) && !IsWindowEnabled(ctl(CONFIRM_SAVE)) && IsWindowEnabled(ctl(PATCH_EXPORT)),
          "An edited recovery point permits a copy or patch and still protects its source");
    g.session->applyBatch({{cap,"2"}});refresh();
    check(!g.reviewError.empty() && !IsWindowEnabled(ctl(SAVEAS)),"An invalid cap cannot be saved from a backup");
    loadDocument(copy,false);navigate(5);
    check(g.backupCopyTarget.empty() && !IsWindowEnabled(ctl(SAVEAS)),"Opening the recovered file restores normal save controls");
    check(GetFocus()==ctl(NAV_BASE+5),"An unchanged regular save retains normal empty-preview focus");
    check(suggestedBackupCopy(archive/L"run.json.before.war.bak")==root/L"run.recovered.json",
          "Manually labeled backups use the original save name");
    fs::remove(backup);fs::remove(copy);fs::remove(nested);fs::remove(archive);fs::remove(root);
}

void testReloadView(){
    AppearanceTestRegistry registry;
    const auto root=fs::temp_directory_path()/(L"suzerain-reload-view-"+writeId());
    const auto target=root/L"Active.json",other=root/L"Other.json",folder=root/L".rizia-backups";
    fs::create_directories(folder);
    const std::string score="BaseGame.Public_Opinion";
    const std::string initial=R"({"currentStoryPack":"StoryPack_Main","variables":"Variable={['BaseGame.GovernmentBudget']=3,['BaseGame.Public_Opinion']=9,['BaseGame.OpinionOther']=2}"})";
    const std::string updated=R"({"currentStoryPack":"StoryPack_Main","variables":"Variable={['BaseGame.GovernmentBudget']=8,['BaseGame.Public_Opinion']=17,['BaseGame.OpinionOther']=4}"})";
    const std::string removed=R"({"currentStoryPack":"StoryPack_Main","variables":"Variable={['BaseGame.GovernmentBudget']=10,['BaseGame.OpinionOther']=7}"})";
    auto write=[&](const std::string& data){std::ofstream(target,std::ios::binary)<<data;};
    auto reloadWithAnswer=[&](int answer){
        // The timer supplies the player's choice to the real discard confirmation.
        const auto timer=SetTimer(g.win,answer,30,[](HWND,UINT,UINT_PTR button,DWORD){
            EnumThreadWindows(GetCurrentThreadId(),[](HWND dialog,LPARAM choice)->BOOL{
                if(GetDlgItem(dialog,IDYES)&&GetDlgItem(dialog,IDNO))EndDialog(dialog,choice);
                return TRUE;
            },button);
        });
        check(timer!=0,"Schedule the reload confirmation answer");
        command(RELOAD,0);KillTimer(g.win,timer);
    };
    write(initial);loadDocument(target,false);navigate(0);
    SetWindowTextW(g.search,L"GovernmentBudget");
    navigate(4);SetWindowTextW(g.search,L"Opinion");selectRow(score);
    command(FIELD_FAVORITE,0);command(FILTER_FAVORITES,0);
    SetWindowTextW(g.edit,L"25");
    write(updated);reloadWithAnswer(IDNO);
    check(g.editingDirty && textOf(g.edit)==L"25" && g.doc->original==initial,
          "Cancelling a reload preserves the draft and the old document");
    reloadWithAnswer(IDYES);
    check(g.tab==4 && textOf(g.search)==L"Opinion" && g.favoritesOnly &&
          g.rows==std::vector<std::string>{score} && g.selected==score && textOf(g.edit)==L"17",
          "Reload shows the new disk value at the selected field with the same query and favorites filter");
    check(!g.editingDirty && pending().empty() && !g.session->canUndo() && !g.session->canRedo(),
          "Confirming reload starts clean editing history");
    write("{");
    const auto warning=SetTimer(nullptr,0,30,dismissValidationWarning);
    check(warning!=0,"Schedule dismissal of the invalid save warning");
    command(RELOAD,0);KillTimer(nullptr,warning);
    check(g.doc->original==updated && g.selected==score && textOf(g.edit)==L"17" &&
          g.favoritesOnly && textOf(g.search)==L"Opinion",
          "An unreadable replacement preserves the loaded document and field view");
    write(updated);
    navigate(0);check(textOf(g.search)==L"GovernmentBudget" && textOf(g.edit)==L"8",
                      "Reload retains the other field pages' searches and reads their new values");
    navigate(4);SetWindowTextW(g.edit,L"31");command(FILTER_CHANGED,0);
    reloadWithAnswer(IDYES);
    check(g.changedOnly && g.rows.empty() && g.selected.empty() && textOf(g.edit).empty() && !IsWindowEnabled(g.edit),
          "Pending-only remains enabled and displays no stale values after discarding pending changes");
    command(FILTER_CHANGED,0);write(removed);command(RELOAD,0);
    check(g.favoritesOnly && textOf(g.search)==L"Opinion" && g.rows.empty() && textOf(g.edit).empty(),
          "A removed favorite leaves its search empty and clears the inspector");
    command(FILTER_FAVORITES,0);
    check(g.selected=="BaseGame.OpinionOther" && textOf(g.edit)==L"7","An available match becomes the selected field");
    SetWindowTextW(g.edit,L"19");navigate(5);reloadWithAnswer(IDYES);
    check(g.tab==5 && g.review.empty() && g.rows.empty() && !IsWindowEnabled(ctl(CONFIRM_SAVE)),
          "Reload stays on Review with a freshly empty change list");
    const auto older=folder/L"Active.json.20261001-010203-001.bak";
    const auto newer=folder/L"Active.json.20261001-010203-002.bak";
    fs::copy_file(target,older);fs::copy_file(target,newer);
    navigate(6);ListView_SetItemState(g.grid,1,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    command(BACKUP_RESTORE,0);check(g.restoreArmed,"Arm the selected backup");
    command(RELOAD,0);
    check(g.tab==6 && g.backupIndex==1 && g.backups[1].path==older && g.backupDoc && !g.restoreArmed,
          "Reload refreshes the selected backup preview and cancels the previous restore confirmation");
    navigate(7);command(RELOAD,0);check(g.tab==7,"Reload keeps the help page open");
    const std::string rizia=R"({"currentStoryPack":"StoryPack_Rizia","variables":"Variable={['RiziaDLC.Resources_Budget']=2}"})";
    write(rizia);command(RELOAD,0);
    check(g.tab==0 && textOf(g.search).empty() && g.pages[4].query.empty() && !g.favoritesOnly && !g.changedOnly,
          "A different story at the same path starts with the normal clean view");
    navigate(4);SetWindowTextW(g.search,L"Rizia");fs::copy_file(target,other);
    loadDocument(other);
    check(textOf(g.search).empty() && g.pages[4].query.empty(),"Opening a different save still clears field search");
    for(const auto& item:{newer,older,folder,target,other,root})fs::remove(item);
}

void testChoiceHintsPreference(){
    AppearanceTestRegistry registry;
    check(!live_setup::choiceHintsEnabled(),"Choice previews default to off");
    live_setup::setChoiceHints(true);
    check(live_setup::choiceHintsEnabled(),"Enable hover previews");
    live_setup::setChoiceHints(false);
    check(!live_setup::choiceHintsEnabled(),"Disable hover previews");
    const DWORD invalid=2;
    RegSetKeyValueW(HKEY_CURRENT_USER,Theme::settingsKey,L"ChoiceHints",REG_DWORD,&invalid,sizeof(invalid));
    check(!live_setup::choiceHintsEnabled(),"Unknown hint settings default to off");
}

void testChineseFonts(const fs::path& path) {
    using namespace typography;
    AppearanceTestRegistry registry;
    const auto originalLanguage=i18n::language;
    const auto original=typography::selected;
    check(stored()==Automatic,"Missing Chinese font preference uses automatic selection");
    const DWORD invalid=90;
    RegSetKeyValueW(HKEY_CURRENT_USER,Theme::settingsKey,L"ChineseUIFont",REG_DWORD,&invalid,sizeof(invalid));
    check(stored()==Automatic,"Unknown Chinese font preference uses automatic selection");

    ChineseFamilies simulated{};
    check(effectiveFont(simulated,PingFang)==Automatic,"Missing all optional fonts uses the generic fallback");
    for(auto font:{Noto,System,PingFang}){
        simulated[font]={L"Test font",L"Test font"};
        check(effectiveFont(simulated,Automatic)==font,"Automatic prioritizes PingFang, Windows UI, then Noto");
        check(effectiveFont(simulated,Noto)==Noto,"An installed explicit choice takes priority");
    }
    i18n::setLanguage(0);useChineseFont(Automatic);
    loadDocumentBytes(readFile(path),path,0);
    SetWindowTextW(g.search,L"GovernmentBudget");
    selectRow("BaseGame.GovernmentBudget");
    SetWindowTextW(g.edit,L"79");
    auto* session=g.session.get();
    for(int language:{0,1,2}){
        i18n::setLanguage(language);
        const auto local=chineseFamilies(language);
        auto menu=CreatePopupMenu();
        AppearanceMenuMetrics metrics{96,g.font};
        populateAppearanceMenu(menu,metrics);
        for(auto finish:{glass::Clear,glass::Frosted,glass::Solid}){
            const auto state=GetMenuState(menu,glass::firstFinishCommand+finish,MF_BYCOMMAND);
            check(state!=UINT(-1) && bool(state&MF_CHECKED)==(finish==glass::effectiveFinish()),
                  "The menu checks exactly the finish that is currently visible");
        }
        if(!isChinese(language)){
            check(GetMenuItemCount(menu)==Theme::count+4,"All languages include themes and three material choices");
        }else{
            check(GetMenuItemCount(menu)==Theme::count+5+fontCount,"Both Chinese languages also expose a font section");
            check(metrics.fontNames[Automatic].find(L'（')!=std::wstring::npos,
                  "Automatic selection displays the current fallback");
            for(auto font:{PingFang,Noto,System}){
                const bool present=local[font].text!=nullptr;
                const int cmd=firstFontCommand+font;
                check(((GetMenuState(menu,cmd,MF_BYCOMMAND)&MF_DISABLED)==0)==present,
                      "Menu only enables fonts installed for the selected written language");
                const auto before=typography::selected;
                applyAppearanceCommand(cmd);
                check(typography::selected==(present?font:before) && stored()==typography::selected,
                      "Installed selections persist and unavailable faces cannot be selected");
                if(present){
                    LOGFONTW chosen{};
                    GetObjectW(g.font,sizeof(chosen),&chosen);
                    check(std::wstring(chosen.lfFaceName)==local[font].text,"The editor uses the selected family");
                }else{
                    auto saved=typography::selected;
                    typography::selected=font;
                    auto fallback=CreatePopupMenu();
                    populateAppearanceMenu(fallback,metrics);
                    check((GetMenuState(fallback,firstFontCommand+Automatic,MF_BYCOMMAND)&MF_CHECKED)!=0 &&
                          (GetMenuState(fallback,cmd,MF_BYCOMMAND)&MF_CHECKED)==0,
                          "A previously selected missing face checks its actual automatic fallback");
                    DestroyMenu(fallback);
                    typography::selected=saved;
                }
            }
            applyAppearanceCommand(firstFontCommand+Automatic);
            check(stored()==Automatic,"Users can switch back to automatic");
        }
        DestroyMenu(menu);
    }
    check(g.session.get()==session && textOf(g.edit)==L"79" && g.editingDirty &&
          textOf(g.search)==L"GovernmentBudget","Changing fonts preserves search, session and uncommitted input");
    i18n::setLanguage(originalLanguage);typography::selected=original;fonts();layout();
}

void testTypography(const fs::path& path) {
    loadDocumentBytes(readFile(path),path,0);
    navigate(0);
    const auto originalDpi=g.dpi;
    const int originalTheme=Theme::index(), originalLanguage=i18n::language;
    const auto dc=GetDC(g.win);
    for (const auto dpi : {96u,144u,192u}) {
        g.dpi=dpi;
        SetWindowPos(g.win,nullptr,0,0,px(840),px(650),SWP_NOMOVE|SWP_NOZORDER);
        for (const auto language : {0,1,2,3,4}) {
            i18n::setLanguage(language);
            for (int theme=0;theme<Theme::count;++theme) {
                Theme::use(theme);
                fonts();
                layout();
                for (const auto digits : {g.large,g.compactNumber,g.tableNumber,g.mono}) {
                    auto old=SelectObject(dc,digits);
                    int widths[10]{};
                    const bool measured=GetCharWidth32W(dc,L'0',L'9',widths)!=0;
                    SelectObject(dc,old);
                    const auto range=std::minmax_element(std::begin(widths),std::end(widths));
                    check(measured && *range.first>0 && *range.first==*range.second,
                          "Dashboard, table and input digits retain equal advance widths");
                }
                for (const auto field : {g.search,g.edit}) {
                    auto old=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(field,WM_GETFONT,0,0)));
                    TEXTMETRICW metrics{};
                    const bool measured=GetTextMetricsW(dc,&metrics)!=0;
                    SelectObject(dc,old);
                    RECT area{};GetClientRect(field,&area);
                    const auto context=std::string(field==g.search?"Search":"Value")+" input at "+
                        std::to_string(dpi)+" DPI, language "+std::to_string(language)+", theme "+
                        std::to_string(theme)+" requires "+std::to_string(metrics.tmHeight)+
                        "px; available "+std::to_string(area.bottom-area.top)+"px";
                    check(measured && metrics.tmHeight<=area.bottom-area.top,
                          context.c_str());
                }
            }
        }
    }
    g.dpi=96;
    SetWindowPos(g.win,nullptr,0,0,px(680),px(630),SWP_NOMOVE|SWP_NOZORDER);
    for (int language=0;language<static_cast<int>(std::size(i18n::names));++language) {
        i18n::setLanguage(language);fonts();
        for (int tab : {0,5}) {
            navigate(tab);
            for (int col : {1,2}) {
                const auto heading=tr(col==1?L"原值":tab==5?L"保存为":L"待保存");
                check(ListView_GetColumnWidth(g.grid,col)>=typography::width(dc,g.small,heading)+px(16),
                      "Original and proposed value headings fit in the compact table");
            }
        }
    }
    i18n::setLanguage(2);fonts();
    SetWindowPos(g.win,nullptr,0,0,px(1160),px(770),SWP_NOMOVE|SWP_NOZORDER);
    for (int theme=0;theme<Theme::count;++theme) {
        Theme::use(theme);updateAppearanceLabel();layout();
        RECT area{};GetClientRect(ctl(APPEARANCE),&area);
        check(typography::width(dc,g.small,textOf(ctl(APPEARANCE)))<=area.right-area.left-px(43),
              "The wide appearance button fits the full English theme name");
    }
    ReleaseDC(g.win,dc);
    g.dpi=originalDpi;i18n::setLanguage(originalLanguage);Theme::use(originalTheme);
    fonts();layout();
}

void testGlassFinishes(){
    std::array<DWORD,glass::LensMap::cols*glass::LensMap::rows> backdrop{};
    const int stride=glass::LensMap::cols;
    backdrop[1]=0x00ff0000;backdrop[stride]=0x0000ff00;backdrop[stride+1]=0x000000ff;
    check(glass::sampleBackdrop(backdrop,{128,128})==0x00404040,
          "A half-texel bend combines all four backdrop colors without color-channel carry");
    backdrop.back()=0x00708fce;
    check(glass::sampleBackdrop(backdrop,{(glass::LensMap::cols-1)*256,(glass::LensMap::rows-1)*256})==backdrop.back(),
          "The final backdrop texel is preserved at the sampling boundary");
    const auto& optics=glass::lensMap(192,38,19);
    const auto end=optics.samples[(glass::LensMap::rows/3)*stride+2];
    const auto centerPoint=optics.samples[(glass::LensMap::rows/2)*stride+stride/2];
    check(((end.x|end.y)&255)!=0 && centerPoint.x==stride/2*256 && centerPoint.y==glass::LensMap::rows/2*256,
          "The rounded shoulder retains subpixel refraction while the reading area stays undistorted");
    const int originalTheme=Theme::index();
    const auto originalFinish=glass::preferred;
    const bool originalSystem=glass::systemAllows;
    glass::systemAllows=true;
    auto dc=GetDC(g.win);
    glass::Dib sample;
    const bool created=sample.size(dc,150,55);
    ReleaseDC(g.win,dc);
    check(created,"A compatible surface is available for the material check");
    if(created){
        auto center=[&](COLORREF background){
            fill(sample.dc,{0,0,150,55},background);
            glass::surface(sample.dc,{5,5,145,50},Theme::panel,18);
            return GetPixel(sample.dc,75,27);
        };
        for(int theme=0;theme<Theme::count;++theme){
            Theme::use(theme);
            auto transmission=[&](glass::Finish finish){
                glass::preferred=finish;
                const auto dark=center(RGB(0,0,0));
                const auto bright=center(RGB(255,255,255));
                return int(GetGValue(bright))-int(GetGValue(dark));
            };
            const auto clear=transmission(glass::Clear);
            const auto frosted=transmission(glass::Frosted);
            check(clear>frosted && frosted>0,"Frosted transmits less backdrop contrast than clear in every theme");
            check(transmission(glass::Solid)==0 && center(RGB(0,0,0))==Theme::panel,
                  "Solid gives the exact theme surface regardless of its backdrop");
        }
    }
    Theme::use(originalTheme);
    glass::preferred=originalFinish;
    glass::systemAllows=originalSystem;
}

void testFileToolbarGroups(){
    const int originalTheme=Theme::index();
    const auto originalFinish=glass::preferred;
    const bool originalSystem=glass::systemAllows;
    RECT original{};GetWindowRect(g.win,&original);
    const auto buttonBounds=[](int id){
        RECT box{};GetWindowRect(ctl(id),&box);
        MapWindowPoints(nullptr,g.win,reinterpret_cast<POINT*>(&box),2);
        return box;
    };
    for(int width:{680,830,1200}){
        SetWindowPos(g.win,nullptr,0,0,px(width),px(width==680?485:730),SWP_NOMOVE|SWP_NOZORDER);
        layout();
        const auto reload=buttonBounds(RELOAD),folder=buttonBounds(FOLDER);
        const auto open=buttonBounds(OPEN),latest=buttonBounds(LATEST);
        const auto geometry=workspaceGeometry();
        const int previousWidth=std::min(465,geometry.content*62/100);
        const int previousUnit=(previousWidth-18)/10;
        const std::array<int,4> previousWidths={2*previousUnit,2*previousUnit,3*previousUnit,previousWidth-18-7*previousUnit};
        const std::array<RECT,4> buttons={reload,folder,open,latest};
        for(size_t i=0;i<buttons.size();++i)
            check(buttons[i].right-buttons[i].left==px(previousWidths[i]),
                  "Every file command retains its original width at minimum, compact and standard window sizes");
        check(reload.left==px(geometry.left+geometry.content-previousWidth-8) &&
              std::abs(latest.right-px(geometry.left+geometry.content))<=1,
              "The additional room for the glass gap comes entirely from widening the toolbar to the left");
        if(width==1200){
            const int previousLanguage=i18n::language;
            i18n::setLanguage(0);fonts();
            const auto dc=GetDC(g.win);
            for(int id:std::initializer_list<int>{RELOAD,FOLDER,OPEN,LATEST}){
                const auto button=ctl(id);
                const auto bounds=buttonBounds(id);
                const auto caption=tr(uiText.at(button));
                const auto font=id==LATEST && !g.doc?g.emphasis:g.font;
                check(px(22)+typography::width(dc,font,caption)+px(10)<=bounds.right-bounds.left,
                      "At normal Chinese window width every file command fits its icon and complete caption");
            }
            ReleaseDC(g.win,dc);
            i18n::setLanguage(previousLanguage);fonts();
        }
        const int separator=(folder.right+open.left)/2,centerY=(open.top+open.bottom)/2;
        const int localJoin=(reload.right+folder.left)/2,openJoin=(open.right+latest.left)/2;
        check(open.left-folder.right>folder.left-reload.right+px(12) &&
              open.left-folder.right>latest.left-open.right+px(12),
              "Actual file command positions give the two related pairs distinct spacing");
        RECT client{};GetClientRect(g.win,&client);
        glass::Dib canvas,toolbar;
        check(canvas.size(nullptr,client.right,client.bottom) && toolbar.size(nullptr,client.right,client.bottom),
              "Create matching toolbar and unadorned workspace surfaces");
        for(int theme=0;theme<Theme::count;++theme){
            Theme::use(theme);
            for(int mode=0;mode<4;++mode){
                glass::systemAllows=mode!=3;
                glass::preferred=mode==1?glass::Frosted:mode==2?glass::Solid:glass::Clear;
                glass::wash(canvas.dc,client,Theme::bg);
                drawWorkspaceBackdrop(toolbar.dc,client,workspaceGeometry());
                const auto isCanvas=[&](int x,int y){return GetPixel(canvas.dc,x,y)==GetPixel(toolbar.dc,x,y);};
                bool exposed=true;
                for(int y=centerY-px(10);y<=centerY+px(10);++y)
                    for(int x=separator-px(1);x<=separator+px(1);++x)
                        exposed=exposed && isCanvas(x,y);
                check(exposed,"Real canvas stays visible through the gap in every theme and transparency mode");
                for(int join:{localJoin,openJoin}){
                    int painted=0;
                    for(int y=centerY-px(8);y<=centerY+px(8);++y)painted+=!isCanvas(join,y);
                    check(painted>=px(4),"Adjacent commands within each group share one continuous material");
                }
            }
        }
    }
    Theme::use(originalTheme);glass::preferred=originalFinish;glass::systemAllows=originalSystem;
    SetWindowPos(g.win,nullptr,0,0,original.right-original.left,original.bottom-original.top,SWP_NOMOVE|SWP_NOZORDER);
    layout();
}

void testGuideSegments(){
    const int originalTheme=Theme::index(),originalTab=g.tab;
    const auto originalFinish=glass::preferred;
    const bool originalSystem=glass::systemAllows,originalLicense=g.licenseMode;
    navigate(7);
    RECT bounds{};
    GetClientRect(ctl(HELP_USAGE),&bounds);
    glass::Dib image;
    check(image.size(nullptr,bounds.right,bounds.bottom),"Create a surface the size of a guide segment");
    auto segmentColor=[&](int id){
        DRAWITEMSTRUCT item{};
        item.CtlType=ODT_BUTTON;item.CtlID=id;item.hwndItem=ctl(id);
        item.itemAction=ODA_DRAWENTIRE;item.hDC=image.dc;item.rcItem=bounds;
        drawButton(&item);
        return GetPixel(image.dc,px(10),bounds.bottom/2);
    };
    auto labelContrast=[&](int id){
        const auto ink=GetTextColor(image.dc);
        const auto button=ctl(id);
        const auto label=textOf(button);
        const int halfWidth=typography::width(image.dc,g.emphasis,label)/2;
        const int centerX=bounds.right/2,centerY=bounds.bottom/2;
        // Repaint without glyphs to measure the actual glass beneath the caption.
        rawText(button,L"");segmentColor(id);rawText(button,label);
        double minimum=21.;
        for(int y=centerY-px(5);y<=centerY+px(5);y+=std::max(1,px(2)))
            for(int x=centerX-halfWidth;x<=centerX+halfWidth;x+=std::max(1,px(2)))
                minimum=std::min(minimum,textContrast(ink,GetPixel(image.dc,x,y)));
        return minimum;
    };
    for(int theme=0;theme<Theme::count;++theme){
        Theme::use(theme);
        glass::systemAllows=true;
        for(auto finish:{glass::Clear,glass::Frosted,glass::Solid}){
            glass::preferred=finish;
            for(int id:{HELP_USAGE,HELP_LICENSES}){
                command(id==HELP_USAGE?HELP_LICENSES:HELP_USAGE,BN_CLICKED);
                const auto idle=segmentColor(id);
                command(id,BN_CLICKED);
                const auto active=segmentColor(id);
                check(idle!=active,"Switching pages changes each guide segment's visible selection");
                const auto contrast=labelContrast(id);
                if(contrast<4.5)std::cerr<<"Guide theme "<<theme<<", finish "<<finish<<", control "<<id
                                        <<", surface "<<active<<", contrast "<<contrast<<"\n";
                check(contrast>=4.5,"The selected guide label contrasts with its rendered surface");
            }
        }
        glass::preferred=glass::Clear;
        glass::systemAllows=false;
        const auto fallback=segmentColor(HELP_LICENSES);
        check(fallback==Theme::accent && labelContrast(HELP_LICENSES)>=4.5,
              "System-suppressed transparency keeps an opaque, legible guide selection");
    }
    Theme::use(originalTheme);
    glass::systemAllows=originalSystem;glass::preferred=originalFinish;
    command(originalLicense?HELP_LICENSES:HELP_USAGE,BN_CLICKED);
    navigate(originalTab);
}

void testAppearance(const fs::path& path){
    AppearanceTestRegistry registry;
    Theme::load();
    check(Theme::index()==2,"A new user starts with Fjord");
    glass::refreshSettings();
    testGlassFinishes();
    testFileToolbarGroups();
    testGuideSegments();
    check(glass::preferred==glass::Clear,"A new profile uses the clear finish by default");
    auto appearanceButton=ctl(APPEARANCE);
    SendMessageW(appearanceButton,WM_MOUSEMOVE,0,MAKELPARAM(5,6));
    check(glassPointer(appearanceButton).x==5 && glassPointer(appearanceButton).y==6,
          "A pointer entering a button locates its reflection in client coordinates");
    if(glass::followsPointer()){
        SendMessageW(appearanceButton,WM_MOUSEMOVE,0,MAKELPARAM(33,9));
        check(glassPointer(appearanceButton).x==33,"The light follows the pointer across one control");
    }
    const auto previousPosition=glassPointer(appearanceButton);
    const bool previousMotion=glass::motionAllowed;
    glass::motionAllowed=false;
    SendMessageW(appearanceButton,WM_MOUSEMOVE,0,MAKELPARAM(46,12));
    check(glassPointer(appearanceButton).x==previousPosition.x && !glass::followsPointer(),
          "Reduced motion keeps the hover state without tracking a moving reflection");
    glass::motionAllowed=previousMotion;
    SendMessageW(appearanceButton,WM_MOUSELEAVE,0,0);
    check(!GetPropW(appearanceButton,L"RiziaHover") && pointerButton!=appearanceButton,
          "Leaving a control clears its hover and its light source");
    const int languageBefore=i18n::language;
    for(int lang=0;lang<static_cast<int>(std::size(i18n::names));++lang){
        i18n::setLanguage(lang);
        std::set<std::wstring> names;
        for(int index=0;index<Theme::count;++index)names.insert(Theme::name(index));
        check(*Theme::label() && names.size()==Theme::count && !names.count(L""),
              "Each supported language has four distinct named appearances");
        std::set<std::wstring> finishes;
        for(auto finish:{glass::Clear,glass::Frosted,glass::Solid})finishes.insert(glass::finishLabel(finish));
        check(finishes.size()==3 && !finishes.count(L""),"Each language has three distinct material choices");
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
    applyAppearanceCommand(glass::firstFinishCommand+glass::Solid);
    glass::refreshSettings();
    check(glass::preferred==glass::Solid && !glass::enabled() && Theme::index()==Theme::fjordIndex,
          "Turning off the material persists independently of the color theme");
    applyAppearanceCommand(glass::firstFinishCommand+glass::Frosted);
    glass::refreshSettings();
    check(glass::preferred==glass::Frosted && g.session.get()==session && pending().at(budget)=="43" &&
          g.editingDirty && textOf(g.edit)==L"unfinished","Switching to frosted glass persists and preserves the active field draft");
    const wchar_t* personalization=L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    DWORD opaque=0;
    RegSetKeyValueW(HKEY_CURRENT_USER,personalization,L"EnableTransparency",REG_DWORD,&opaque,sizeof(opaque));
    glass::refreshSettings();
    check(glass::effectiveFinish()==glass::Solid && glass::preferred==glass::Frosted,
          "Windows can suppress glass without replacing the saved finish");
    RegDeleteKeyValueW(HKEY_CURRENT_USER,personalization,L"EnableTransparency");
    glass::refreshSettings();
    check(glass::effectiveFinish()==glass::Frosted || !glass::systemAllows,
          "Restoring system transparency recovers the chosen material");
    applyAppearanceCommand(glass::firstFinishCommand+glass::Clear);
    glass::refreshSettings();
    check(glass::preferred==glass::Clear,"The original clear finish can be selected again");
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
    SetWindowTextW(g.search,L"GB");
    check(g.rows==std::vector<std::string>{budget},"A favorited balance can be found by its guide abbreviation");
    SetWindowTextW(g.search,L"PW");
    check(g.rows.empty(),"An abbreviation cannot bypass the favorites filter");
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
    SetWindowTextW(g.search,L"pW");
    check(g.rows==std::vector<std::string>{"BaseGame.PersonalWealth"},"PW finds only the wealth balance on the demo economy page");
    SetWindowTextW(g.search,L"gb");
    check(g.rows==std::vector<std::string>{budget},"GB finds only the budget balance on the demo economy page");
    SetWindowTextW(g.search,L"");
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
    SetWindowTextW(g.search,L"预算 gB");
    check(g.rows==std::vector<std::string>{budget},"A mixed-language GB query finds the pending budget on the advanced page");
    SetWindowTextW(g.search,L"PW");
    check(g.rows.empty(),"An abbreviation cannot bypass the pending-only filter");
    SetWindowTextW(g.search,L"budget 预算 MAX");
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
    const std::string collision=R"({"currentStoryPack":"StoryPack_Main","variables":"Variable={['BaseGame.RumburgBorder']=1,['BaseGame.GovernmentBudget']=3}"})";
    loadDocumentBytes(collision,{},0);
    navigate(4);
    SetWindowTextW(g.search,L"GB");
    check(g.rows==std::vector<std::string>{budget,"BaseGame.RumburgBorder"},
          "Advanced search lists the balance first and keeps raw keys containing GB");
    i18n::setLanguage(0);
}

void testGuidePrefixSearch(const fs::path& path){
    const auto bytes=readFile(path);
    loadDocumentBytes(bytes,path,0);
    navigate(4);
    const std::string hugo="RiziaDLC.Relations_Hugo";
    selectRow(hugo);
    SetWindowTextW(g.edit,L"17");
    const rizia::ChangeMap expectedEdits{{hugo,"17"}};
    const std::set<std::string> relatives{
        "RiziaDLC.Relations_Estela",hugo,"RiziaDLC.Relations_Lucita_Romance",
        "RiziaDLC.Relations_Pabel","RiziaDLC.Relations_Sal","RiziaDLC.Relations_Titus","RiziaDLC.Relations_Vina"};
    for(const auto* query:{LR"([\"RiziaDLC.Relations_)",LR"(["rIzIaDlC.rELATIONS_)",LR"(['RiziaDLC.Relations_)"}) {
        SetWindowTextW(g.search,query);
        check(g.rows.size()==relatives.size() && std::set<std::string>(g.rows.begin(),g.rows.end())==relatives,
              "An open quoted guide prefix finds every matching relation in the Rizia demo");
        check(g.selected==hugo && pending()==expectedEdits,"Prefix search keeps the selected relation and its edit");
    }
    command(FILTER_CHANGED,0);
    check(g.rows==std::vector<std::string>{hugo},"The pending-only filter also applies to pasted field prefixes");
    command(FILTER_CHANGED,0);

    const std::string budget="RiziaDLC.Resources_Budget";
    SetWindowTextW(g.search,LR"(["RiziaDLC.Resources_Budget)");
    check(g.rows==std::vector<std::string>{budget,budget+"ModifiersTotal"},
          "An open prefix can match both an entire field name and its longer sibling");
    SetWindowTextW(g.search,LR"(["RiziaDLC.Resources_Budget"])");
    check(g.rows==std::vector<std::string>{budget},"Closing the quoted key excludes its longer sibling");
    for(const auto* query:{LR"(["RiziaDLC.Relations_"])",LR"([\"RiziaDLC.Relations_\"]=90)",LR"(["BaseGame.Relations_)"}) {
        SetWindowTextW(g.search,query);
        check(g.rows.empty(),"A closed partial key or an open prefix from a different campaign shows no results");
    }
    SetWindowTextW(g.search,LR"([\"RiziaDLC.Relations_Hugo\"]=-99)");
    check(g.rows==std::vector<std::string>{hugo} && pending()==expectedEdits,"A complete guide assignment finds Hugo without applying its value");
    navigate(0);
    SetWindowTextW(g.search,LR"(["RiziaDLC.Relations_)");
    check(g.rows.empty(),"A quoted relation prefix respects the economy page's field list");
    navigate(4);
    check(g.session->undo() && pending().empty() && !g.session->canUndo() && g.doc->render({})==bytes,
          "Searching and toggling filters add no undo steps and preserve the original demo");
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
    SetWindowTextW(g.search,L"GB");
    check(g.rows.empty(),"Sordland's budget abbreviation does not select the Rizia budget");
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

void testPerTurnGuideSearch(const fs::path& path) {
    const auto bytes=readFile(path);
    loadDocumentBytes(bytes,path,0);
    const std::string energy=riziaResources[2].turnTotal;
    const std::string perception="RiziaDLC.Country_GlobalPerception";
    const auto search=[](const wchar_t* query){SetWindowTextW(g.search,query);};
    for(int tab:{0,4}) {
        navigate(tab);
        for(const auto& resource:riziaResources) {
            search(resource.turnUnit);
            check(!g.rows.empty() && g.rows.front()==resource.turnTotal &&
                  std::count(g.rows.begin(),g.rows.end(),resource.turnTotal)==1,
                  "Each Rizia per-turn abbreviation ranks its saved total first exactly once");
        }
        for(const auto& [query,key]:std::initializer_list<std::pair<const wchar_t*,const char*>>{
                {L"+3Apt",riziaResources[1].turnTotal},{L"+1 Bpt",riziaResources[0].turnTotal},
                {L"-4Bpt",riziaResources[0].turnTotal},{L"+2Ept",riziaResources[2].turnTotal},
                {L"\u3000\u221212\u00a0ePt ",riziaResources[2].turnTotal}}) {
            search(query);
            check(g.rows==std::vector<std::string>{key} && g.selected==key &&
                  textOf(g.edit)==wide(g.doc->find(key)->value),
                  "Signed guide amounts locate the total and leave the editor showing the saved value");
        }
        command(FILTER_CHANGED,0);
        check(g.rows.empty(),"A signed resource query respects pending-only filtering");
        command(FILTER_CHANGED,0);
    }
    search(L" ePt ");
    check(g.rows.front()==energy && std::find(g.rows.begin(),g.rows.end(),perception)!=g.rows.end(),
          "Bare EPT keeps the ordinary Perception substring match");
    const auto favorites=g.favorites;
    g.favorites={perception};
    command(FILTER_FAVORITES,0);
    check(g.rows==std::vector<std::string>{perception},"A bare unit still filters its ranked result by favorites");
    search(L"+2EPT");
    check(g.rows.empty(),"A signed unit respects favorites and does not admit unrelated substring results");
    command(FILTER_FAVORITES,0);
    g.favorites=favorites;
    search(L"[\"RiziaDLC.Country_GlobalPerception\"]=3");
    check(g.rows==std::vector<std::string>{perception},"A pasted field assignment containing EPT still selects the raw key");
    for(auto query:{L"3Apt",L"+Apt",L"A-PT",L"+3Apt extra",L"+3Apt+1Bpt",L"+3Aptitude"}) {
        search(query);
        check(std::find(g.rows.begin(),g.rows.end(),riziaResources[1].turnTotal)==g.rows.end(),
              "Malformed or partial unit expressions cannot select the authority total");
    }
    navigate(1);
    search(L"+3Apt");
    check(g.rows.empty(),"The guide shortcut respects the current field category");
    check(pending().empty() && !g.session->canUndo() && g.doc->render({})==bytes,
          "Guide shorthand searches preserve save bytes and history");

    loadDocumentBytes(R"({"currentStoryPack":"StoryPack_Rizia","variables":"Variable={[\"RiziaDLC.Country_GlobalPerception\"]=4}"})",{},0);
    navigate(4);search(L"EPT");
    check(g.rows==std::vector<std::string>{perception},"A missing per-turn value leaves ordinary bare matches available");
    search(L"+2EPT");
    check(g.rows.empty(),"A missing total cannot be manufactured from a signed guide amount");
    loadDocumentBytes(R"({"currentStoryPack":"StoryPack_Main","variables":"Variable={[\"BaseGame.Adaptation\"]=1,[\"RiziaDLC.Resources_AuthorityModifiersTotal\"]=7}"})",{},0);
    navigate(4);search(L"APT");
    check(g.rows==std::vector<std::string>{"BaseGame.Adaptation"},"Rizia unit shortcuts do not apply to a Sordland campaign");
    search(L"+3Apt");
    check(g.rows.empty(),"A signed guide amount does not expose a foreign-campaign total");
}

void testEpaGuideSearch(const fs::path& mainFixture) {
    i18n::setLanguage(0);
    const auto bytes=readFile(mainFixture);
    loadDocumentBytes(bytes,mainFixture,0);
    navigate(4);
    const auto search=[](const wchar_t* query){SetWindowTextW(g.search,query);};
    const std::string policy="BaseGame.Policy_Economy_EnergyProtectionAct_";
    const std::string turn="BaseGame.Turn03_Admin_EnergyProtectionAct_";
    const std::string promise="BaseGame.Relations_Ewald_PromiseNotHarsherEPA";
    const std::vector<std::string> family={policy+"10Percent",policy+"49Percent",policy+"Default",policy+"Removed",
                                          turn+"10Percent",turn+"49Percent",turn+"Keep20Percent",turn+"Removed"};
    const auto hasFamily=[&]{
        return std::any_of(family.begin(),family.end(),[](const auto& key){
            return std::find(g.rows.begin(),g.rows.end(),key)!=g.rows.end();
        });
    };
    search(L" ePa ");
    check(g.rows.size()>=family.size() && std::equal(family.begin(),family.end(),g.rows.begin()) &&
          std::set<std::string>(g.rows.begin(),g.rows.end()).size()==g.rows.size(),
          "EPA ranks each saved policy and turn field once before ordinary matches");
    check(std::find(g.rows.begin(),g.rows.end(),promise)!=g.rows.end() &&
          std::find(g.rows.begin(),g.rows.end(),"BaseGame.Enable_Preparations")!=g.rows.end(),
          "EPA preserves the promise key and existing substring matches");
    for(const auto& [query,expected]:std::vector<std::pair<const wchar_t*,std::vector<std::string>>>{
            {L"EPA 10",{policy+"10Percent",turn+"10Percent"}},{L"EPA 10%",{policy+"10Percent",turn+"10Percent"}},
            {L"EPA 49%",{policy+"49Percent",turn+"49Percent"}},{L"49\uFF05\u3000epa",{policy+"49Percent",turn+"49Percent"}},
            {L"20% EPA",{turn+"Keep20Percent"}},{L"EPA default",{policy+"Default"}},
            {L"EPA repeal",{policy+"Removed",turn+"Removed"}},{L"EPA removed",{policy+"Removed",turn+"Removed"}}}) {
        search(query);
        check(g.rows==expected,"Qualified EPA searches match only the corresponding saved choices");
    }
    for(const auto query:{L"EPA 9%",L"EPA 3",L"EPA 100%",L"EPA 49% repeal",L"Episode",L"Energy EPAx"}) {
        search(query);
        check(!hasFamily(),"Partial percentages, turn numbers, conflicting choices and longer tokens do not gain an EPA alias");
    }
    search(L"EnergyProtectionAct");
    check(g.rows==family,"The full saved field family is still searchable in normal order");
    search(L"[\"BaseGame.Turn03_Admin_EnergyProtectionAct_49Percent\"]=true");
    check(g.rows==std::vector<std::string>{turn+"49Percent"},"Pasting a single EPA assignment still selects its exact key");
    const auto oldFavorites=g.favorites;
    g.favorites={promise,policy+"49Percent"};
    search(L"EPA");
    command(FILTER_FAVORITES,0);
    check(g.rows==std::vector<std::string>{policy+"49Percent",promise},"EPA favorites include the ranked field and the literal result");
    command(FILTER_FAVORITES,0);
    g.favorites=oldFavorites;
    command(FILTER_CHANGED,0);
    check(g.rows.empty(),"EPA respects the pending-only view");
    command(FILTER_CHANGED,0);
    navigate(0);search(L"EPA");
    check(!hasFamily(),"EPA does not expose advanced policies on the resources tab");
    check(pending().empty() && !g.session->canUndo() && g.doc->render({})==bytes,"EPA searches preserve source bytes and edit history");

    const std::string variables=R"(","variables":"Variable={[\"BaseGame.Policy_Economy_EnergyProtectionAct_Removed\"]=true,[\"BaseGame.Relations_Ewald_PromiseNotHarsherEPA\"]=false,[\"RiziaDLC.Pales_Rizia_PreparationsUnification\"]=false}"})";
    for(const bool sordland:{true,false}) {
        const auto story=sordland?"StoryPack_Main":"StoryPack_Rizia";
        loadDocumentBytes(std::string(R"({"currentStoryPack":")")+story+variables,{},0);
        navigate(4);search(L"EPA");
        const std::vector<std::string> expected=sordland?std::vector<std::string>{policy+"Removed",promise}:
                                                 std::vector<std::string>{"RiziaDLC.Pales_Rizia_PreparationsUnification"};
        check(g.rows==expected,"Aliases only rank fields present in Sordland, retaining raw matches in mixed campaign data");
        search(L"EPA 49%");
        check(g.rows.empty(),"Missing EPA fields cannot be created by a percentage query");
    }
}

void testPublicOpinionSearch() {
    i18n::setLanguage(0);
    for (const bool sordland : {true, false}) {
        const auto story = sordland ? "StoryPack_Main" : "StoryPack_Rizia";
        const std::string key = sordland ? "BaseGame.Public_Opinion" : "RiziaDLC.Country_PublicOpinion";
        const std::string bytes = std::string(R"({"currentStoryPack":")") + story +
            R"(","variables":"Variable={[\"BaseGame.Public_Opinion\"]=9,[\"BaseGame.Bludish_Opinion\"]=17,)"
            R"([\"RiziaDLC.Country_PublicOpinion\"]=8,[\"RiziaDLC.Country_DastnuristOpinion\"]=12,)"
            R"([\"RiziaDLC.Resources_Military_Manpower\"]=4,[\"RiziaDLC.Decision_Floating_DastnuristApproval_Happened\"]=true}"})";
        loadDocumentBytes(bytes, {}, 0);
        for (const int tab : {sordland ? 1 : 2, 4}) {
            navigate(tab);
            for (const auto query : {L"民意",L"支持率",L"民调",L"民調",L"public approval",L"公众支持度",L"public opinion"}) {
                SetWindowTextW(g.search, query);
                check(g.rows == std::vector<std::string>{key} && g.selected == key,
                      "Public opinion terms select the active campaign's overall score");
                check(textOf(g.name) == L"公众支持度" && textOf(g.desc).find(L"不是百分比") != std::wstring::npos,
                      "Polling aliases preserve the score label and its units");
            }
            SetWindowTextW(g.search, wide(key).c_str());
            check(g.rows == std::vector<std::string>{key}, "The raw public opinion key remains searchable");
            SetWindowTextW(g.search, L" pO ");
            check(!g.rows.empty() && g.rows.front() == key && std::count(g.rows.begin(),g.rows.end(),key) == 1,
                  "The PO abbreviation shows the active campaign score first, once");
        }
        if (!sordland) {
            const std::string manpower = "RiziaDLC.Resources_Military_Manpower";
            check(g.rows == std::vector<std::string>{key,manpower}, "PO keeps unrelated substring results available");
            selectRow(manpower);
            refresh();
            check(g.selected == manpower, "A player's chosen substring result keeps its selection");
            command(FILTER_CHANGED,0);
            check(g.rows.empty(), "The PO shortcut respects pending-only filtering");
            command(FILTER_CHANGED,0);
            const auto oldFavorites = g.favorites;
            g.favorites = {manpower};
            command(FILTER_FAVORITES,0);
            check(g.rows == std::vector<std::string>{manpower}, "The PO shortcut respects favorites-only filtering");
            command(FILTER_FAVORITES,0);
            g.favorites = oldFavorites;
            SetWindowTextW(g.search, L"P-O");
            check(g.rows.front() == manpower, "Only a complete PO query changes the normal substring order");
        }
        check(g.doc->render({}) == bytes && pending().empty(), "Searching opinion scores leaves the save unchanged");
    }
    loadDocumentBytes(R"({"currentStoryPack":"StoryPack_Rizia","variables":"Variable={[\"RiziaDLC.Resources_Military_Manpower\"]=4}"})",{},0);
    navigate(4);
    SetWindowTextW(g.search,L"PO");
    check(g.rows == std::vector<std::string>{"RiziaDLC.Resources_Military_Manpower"},
          "Without a public opinion field, PO still shows ordinary matches");
}

void testReviewCsv(const fs::path& riziaPath,const fs::path& mainPath){
    const auto riziaBytes=readFile(riziaPath);
    loadDocumentBytes(riziaBytes,riziaPath,0);
    const auto path=fs::temp_directory_path()/(L"suzerain-review-"+writeId()+L".csv");
    auto exportCsv=[&]{
        const auto staged=pending();
        const bool undo=g.session->canUndo(),redo=g.session->canRedo();
        writeFieldCsv(path);
        const auto bytes=readFile(path);
        fs::remove(path);
        check(pending()==staged&&g.session->canUndo()==undo&&g.session->canRedo()==redo,
              "Export preserves staged fields and undo/redo availability");
        return bytes;
    };
    auto expectCsv=[&](const rizia::ChangeMap& expected,const char* reason){
        const auto csv=exportCsv();
        check(csv==renderFieldCsv(*g.doc,expected),reason);
    };

    g.autoCap=true;
    const auto unchanged=exportCsv();
    preset(99);
    auto linked=pending();
    for(const auto& resource:riziaResources)linked[resource.cap]="99";
    expectCsv(linked,"CSV includes all three original cap values and their linked current values");
    check(g.session->undo()&&pending().empty(),"One undo removes the stock preset after export");
    check(exportCsv()==unchanged,"Undo removes derived values from the exported CSV");
    check(g.session->redo(),"Export keeps the original preset available for redo");

    const auto budgetCap=riziaResources[0].cap;
    g.session->applyBatch({{budgetCap,"120"}});
    linked[budgetCap]="120";
    expectCsv(linked,"A sufficient manual cap overrides automatic synchronization in the CSV");
    g.session->applyBatch({{budgetCap,"60"}});
    expectCsv(pending(),"A conflicting manual cap exports only explicit changes");
    g.session->revertField(budgetCap);
    g.autoCap=false;
    expectCsv(pending(),"Disabled synchronization exports only explicit stock edits");
    g.autoCap=true;
    check(readFile(riziaPath)==riziaBytes,"Export keeps the Rizia save byte-for-byte intact");

    const auto mainBytes=readFile(mainPath);
    loadDocumentBytes(mainBytes,mainPath,0);
    const auto mainCsv=exportCsv();
    check(mainCsv==renderFieldCsv(*g.doc,{})&&mainCsv.find("RiziaDLC.")==std::string::npos,
          "Rizia linked values do not leak into a new Sordland session");
    check(readFile(mainPath)==mainBytes,"Export keeps the Sordland save byte-for-byte intact");

    const std::string customKey=u8"BaseGame.汉字Mod🦊,\"Q\"";
    const std::string customSave=u8R"({"currentStoryPack":"StoryPack_Main","variables":"Variable={['BaseGame.汉字Mod🦊,\"Q\"']=false}"})";
    const int originalLanguage=i18n::language;
    for(int language:{0,1}){
        i18n::setLanguage(language);
        loadDocumentBytes(customSave,{},0);
        navigate(4);
        SetWindowTextW(g.search,L"汉字Mod🦊");
        check(g.rows==std::vector<std::string>{customKey},"The original Unicode field name is searchable in both Chinese interfaces");
        SetWindowTextW(g.search,LR"(['BaseGame.汉字Mod🦊,"Q"']=true)");
        check(g.rows==std::vector<std::string>{customKey} && pending().empty(),
              "Pasted custom keys preserve Unicode and quoted punctuation without applying their value");
        SetWindowTextW(g.search,LR"([\"BaseGame.汉字Mod🦊)");
        check(g.rows==std::vector<std::string>{customKey},"An escaped guide prefix can contain multiple scripts");
        g.session->applyBatch({{customKey,"true"}});
        const std::string prefix=u8"\"StoryPack_Main\",\"BaseGame.汉字Mod🦊,\"\"Q\"\"\",\"";
        const std::string caption=language==0?u8"汉字Mod🦊":u8"漢字Mod🦊";
        check(exportCsv().find(prefix+caption+",\"\"Q\"\"\",\"false\",\"true\",\"boolean\"\r\n")!=std::string::npos,
              "Chinese field CSV preserves Unicode keys, localized labels, quotes and boolean edits");
    }
    i18n::setLanguage(originalLanguage);
}

void testReviewShortcut(const fs::path& sample) {
    const auto source=readFile(sample);
    loadDocumentBytes(source,sample,0);
    navigate(0);
    SetFocus(g.search);
    check(handleEditorShortcut('S',GetFocus()) && g.tab==5 && g.rows.empty() &&
          GetFocus()==ctl(NAV_BASE+5),"An empty review focuses its navigation button");

    navigate(0);
    const std::string budget="BaseGame.GovernmentBudget";
    selectRow(budget);
    SetFocus(g.edit);
    SetWindowTextW(g.edit,L"invalid");
    const auto timer=SetTimer(nullptr,0,50,dismissValidationWarning);
    check(timer!=0,"Install warning handler for review validation");
    handleEditorShortcut('S',GetFocus());
    KillTimer(nullptr,timer);
    check(g.tab==0 && GetFocus()==g.edit && textOf(g.edit)==L"invalid" && g.editingDirty &&
          pending().empty(),"An invalid value stays in its editor when review is requested");

    g.session->applyBatch({{"BaseGame.PersonalWealth","12"}});
    SetWindowTextW(g.edit,L"31");
    check(handleEditorShortcut('S',GetFocus()) && g.tab==5 && GetFocus()==g.grid &&
          g.rows.size()==2 && g.review.at(budget)=="31","Ctrl+S stages the draft and focuses the review rows");
    const auto selected=g.selected;
    const auto edits=pending();
    SendMessageW(GetFocus(),WM_KEYDOWN,selected==g.rows.front()?VK_DOWN:VK_UP,1);
    check(g.selected!=selected && GetFocus()==g.grid && pending()==edits && readFile(sample)==source,
          "An arrow immediately inspects the other change without altering the save or pending values");
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
        Gdiplus::GdiplusStartupInput graphicsInput;
        ULONG_PTR token{};
        check(Gdiplus::GdiplusStartup(&token,&graphicsInput,nullptr)==Gdiplus::Ok,"Initialize GDI+ for native editor painting");
        struct GraphicsShutdown { ULONG_PTR token; ~GraphicsShutdown(){Gdiplus::GdiplusShutdown(token);} } graphics{token};
        testInterfaceIcons();
        INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_WIN95_CLASSES};
        InitCommonControlsEx(&controls);
        g.bg=CreateSolidBrush(Theme::bg);g.panel=CreateSolidBrush(Theme::panel);g.input=CreateSolidBrush(Theme::input);
        fonts();
        WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"SuzerainEditorRegression";
        check(RegisterClassW(&wc)!=0,"Register test editor");
        const HWND window=CreateWindowW(wc.lpszClassName,L"",WS_OVERLAPPEDWINDOW,0,0,1120,740,nullptr,nullptr,wc.hInstance,nullptr);
        check(window!=nullptr&&g.grid,"Create hidden editor");
        testChoiceHintsPreference();
        testBackupRefreshAndRestore(fs::u8path(argv[1]));
        testBackupSaveAs();
        testReloadView();
        testSaveQueries();
        testPickerSearchWhileScanning(fs::u8path(argv[1]));
        testPickerCurrentSave(fs::u8path(argv[1]));
        testSearchControls(fs::u8path(argv[1]));
        testGuideKeyboard();
        testFieldDetailKeyboard(fs::u8path(argv[1]));
        testFieldActivation(fs::u8path(argv[1]));
        testHistoryShortcuts(fs::u8path(argv[1]));
        testSearch(fs::u8path(argv[1]));
        testGuidePrefixSearch(fs::u8path(argv[2]));
        testResourceSearch(fs::u8path(argv[2]));
        testPerTurnGuideSearch(fs::u8path(argv[2]));
        testEpaGuideSearch(fs::u8path(argv[1]).parent_path()/"fixture_all_main.json");
        testPublicOpinionSearch();
        testReviewShortcut(fs::u8path(argv[1]));
        testReview(fs::u8path(argv[2]));
        testReviewCsv(fs::u8path(argv[2]),fs::u8path(argv[1]));
        testAppearance(fs::u8path(argv[1]));
        testFavorites(fs::u8path(argv[1]),fs::u8path(argv[2]));
        testInspectorLayout(fs::u8path(argv[2]));
        testChineseFonts(fs::u8path(argv[1]));
        testTypography(fs::u8path(argv[2]));
        testSponsorDialog();
        DestroyWindow(window);
        std::cout<<"PASS: "<<checks<<" native editor checks\n";
        return 0;
    }catch(const std::exception& error){
        std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';
        return 1;
    }
}
