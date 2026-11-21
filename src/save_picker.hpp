#pragma once
#include <atomic>
#include <thread>
#include <shobjidl.h>
#include "save_discovery.hpp"

// The picker owns its controls and fonts. Discovery never touches the editor or files.
namespace save_picker {
enum { LIST=410, FILTER, ADD_FOLDER, RESCAN, STATUS, PATH, CAMPAIGN_FILTER, BROWSE, DEMO_MAIN, DEMO_RIZIA, LIVE_MODE, APPEARANCE, CLEAR_SEARCH, START_SCAN=WM_APP+44 };
inline std::vector<fs::path> extraRoots;
struct State {
    HWND win{}, list{}, filter{}, clearSearch{}, campaign{}, status{}, path{}, add{}, refresh{}, open{}, cancelButton{}, heading{};
    HWND browse{}, live{}, appearance{}, practiceTitle{}, practiceHint{}, mainDemo{}, riziaDemo{}, empty{};
    UINT dpi=96;
    HFONT font{}, small{}, bold{}, subheading{}, emphasis{};
    HIMAGELIST rowHeight{};
    std::vector<fs::path> roots;
    rizia::discovery::Result result;
    std::vector<size_t> rows;
    fs::path choice;
    fs::path beforeScan;
    int chosenDemo=0, appearanceIndex=-1;
    bool stacked=false;
    RECT library{}, practice{}, searchFrame{}, headingIcon{};
    int footerY=0;
    std::thread worker;
    std::atomic_bool cancel{false}, done{false};
    bool busy=false, populating=false;
    std::wstring error;
    ~State(){ stop(); if(rowHeight)ImageList_Destroy(rowHeight); for(auto f:{font,small,bold,subheading,emphasis})if(f)DeleteObject(f); }
    void stop(){cancel.store(true);if(worker.joinable())worker.join();busy=false;}
    int scale(int n)const{return MulDiv(n,dpi,96);}
    void move(HWND h,int x,int y,int w,int he){MoveWindow(h,scale(x),scale(y),scale(std::max(1,w)),scale(std::max(1,he)),TRUE);}
    RECT box(int x,int y,int w,int h)const{return {scale(x),scale(y),scale(x+w),scale(y+h)};}
};
inline State* state(HWND w){return reinterpret_cast<State*>(GetWindowLongPtrW(w,DWLP_USER));}

inline HWND child(State& s,const wchar_t* cls,const wchar_t* text,DWORD style,int id,DWORD ex=0){
    auto h=CreateWindowExW(ex,cls,tr(text).c_str(),WS_CHILD|WS_VISIBLE|style,0,0,0,0,s.win,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
    if(!h)throw WinFailure(L"无法创建存档列表："+winError());
    SendMessageW(h,WM_SETFONT,(WPARAM)s.font,TRUE);return h;
}
inline void setFonts(State& s){
    auto make=[&](int size,int weight,const wchar_t* face=nullptr){return CreateFontW(-s.scale(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,face?face:Theme::bodyFace());};
    HFONT old[]={s.font,s.small,s.bold,s.subheading,s.emphasis};
    s.font=make(14,FW_NORMAL);s.small=make(12,FW_NORMAL);s.bold=make(32,FW_NORMAL,Theme::headingFace());
    s.subheading=make(21,FW_NORMAL,Theme::headingFace());s.emphasis=make(14,FW_SEMIBOLD);
    EnumChildWindows(s.win,[](HWND h,LPARAM p){auto&s=*reinterpret_cast<State*>(p);SendMessageW(h,WM_SETFONT,(WPARAM)s.font,TRUE);return TRUE;},(LPARAM)&s);
    for(auto h:{s.status,s.path,s.practiceHint})if(h)SendMessageW(h,WM_SETFONT,(WPARAM)s.small,TRUE);
    if(s.heading)SendMessageW(s.heading,WM_SETFONT,(WPARAM)s.bold,TRUE);
    if(s.practiceTitle)SendMessageW(s.practiceTitle,WM_SETFONT,(WPARAM)s.subheading,TRUE);
    if(s.campaign){
        SendMessageW(s.campaign,CB_SETITEMHEIGHT,static_cast<WPARAM>(-1),s.scale(23));
        SendMessageW(s.campaign,CB_SETITEMHEIGHT,0,s.scale(26));
    }
    for(auto f:old)if(f)DeleteObject(f);
    if(s.list){auto oldImage=s.rowHeight;s.rowHeight=ImageList_Create(1,s.scale(47),ILC_COLOR32,1,1);ListView_SetImageList(s.list,s.rowHeight,LVSIL_SMALL);if(oldImage)ImageList_Destroy(oldImage);}
}
inline void layoutSearch(State& s){
    const bool showClear=!s.busy && GetWindowTextLengthW(s.filter)>0;
    RECT input=s.searchFrame;
    input.left+=s.scale(29);input.top+=s.scale(4);input.right-=s.scale(showClear?30:7);
    MoveWindow(s.filter,input.left,input.top,std::max<LONG>(1,input.right-input.left),s.scale(22),TRUE);
    MoveWindow(s.clearSearch,s.searchFrame.right-s.scale(28),s.searchFrame.top+s.scale(3),s.scale(24),s.scale(24),TRUE);
    visible(s.clearSearch,showClear);
}
inline void layout(State& s){
    if(!s.empty)return;
    RECT r{};GetClientRect(s.win,&r);
    const int w=MulDiv(r.right,96,s.dpi), h=MulDiv(r.bottom,96,s.dpi);
    const bool compact=h<470;
    s.stacked=w<830;
    const int margin=s.stacked?16:26;
    const int inner=w-2*margin, top=compact?7:17, headerH=compact?43:55;
    const int bodyY=top+headerH+6;
    const int gap=s.stacked?8:20, rail=std::clamp(inner/4,206,270);
    const int cardW=s.stacked?inner:inner-rail-gap;
    const int pad=s.stacked?10:14;
    s.footerY=h-(compact?48:62);
    const int bodyBottom=s.footerY-(compact?7:12);
    const int cardBottom=s.stacked?bodyBottom-(compact?97:104)-gap:bodyBottom;
    const int tableInset=s.stacked?5:9;
    const int tableX=margin+tableInset, tableTop=bodyY+(compact?45:55);
    const int tableWidth=cardW-2*tableInset, tableBottom=cardBottom-(compact?38:46);
    const int campaignW=std::clamp(cardW/3,120,165);
    const int filterX=margin+pad, filterY=bodyY+(compact?7:12);
    const int filterW=cardW-pad*2-campaignW-8;
    s.library=s.box(margin,bodyY,cardW,cardBottom-bodyY);
    s.searchFrame=s.box(filterX,filterY,filterW,30);
    const bool narrowHeader=w<660;
    const int headingX=margin+(narrowHeader?0:47);
    const int browseWidth=narrowHeader?103:143;
    const int appearanceWidth=narrowHeader?138:179;
    const int browseX=w-margin-browseWidth;
    const int appearanceX=browseX-8-appearanceWidth;
    const int actionY=top+(compact?3:8);
    s.headingIcon=narrowHeader?RECT{}:s.box(margin,top+(compact?4:8),35,35);
    SendMessageW(s.heading,WM_SETFONT,(WPARAM)(narrowHeader?s.subheading:s.bold),FALSE);
    const int headingOffset=narrowHeader?8:0;
    s.move(s.heading,headingX,top+headingOffset,appearanceX-headingX-9,headerH-headingOffset);
    s.move(s.appearance,appearanceX,actionY,appearanceWidth,35);
    s.move(s.browse,browseX,actionY,browseWidth,35);
    layoutSearch(s);
    s.move(s.campaign,filterX+filterW+8,filterY+1,campaignW,210);
    s.move(s.list,tableX,tableTop,tableWidth,std::max(28,tableBottom-tableTop));
    s.move(s.empty,tableX,tableTop,tableWidth,std::max(28,tableBottom-tableTop));
    const int toolsY=tableBottom+(compact?2:6);
    const int addW=s.stacked?121:141, refreshW=s.stacked?95:108;
    s.move(s.add,margin+pad,toolsY,addW,31);
    s.move(s.refresh,margin+pad+addW+6,toolsY,refreshW,31);
    int statusX=margin+pad+addW+refreshW+20;
    s.move(s.status,statusX,toolsY+6,margin+cardW-pad-statusX,22);
    auto hint=tr(L"可体验编辑功能，无法在游戏中载入。");
    if(!s.stacked&&i18n::language<=1){
        auto split=hint.find(L'，');
        if(split!=std::wstring::npos)hint.insert(split+1,L"\r\n");
    }
    if(textOf(s.practiceHint)!=hint)rawText(s.practiceHint,hint);
    if(s.stacked){
        const int y=cardBottom+gap;
        const int liveWidth=std::clamp(inner/4,118,160);
        const int practiceW=inner-liveWidth-gap;
        s.practice=s.box(margin,y,practiceW,bodyBottom-y);
        s.move(s.practiceTitle,margin+pad,y+3,practiceW-pad*2,26);
        s.move(s.practiceHint,margin+pad,y+29,practiceW-pad*2,32);
        const int itemWidth=(practiceW-pad*2-6)/2;
        const int by=y+62, bh=compact?29:34;
        s.move(s.mainDemo,margin+pad,by,itemWidth,bh);
        s.move(s.riziaDemo,margin+pad+itemWidth+6,by,itemWidth,bh);
        s.move(s.live,margin+practiceW+gap,y,liveWidth,bodyBottom-y);
    }else{
        const int x=margin+cardW+gap, step=compact?37:46;
        const int buttonY=bodyY+(compact?77:93);
        const int boxH=buttonY-bodyY+step*2+13;
        s.practice=s.box(x,bodyY,rail,boxH);
        s.move(s.practiceTitle,x+11,bodyY+6,rail-22,29);
        s.move(s.practiceHint,x+11,bodyY+39,rail-22,buttonY-bodyY-39);
        s.move(s.mainDemo,x+3,buttonY,rail-6,step);
        s.move(s.riziaDemo,x+3,buttonY+step,rail-6,step);
        s.move(s.live,x+3,bodyY+boxH+7,rail-6,compact?45:54);
    }
    const int footerButtons=s.footerY+(compact?6:12);
    HDC dc=GetDC(s.win);
    auto previousFont=SelectObject(dc,s.emphasis);
    const auto openCaption=textOf(s.open);SIZE extent{};
    GetTextExtentPoint32W(dc,openCaption.c_str(),static_cast<int>(openCaption.size()),&extent);
    SelectObject(dc,previousFont);ReleaseDC(s.win,dc);
    const int openWidth=std::clamp(MulDiv(extent.cx,96,s.dpi)+44,141,210);
    const int openX=w-margin-openWidth, cancelX=openX-108;
    s.move(s.path,margin,footerButtons+6,cancelX-margin-12,23);
    s.move(s.cancelButton,cancelX,footerButtons,100,34);
    s.move(s.open,openX,footerButtons,openWidth,34);
    const int columns=tableWidth-20;
    // Slot type and campaign name share the second line and a native subitem.
    int widths[]={0,columns>=510?90:84,0,columns>=510?56:48,columns>=510?143:110};
    widths[0]=std::max(108,columns-widths[1]-widths[2]-widths[3]-widths[4]);
    for(int i=0;i<5;++i)ListView_SetColumnWidth(s.list,i,s.scale(widths[i]));
    InvalidateRect(s.win,nullptr,FALSE);
}
inline int selected(const State& s){return ListView_GetNextItem(s.list,-1,LVNI_SELECTED);}
inline void selection(State& s){
    int i=selected(s);bool valid=!s.busy&&i>=0&&i<(int)s.rows.size();
    EnableWindow(s.open,valid);
    rawText(s.path,valid?s.result.saves[s.rows[i]].path.wstring():L"");
}
#include "save_query.hpp"
inline void fill(State& s){
    fs::path keep=std::move(s.beforeScan);s.beforeScan.clear();
    int current=selected(s);if(current>=0&&current<(int)s.rows.size())keep=s.result.saves[s.rows[current]].path;
    s.populating=true;SendMessageW(s.list,WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(s.list);s.rows.clear();
    const auto query=parseSaveQuery(textOf(s.filter));
    int at=-1;auto campaign=(int)SendMessageW(s.campaign,CB_GETCURSEL,0,0);
    for(size_t n=0;n<s.result.saves.size();++n){const auto& save=s.result.saves[n];
        if((campaign==1&&save.story!="StoryPack_Main")||(campaign==2&&save.story!="StoryPack_Rizia"))continue;
        auto name=save.path.filename().wstring();
        const auto* slot=saveSlot(name);
        if(!matchesSave(save,query,slot))continue;
        int row=(int)s.rows.size();s.rows.push_back(n);
        LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=row;item.pszText=name.data();ListView_InsertItem(s.list,&item);
        std::wstring cells[]={campaignLabel(save.story),saveSubtitle(slot,wide(save.campaignName)),wide(save.turn),fileTimeText(save.path)};
        for(int c=0;c<4;++c)ListView_SetItemText(s.list,row,c+1,cells[c].data());
        if(save.path==keep)at=row;
    }
    if(at<0&&!s.rows.empty())at=0;
    if(at>=0){ListView_SetItemState(s.list,at,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);ListView_EnsureVisible(s.list,at,FALSE);}
    SendMessageW(s.list,WM_SETREDRAW,TRUE,0);InvalidateRect(s.list,nullptr,TRUE);s.populating=false;selection(s);
    std::wstring status;
    if(s.rows.size()!=s.result.saves.size() || s.rows.empty())status=std::to_wstring(s.rows.size())+L" / "+std::to_wstring(s.result.saves.size());
    else status=std::to_wstring(s.rows.size())+L" 个存档 · 最新在前";
    if(s.result.truncated)status+=L" · 已到搜索上限";
    else if(s.result.errors)status+=L" · 部分文件无法读取";
    setText(s.status,status);
    setText(s.empty,!s.error.empty()?s.error:s.result.saves.empty()?L"未找到存档，请添加文件夹。":L"没有匹配的存档");
    ShowWindow(s.list,s.rows.empty()?SW_HIDE:SW_SHOW);
    ShowWindow(s.empty,s.rows.empty()?SW_SHOW:SW_HIDE);
    layoutSearch(s);
}
inline void start(State& s){
    if(s.busy)return;
    int current=selected(s);
    if(current>=0&&current<(int)s.rows.size())s.beforeScan=s.result.saves[s.rows[current]].path;
    s.stop();s.cancel.store(false);s.done.store(false);s.error.clear();s.result={};s.rows.clear();
    s.populating=true;ListView_DeleteAllItems(s.list);s.populating=false;s.busy=true;
    layoutSearch(s);
    setText(s.status,L"");setText(s.path,L"");setText(s.empty,L"正在搜索…");
    ShowWindow(s.list,SW_HIDE);ShowWindow(s.empty,SW_SHOW);
    for(auto h:{s.open,s.refresh,s.add,s.filter,s.campaign})EnableWindow(h,FALSE);
    // Captured roots cannot change until the worker finishes; all UI updates stay on the UI thread.
    auto roots=s.roots;
    s.worker=std::thread([&s,roots]{
        try{rizia::discovery::Limits limits;limits.cancel=&s.cancel;s.result=rizia::discovery::scan(roots,limits);}
        catch(const std::exception&){s.error=L"搜索失败，请刷新或添加文件夹";}
        catch(...){s.error=L"搜索失败，请刷新或添加文件夹";}
        s.done.store(true,std::memory_order_release);
    });
    if(!SetTimer(s.win,1,100,nullptr)){s.stop();throw WinFailure(L"无法启动存档搜索。");}
}
inline fs::path folder(HWND owner){
    IFileOpenDialog* dialog=nullptr;fs::path result;
    HRESULT hr=CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog));
    if(FAILED(hr))throw WinFailure(L"无法打开文件夹选择窗口。");
    struct Release {IFileOpenDialog*p;~Release(){p->Release();}} cleanup{dialog};
    DWORD flags=0;dialog->GetOptions(&flags);dialog->SetOptions(flags|FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST);
    auto title=tr(L"选择存档文件夹");dialog->SetTitle(title.c_str());hr=dialog->Show(owner);
    if(SUCCEEDED(hr)){IShellItem* item=nullptr;if(SUCCEEDED(dialog->GetResult(&item))){PWSTR p=nullptr;if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&p))){try{result=p;}catch(...){CoTaskMemFree(p);item->Release();throw;}CoTaskMemFree(p);}item->Release();}}
    else if(hr!=HRESULT_FROM_WIN32(ERROR_CANCELLED))throw WinFailure(L"无法选择这个文件夹。");
    return result;
}
inline void accept(State& s){
    if(s.busy)return;int i=selected(s);if(i<0||i>=(int)s.rows.size())return;
    s.choice=s.result.saves[s.rows[i]].path;EndDialog(s.win,IDOK);
}
#include "picker_design.hpp"
inline void applyPickerAppearance(State& s) {
    s.appearanceIndex=Theme::index();
    Theme::caption(s.win);
    SetWindowTheme(s.list,Theme::dark()?L"DarkMode_Explorer":L"Explorer",nullptr);
    ListView_SetBkColor(s.list,Theme::panel);
    ListView_SetTextBkColor(s.list,Theme::panel);
    ListView_SetTextColor(s.list,Theme::text);
    rawText(s.appearance,std::wstring(Theme::label())+L" · "+Theme::name(Theme::index()));
    setFonts(s);
    layout(s);
    RedrawWindow(s.win,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN);
}
inline INT_PTR CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
    if(themeMenuMessage(m,wp,lp))return TRUE;
    State* p=state(w);
    try{
        if(m==WM_INITDIALOG){p=reinterpret_cast<State*>(lp);SetWindowLongPtrW(w,DWLP_USER,(LONG_PTR)p);auto&s=*p;s.win=w;SetWindowTextW(w,APP_TITLE);s.dpi=GetDpiForWindow(w);if(!s.dpi)s.dpi=96;setFonts(s);
            s.heading=child(s,L"STATIC",L"选择存档",SS_ENDELLIPSIS,0);
            s.filter=child(s,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,FILTER);
            s.clearSearch=child(s,L"BUTTON",L"清除搜索",WS_TABSTOP|BS_OWNERDRAW,CLEAR_SEARCH);
            SetWindowSubclass(s.clearSearch,buttonProc,1,0);
            s.campaign=child(s,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL,CAMPAIGN_FILTER);
            SetWindowSubclass(s.campaign,campaignFaceProc,1,(DWORD_PTR)&s);
            for(const auto* name:{L"All campaigns",L"Sordland",L"Rizia"}){auto text=tr(name);SendMessageW(s.campaign,CB_ADDSTRING,0,(LPARAM)text.c_str());}SendMessageW(s.campaign,CB_SETCURSEL,0,0);
            s.add=child(s,L"BUTTON",L"添加文件夹",WS_TABSTOP|BS_OWNERDRAW,ADD_FOLDER);s.refresh=child(s,L"BUTTON",L"刷新",WS_TABSTOP|BS_OWNERDRAW,RESCAN);
            s.list=child(s,WC_LISTVIEWW,L"存档列表",WS_TABSTOP|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS,LIST);
            configureSearchField(s.filter,s.list,L"搜索文件名、战役或回合");
            ListView_SetExtendedListViewStyle(s.list,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_LABELTIP);
            const wchar_t* cols[]={L"文件名",L"战役",L"存档详情",L"回合",L"修改时间"};for(int i=0;i<5;++i){LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;auto title=tr(cols[i]);c.pszText=title.data();c.cx=s.scale(120);ListView_InsertColumn(s.list,i,&c);}
            SetWindowSubclass(ListView_GetHeader(s.list),header,1,(DWORD_PTR)&s);
            s.path=child(s,L"STATIC",L"",SS_PATHELLIPSIS|SS_NOPREFIX,PATH);
            s.status=child(s,L"STATIC",L"正在搜索…",SS_RIGHT|SS_ENDELLIPSIS,STATUS);
            s.practiceTitle=child(s,L"STATIC",L"内置练习示例",SS_ENDELLIPSIS,0);
            s.practiceHint=child(s,L"STATIC",L"可体验编辑功能，无法在游戏中载入。",0,0);
            s.mainDemo=child(s,L"BUTTON",L"索德兰示例",WS_TABSTOP|BS_OWNERDRAW,DEMO_MAIN);
            s.riziaDemo=child(s,L"BUTTON",L"里奇亚示例",WS_TABSTOP|BS_OWNERDRAW,DEMO_RIZIA);
            s.browse=child(s,L"BUTTON",L"手动打开",WS_TABSTOP|BS_OWNERDRAW,BROWSE);
            s.appearance=child(s,L"BUTTON",L"",WS_TABSTOP|BS_OWNERDRAW,APPEARANCE);
            s.live=child(s,L"BUTTON",L"实时模式",WS_TABSTOP|BS_OWNERDRAW,LIVE_MODE);
            s.open=child(s,L"BUTTON",L"打开所选",WS_TABSTOP|BS_OWNERDRAW,IDOK);s.cancelButton=child(s,L"BUTTON",L"取消",WS_TABSTOP|BS_OWNERDRAW,IDCANCEL);
            s.empty=child(s,L"STATIC",L"正在搜索…",SS_OWNERDRAW,0);
            for(auto button:{s.add,s.refresh,s.mainDemo,s.riziaDemo,s.appearance,s.browse,s.live,s.open,s.cancelButton})SetWindowSubclass(button,buttonProc,1,0);
            applyPickerAppearance(s);RECT owner{},area{};GetWindowRect(g.win,&owner);MONITORINFO mi{};mi.cbSize=sizeof(mi);GetMonitorInfoW(MonitorFromWindow(g.win,MONITOR_DEFAULTTONEAREST),&mi);area=mi.rcWork;
            int width=std::min(s.scale(1010),(int)(area.right-area.left)),height=std::min(s.scale(624),(int)(area.bottom-area.top));
            int x=std::max((int)area.left,std::min((int)area.right-width,(int)(owner.left+owner.right-width)/2));int y=std::max((int)area.top,std::min((int)area.bottom-height,(int)(owner.top+owner.bottom-height)/2));
            SetWindowPos(w,nullptr,x,y,width,height,SWP_NOZORDER);layout(s);EnableWindow(s.open,FALSE);PostMessageW(w,START_SCAN,0,0);return TRUE;
        }
        if(!p)return FALSE;auto&s=*p;
        switch(m){
        case WM_ACTIVATE:
            if(LOWORD(wp)!=WA_INACTIVE&&s.appearanceIndex>=0){
                if(Theme::stored()!=Theme::index())syncAppearance();
                if(s.appearanceIndex!=Theme::index())applyPickerAppearance(s);
            }
            break;
        case START_SCAN:start(s);return TRUE;
        case WM_TIMER:if(wp==1&&s.done.load(std::memory_order_acquire)){KillTimer(w,1);if(s.worker.joinable())s.worker.join();s.busy=false;for(auto h:{s.refresh,s.add,s.filter,s.campaign})EnableWindow(h,TRUE);fill(s);}return TRUE;
        case WM_COMMAND:{int id=LOWORD(wp);if(id==IDCANCEL){s.stop();EndDialog(w,IDCANCEL);return TRUE;}if(id==IDOK){accept(s);return TRUE;}
            if(id==DEMO_MAIN||id==DEMO_RIZIA){s.chosenDemo=id==DEMO_MAIN?demo::sordland:demo::rizia;s.stop();EndDialog(w,IDOK);return TRUE;}
            if(id==BROWSE){auto picked=pickFile(false,0,w);if(!picked.empty()){s.choice=picked;s.stop();EndDialog(w,IDOK);}return TRUE;}
            if(id==LIVE_MODE){live_setup::show(w);return TRUE;}
            if(id==APPEARANCE){
                const int index=pickAppearance(w,s.appearance);
                if(index>=0&&index!=Theme::index()){applyAppearance(index);applyPickerAppearance(s);}
                return TRUE;
            }
            if(id==FILTER&&(HIWORD(wp)==EN_SETFOCUS||HIWORD(wp)==EN_KILLFOCUS)){InvalidateRect(w,&s.searchFrame,FALSE);return TRUE;}
            if(id==FILTER&&HIWORD(wp)==EN_CHANGE&&!s.busy&&s.list&&s.status&&s.path){fill(s);return TRUE;}
            if(id==CLEAR_SEARCH&&HIWORD(wp)==BN_CLICKED&&!s.busy){clearSearchField(s.filter);return TRUE;}
            if(id==CAMPAIGN_FILTER&&HIWORD(wp)==CBN_SELCHANGE&&!s.busy){fill(s);return TRUE;}
            if(id==RESCAN&&!s.busy){start(s);return TRUE;}
            if(id==ADD_FOLDER&&!s.busy){auto path=folder(w);if(!path.empty()){s.roots.push_back(path);extraRoots.push_back(path);start(s);}return TRUE;}break;}
        case WM_NOTIFY:{auto*n=reinterpret_cast<NMHDR*>(lp);if(n->idFrom==LIST){
            if(n->code==NM_CUSTOMDRAW){SetWindowLongPtrW(w,DWLP_MSGRESULT,drawRow(s,*reinterpret_cast<NMLVCUSTOMDRAW*>(lp)));return TRUE;}
            if(n->code==LVN_ITEMCHANGED&&!s.populating){selection(s);return TRUE;}
            if(n->code==NM_DBLCLK){PostMessageW(w,WM_COMMAND,IDOK,0);return TRUE;}
            if(n->code==LVN_KEYDOWN&&reinterpret_cast<NMLVKEYDOWN*>(lp)->wVKey==VK_RETURN){PostMessageW(w,WM_COMMAND,IDOK,0);return TRUE;}
        }break;}
        case WM_SIZE:layout(s);return TRUE;
        case WM_GETMINMAXINFO:{auto*i=reinterpret_cast<MINMAXINFO*>(lp);MONITORINFO info{};info.cbSize=sizeof(info);
            bool known=GetMonitorInfoW(MonitorFromWindow(w,MONITOR_DEFAULTTONEAREST),&info)!=0;
            int width=s.scale(500),height=s.scale(410);
            if(known){width=std::min(width,(int)(info.rcWork.right-info.rcWork.left));height=std::min(height,(int)(info.rcWork.bottom-info.rcWork.top));}
            i->ptMinTrackSize={width,height};return TRUE;}
        case WM_DPICHANGED:{s.dpi=HIWORD(wp);auto*r=reinterpret_cast<RECT*>(lp);setFonts(s);SetWindowPos(w,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);layout(s);return TRUE;}
        case WM_MEASUREITEM:{
            auto& item=*reinterpret_cast<MEASUREITEMSTRUCT*>(lp);
            if(item.CtlType==ODT_COMBOBOX&&item.CtlID==CAMPAIGN_FILTER){item.itemHeight=s.scale(24);return TRUE;}
            break;}
        case WM_DRAWITEM:{
            const auto& item=*reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if(item.CtlType==ODT_STATIC&&item.hwndItem==s.empty){drawEmpty(s,item);return TRUE;}
            if(item.CtlType==ODT_COMBOBOX&&item.CtlID==CAMPAIGN_FILTER){drawCampaignItem(s,item);return TRUE;}
            if(item.CtlType==ODT_BUTTON&&item.CtlID==CLEAR_SEARCH){drawClearSearch(item,s.dpi);return TRUE;}
            if(item.CtlType==ODT_BUTTON){drawButton(s,item);return TRUE;}
            break;}
        case WM_CTLCOLORDLG:return (INT_PTR)g.bg;
        case WM_PAINT:{PAINTSTRUCT paint{};auto dc=BeginPaint(w,&paint);FillRect(dc,&paint.rcPaint,g.bg);paintFrame(s,dc);EndPaint(w,&paint);return TRUE;}
        case WM_CTLCOLORLISTBOX:{auto dc=(HDC)wp;SetTextColor(dc,Theme::text);SetBkColor(dc,Theme::input);return (INT_PTR)g.input;}
        case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:{auto dc=(HDC)wp;auto h=(HWND)lp;
            SetTextColor(dc,h==s.path||h==s.status||h==s.practiceHint||h==s.empty?Theme::muted:Theme::text);
            bool panel=h==s.status||((h==s.practiceTitle||h==s.practiceHint)&&s.stacked);
            SetBkColor(dc,h==s.filter?Theme::input:panel?Theme::panel:Theme::bg);
            return (INT_PTR)(h==s.filter?g.input:panel?g.panel:g.bg);}
        case WM_CLOSE:s.stop();EndDialog(w,IDCANCEL);return TRUE;
        case WM_DESTROY:KillTimer(w,1);s.stop();uiText.erase(s.status);uiText.erase(s.path);uiText.erase(s.empty);return TRUE;
        }
    }catch(const std::exception&e){if(p){p->stop();KillTimer(w,1);for(auto h:{p->refresh,p->add,p->filter,p->campaign})if(h)EnableWindow(h,TRUE);}localizedMessageBox(w,wide(e.what()),MB_OK|MB_ICONERROR);if(m==WM_INITDIALOG)EndDialog(w,IDCANCEL);return TRUE;}
    catch(...){if(p)p->stop();localizedMessageBox(w,L"搜索未完成，请重试。",MB_OK|MB_ICONERROR);if(m==WM_INITDIALOG)EndDialog(w,IDCANCEL);return TRUE;}
    return FALSE;
}
} // namespace save_picker

inline void chooseSave(){
    save_picker::State s;s.roots.push_back(saveFolder());if(!g.file.empty())s.roots.push_back(g.file.parent_path());s.roots.insert(s.roots.end(),save_picker::extraRoots.begin(),save_picker::extraRoots.end());
    // An empty, correctly aligned dialog template; controls are created at the current monitor DPI.
    struct alignas(DWORD) Template {DLGTEMPLATE dlg;WORD menu=0,windowClass=0,title=0;} t{};
    t.dlg.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN|DS_MODALFRAME;t.dlg.dwExtendedStyle=WS_EX_CONTROLPARENT;t.dlg.cx=560;t.dlg.cy=370;
    INT_PTR result=DialogBoxIndirectParamW(GetModuleHandleW(nullptr),&t.dlg,g.win,save_picker::proc,(LPARAM)&s);
    if(result==-1)throw WinFailure(L"无法打开存档选择窗口："+winError());
    if(result==IDOK){if(s.chosenDemo)loadDemo(s.chosenDemo);else if(!s.choice.empty())loadDocument(s.choice);}
}
