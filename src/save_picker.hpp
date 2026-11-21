#pragma once
#include <atomic>
#include <thread>
#include <shobjidl.h>
#include "save_discovery.hpp"

// The picker owns its controls and fonts. Discovery never touches the editor or files.
namespace save_picker {
enum { LIST=410, FILTER, ADD_FOLDER, RESCAN, STATUS, PATH, CAMPAIGN_FILTER, BROWSE, DEMO_MAIN, DEMO_RIZIA, START_SCAN=WM_APP+44 };
inline std::vector<fs::path> extraRoots;
struct State {
    HWND win{}, list{}, filter{}, campaign{}, status{}, path{}, add{}, refresh{}, open{}, cancelButton{}, heading{};
    HWND browse{}, practiceTitle{}, practiceHint{}, mainDemo{}, riziaDemo{}, empty{};
    UINT dpi=96;
    HFONT font{}, small{}, bold{}, subheading{};
    HIMAGELIST rowHeight{};
    std::vector<fs::path> roots;
    rizia::discovery::Result result;
    std::vector<size_t> rows;
    fs::path choice;
    fs::path beforeScan;
    int chosenDemo=0;
    int dividerY=0;
    std::thread worker;
    std::atomic_bool cancel{false}, done{false};
    bool busy=false, populating=false;
    std::wstring error;
    ~State(){ stop(); if(rowHeight)ImageList_Destroy(rowHeight); for(auto f:{font,small,bold,subheading})if(f)DeleteObject(f); }
    void stop(){cancel.store(true);if(worker.joinable())worker.join();busy=false;}
    int scale(int n)const{return MulDiv(n,dpi,96);}
    void move(HWND h,int x,int y,int w,int he){MoveWindow(h,scale(x),scale(y),scale(std::max(1,w)),scale(std::max(1,he)),TRUE);}
};
inline State* state(HWND w){return reinterpret_cast<State*>(GetWindowLongPtrW(w,DWLP_USER));}

inline HWND child(State& s,const wchar_t* cls,const wchar_t* text,DWORD style,int id,DWORD ex=0){
    auto h=CreateWindowExW(ex,cls,tr(text).c_str(),WS_CHILD|WS_VISIBLE|style,0,0,0,0,s.win,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
    if(!h)throw WinFailure(L"无法创建存档列表："+winError());
    SendMessageW(h,WM_SETFONT,(WPARAM)s.font,TRUE);return h;
}
inline void setFonts(State& s){
    auto make=[&](int size,int weight,const wchar_t* face=L"Segoe UI"){return CreateFontW(-s.scale(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,face);};
    HFONT old[]={s.font,s.small,s.bold,s.subheading};s.font=make(15,FW_NORMAL);s.small=make(13,FW_NORMAL);s.bold=make(26,FW_SEMIBOLD);s.subheading=make(16,FW_SEMIBOLD);
    EnumChildWindows(s.win,[](HWND h,LPARAM p){auto&s=*reinterpret_cast<State*>(p);SendMessageW(h,WM_SETFONT,(WPARAM)s.font,TRUE);return TRUE;},(LPARAM)&s);
    for(auto h:{s.status,s.path,s.practiceHint})if(h)SendMessageW(h,WM_SETFONT,(WPARAM)s.small,TRUE);
    if(s.heading)SendMessageW(s.heading,WM_SETFONT,(WPARAM)s.bold,TRUE);
    if(s.practiceTitle)SendMessageW(s.practiceTitle,WM_SETFONT,(WPARAM)s.subheading,TRUE);
    for(auto f:old)if(f)DeleteObject(f);
    if(s.list){auto oldImage=s.rowHeight;s.rowHeight=ImageList_Create(1,s.scale(35),ILC_COLOR32,1,1);ListView_SetImageList(s.list,s.rowHeight,LVSIL_SMALL);if(oldImage)ImageList_Destroy(oldImage);}
}
inline void layout(State& s){
    if(!s.empty)return;
    RECT r{};GetClientRect(s.win,&r);
    int w=MulDiv(r.right,96,s.dpi),h=MulDiv(r.bottom,96,s.dpi);
    const bool narrow=w<700, shortWindow=h<450;
    const int margin=narrow?16:24, inner=w-2*margin, top=shortWindow?8:17;
    const int buttonsY=h-(shortWindow?47:57);
    const int demoY=buttonsY-(narrow?77:68);
    const int searchY=top+(shortWindow?39:50), listY=searchY+42;
    const int pathY=demoY-35;
    s.move(s.heading,margin,top,inner-242,37);
    s.move(s.add,w-margin-236,top+2,132,32);
    s.move(s.refresh,w-margin-96,top+2,96,32);
    int campaignW=std::min(200,inner/2);
    s.move(s.filter,margin,searchY,inner-campaignW-10,30);
    s.move(s.campaign,w-margin-campaignW,searchY,campaignW,220);
    const int listH=std::max(54,pathY-listY-5);
    s.move(s.list,margin,listY,inner,listH);
    s.move(s.empty,margin,listY,inner,listH);
    const int statusW=narrow?std::min(185,inner/2):235;
    s.move(s.path,margin,pathY,inner-statusW-10,22);
    s.move(s.status,w-margin-statusW,pathY,statusW,22);
    s.dividerY=demoY-8;
    s.move(s.practiceTitle,margin,demoY,narrow?inner:inner-330,21);
    if(narrow)s.move(s.practiceHint,margin+145,demoY,inner-145,24);
    else s.move(s.practiceHint,margin,demoY+24,inner-330,32);
    int demoButtonW=narrow?std::min(160,(inner-8)/2):155;
    int demoButtonX=narrow?margin:w-margin-2*demoButtonW-8;
    int demoButtonY=demoY+(narrow?27:7);
    s.move(s.mainDemo,demoButtonX,demoButtonY,demoButtonW,32);
    s.move(s.riziaDemo,demoButtonX+demoButtonW+8,demoButtonY,demoButtonW,32);
    s.move(s.browse,margin,buttonsY,135,34);
    s.move(s.cancelButton,w-margin-251,buttonsY,103,34);
    s.move(s.open,w-margin-140,buttonsY,140,34);
    // Give the filename meaningful space on smaller displays; the selected path remains visible.
    int widths[]={std::max(165,inner-550),109,159,63,190};
    if(narrow){widths[0]=std::max(126,inner-313);widths[1]=97;widths[2]=0;widths[3]=50;widths[4]=146;}
    for(int i=0;i<5;++i)ListView_SetColumnWidth(s.list,i,s.scale(widths[i]));
    InvalidateRect(s.win,nullptr,FALSE);
}
inline int selected(const State& s){return ListView_GetNextItem(s.list,-1,LVNI_SELECTED);}
inline void selection(State& s){
    int i=selected(s);bool valid=!s.busy&&i>=0&&i<(int)s.rows.size();
    EnableWindow(s.open,valid);
    rawText(s.path,valid?s.result.saves[s.rows[i]].path.wstring():L"");
}
inline void fill(State& s){
    fs::path keep=std::move(s.beforeScan);s.beforeScan.clear();
    int current=selected(s);if(current>=0&&current<(int)s.rows.size())keep=s.result.saves[s.rows[current]].path;
    s.populating=true;SendMessageW(s.list,WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(s.list);s.rows.clear();
    auto query=lower(trim(textOf(s.filter)));int at=-1;auto campaign=(int)SendMessageW(s.campaign,CB_GETCURSEL,0,0);
    for(size_t n=0;n<s.result.saves.size();++n){const auto& save=s.result.saves[n];
        if((campaign==1&&save.story!="StoryPack_Main")||(campaign==2&&save.story!="StoryPack_Rizia"))continue;
        if(!query.empty()&&lower(save.path.filename().wstring()+L" "+wide(save.campaignName)+L" "+campaignLabel(save.story)+(save.story=="StoryPack_Main"?L" Sordland":L" Rizia")).find(query)==std::wstring::npos)continue;
        int row=(int)s.rows.size();s.rows.push_back(n);auto name=save.path.filename().wstring();
        LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=row;item.pszText=name.data();ListView_InsertItem(s.list,&item);
        std::wstring cells[]={campaignLabel(save.story),wide(save.campaignName),wide(save.turn),fileTimeText(save.path)};
        for(int c=0;c<4;++c)ListView_SetItemText(s.list,row,c+1,cells[c].data());
        if(save.path==keep)at=row;
    }
    if(at<0&&!s.rows.empty())at=0;
    if(at>=0){ListView_SetItemState(s.list,at,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);ListView_EnsureVisible(s.list,at,FALSE);}
    SendMessageW(s.list,WM_SETREDRAW,TRUE,0);InvalidateRect(s.list,nullptr,TRUE);s.populating=false;selection(s);
    std::wstring status;
    if(s.rows.empty())status=L"0 / "+std::to_wstring(s.result.saves.size());
    else status=std::to_wstring(s.rows.size())+L" 个存档 · 最新在前";
    if(s.result.truncated)status+=L" · 已到搜索上限";
    else if(s.result.errors)status+=L" · 部分文件无法读取";
    setText(s.status,status);
    setText(s.empty,!s.error.empty()?s.error:s.result.saves.empty()?L"未找到存档，请添加文件夹。":L"没有匹配的存档");
    ShowWindow(s.list,s.rows.empty()?SW_HIDE:SW_SHOW);
    ShowWindow(s.empty,s.rows.empty()?SW_SHOW:SW_HIDE);
}
inline void start(State& s){
    if(s.busy)return;
    int current=selected(s);
    if(current>=0&&current<(int)s.rows.size())s.beforeScan=s.result.saves[s.rows[current]].path;
    s.stop();s.cancel.store(false);s.done.store(false);s.error.clear();s.result={};s.rows.clear();
    s.populating=true;ListView_DeleteAllItems(s.list);s.populating=false;s.busy=true;
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
inline void drawButton(State& s,const DRAWITEMSTRUCT& d){
    bool disabled=(d.itemState&ODS_DISABLED)!=0,primary=d.CtlID==IDOK;
    COLORREF bg=primary?Theme::sidebar:Theme::panel,fg=primary?RGB(250,247,234):Theme::text;
    if(disabled){bg=Theme::panel;fg=Theme::dim;}else if(d.itemState&ODS_SELECTED)bg=primary?RGB(47,86,67):Theme::select;
    else if(GetPropW(d.hwndItem,L"RiziaHover")&&!primary)bg=Theme::select;
    RECT r=d.rcItem;FillRect(d.hDC,&r,g.bg);auto brush=CreateSolidBrush(bg);auto pen=CreatePen(PS_SOLID,1,primary?bg:Theme::line);auto ob=SelectObject(d.hDC,brush),op=SelectObject(d.hDC,pen);
    --r.right;--r.bottom;RoundRect(d.hDC,r.left,r.top,r.right,r.bottom,s.scale(8),s.scale(8));SelectObject(d.hDC,ob);SelectObject(d.hDC,op);DeleteObject(brush);DeleteObject(pen);
    drawText(d.hDC,textOf(d.hwndItem),r,s.font,fg,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(d.itemState&ODS_FOCUS){InflateRect(&r,-s.scale(4),-s.scale(4));DrawFocusRect(d.hDC,&r);}
}
inline LRESULT CALLBACK header(HWND h,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
    auto&s=*reinterpret_cast<State*>(data);
    if(m==WM_ERASEBKGND)return 1;
    if(m==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(h,&ps);RECT r{};GetClientRect(h,&r);FillRect(dc,&r,g.panel);
        for(int i=0;i<Header_GetItemCount(h);++i){Header_GetItemRect(h,i,&r);wchar_t text[64]{};HDITEMW item{};item.mask=HDI_TEXT;item.pszText=text;item.cchTextMax=64;Header_GetItem(h,i,&item);r.left+=s.scale(10);drawText(dc,text,r,s.small,Theme::muted);}
        EndPaint(h,&ps);return 0;}
    return DefSubclassProc(h,m,wp,lp);
}
inline INT_PTR CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
    State* p=state(w);
    try{
        if(m==WM_INITDIALOG){p=reinterpret_cast<State*>(lp);SetWindowLongPtrW(w,DWLP_USER,(LONG_PTR)p);auto&s=*p;s.win=w;SetWindowTextW(w,tr(L"选择存档").c_str());s.dpi=GetDpiForWindow(w);if(!s.dpi)s.dpi=96;setFonts(s);
            BOOL dark=TRUE;DwmSetWindowAttribute(w,DWMWA_USE_IMMERSIVE_DARK_MODE,&dark,sizeof(dark));COLORREF caption=Theme::sidebar;DwmSetWindowAttribute(w,DWMWA_CAPTION_COLOR,&caption,sizeof(caption));
            COLORREF captionText=RGB(249,247,235);DwmSetWindowAttribute(w,DWMWA_TEXT_COLOR,&captionText,sizeof(captionText));
            s.heading=child(s,L"STATIC",L"选择存档",SS_ENDELLIPSIS,0);
            s.filter=child(s,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,FILTER,WS_EX_CLIENTEDGE);auto cue=tr(L"搜索文件名或战役");SendMessageW(s.filter,EM_SETCUEBANNER,TRUE,(LPARAM)cue.c_str());SendMessageW(s.filter,EM_SETLIMITTEXT,160,0);
            s.campaign=child(s,L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,CAMPAIGN_FILTER);
            for(const auto* name:{L"All campaigns",L"Sordland",L"Rizia"}){auto text=tr(name);SendMessageW(s.campaign,CB_ADDSTRING,0,(LPARAM)text.c_str());}SendMessageW(s.campaign,CB_SETCURSEL,0,0);
            s.add=child(s,L"BUTTON",L"添加文件夹",WS_TABSTOP|BS_OWNERDRAW,ADD_FOLDER);s.refresh=child(s,L"BUTTON",L"刷新",WS_TABSTOP|BS_OWNERDRAW,RESCAN);
            s.list=child(s,WC_LISTVIEWW,L"存档列表",WS_TABSTOP|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS,LIST);
            ListView_SetExtendedListViewStyle(s.list,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_LABELTIP);SetWindowTheme(s.list,L"Explorer",nullptr);
            ListView_SetBkColor(s.list,Theme::panel);ListView_SetTextBkColor(s.list,Theme::panel);ListView_SetTextColor(s.list,Theme::text);
            const wchar_t* cols[]={L"文件名",L"Campaign",L"战役",L"回合",L"修改时间"};for(int i=0;i<5;++i){LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;auto title=tr(cols[i]);c.pszText=title.data();c.cx=s.scale(120);ListView_InsertColumn(s.list,i,&c);}
            SetWindowSubclass(ListView_GetHeader(s.list),header,1,(DWORD_PTR)&s);
            s.path=child(s,L"STATIC",L"",SS_PATHELLIPSIS|SS_NOPREFIX,PATH);
            s.status=child(s,L"STATIC",L"正在搜索…",SS_RIGHT|SS_ENDELLIPSIS,STATUS);
            s.practiceTitle=child(s,L"STATIC",L"内置练习示例",SS_ENDELLIPSIS,0);
            s.practiceHint=child(s,L"STATIC",L"可体验编辑功能，无法在游戏中载入。",0,0);
            s.mainDemo=child(s,L"BUTTON",L"索德兰示例",WS_TABSTOP|BS_OWNERDRAW,DEMO_MAIN);
            s.riziaDemo=child(s,L"BUTTON",L"里奇亚示例",WS_TABSTOP|BS_OWNERDRAW,DEMO_RIZIA);
            s.browse=child(s,L"BUTTON",L"手动打开",WS_TABSTOP|BS_OWNERDRAW,BROWSE);
            s.open=child(s,L"BUTTON",L"打开所选",WS_TABSTOP|BS_OWNERDRAW,IDOK);s.cancelButton=child(s,L"BUTTON",L"取消",WS_TABSTOP|BS_OWNERDRAW,IDCANCEL);
            s.empty=child(s,L"STATIC",L"正在搜索…",SS_CENTER|SS_CENTERIMAGE|SS_ENDELLIPSIS,0);
            for(auto button:{s.add,s.refresh,s.mainDemo,s.riziaDemo,s.browse,s.open,s.cancelButton})SetWindowSubclass(button,buttonProc,1,0);
            setFonts(s);RECT owner{},area{};GetWindowRect(g.win,&owner);MONITORINFO mi{};mi.cbSize=sizeof(mi);GetMonitorInfoW(MonitorFromWindow(g.win,MONITOR_DEFAULTTONEAREST),&mi);area=mi.rcWork;
            int width=std::min(s.scale(1010),(int)(area.right-area.left)),height=std::min(s.scale(624),(int)(area.bottom-area.top));
            int x=std::max((int)area.left,std::min((int)area.right-width,(int)(owner.left+owner.right-width)/2));int y=std::max((int)area.top,std::min((int)area.bottom-height,(int)(owner.top+owner.bottom-height)/2));
            SetWindowPos(w,nullptr,x,y,width,height,SWP_NOZORDER);layout(s);EnableWindow(s.open,FALSE);PostMessageW(w,START_SCAN,0,0);return TRUE;
        }
        if(!p)return FALSE;auto&s=*p;
        switch(m){
        case START_SCAN:start(s);return TRUE;
        case WM_TIMER:if(wp==1&&s.done.load(std::memory_order_acquire)){KillTimer(w,1);if(s.worker.joinable())s.worker.join();s.busy=false;for(auto h:{s.refresh,s.add,s.filter,s.campaign})EnableWindow(h,TRUE);fill(s);}return TRUE;
        case WM_COMMAND:{int id=LOWORD(wp);if(id==IDCANCEL){s.stop();EndDialog(w,IDCANCEL);return TRUE;}if(id==IDOK){accept(s);return TRUE;}
            if(id==DEMO_MAIN||id==DEMO_RIZIA){s.chosenDemo=id==DEMO_MAIN?demo::sordland:demo::rizia;s.stop();EndDialog(w,IDOK);return TRUE;}
            if(id==BROWSE){auto picked=pickFile(false,0,w);if(!picked.empty()){s.choice=picked;s.stop();EndDialog(w,IDOK);}return TRUE;}
            if(id==FILTER&&HIWORD(wp)==EN_CHANGE&&!s.busy&&s.list&&s.status&&s.path){fill(s);return TRUE;}
            if(id==CAMPAIGN_FILTER&&HIWORD(wp)==CBN_SELCHANGE&&!s.busy){fill(s);return TRUE;}
            if(id==RESCAN&&!s.busy){start(s);return TRUE;}
            if(id==ADD_FOLDER&&!s.busy){auto path=folder(w);if(!path.empty()){s.roots.push_back(path);extraRoots.push_back(path);start(s);}return TRUE;}break;}
        case WM_NOTIFY:{auto*n=reinterpret_cast<NMHDR*>(lp);if(n->idFrom==LIST){
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
        case WM_DRAWITEM:drawButton(s,*reinterpret_cast<DRAWITEMSTRUCT*>(lp));return TRUE;
        case WM_CTLCOLORDLG:return (INT_PTR)g.bg;
        case WM_PAINT:{PAINTSTRUCT paint{};auto dc=BeginPaint(w,&paint);FillRect(dc,&paint.rcPaint,g.bg);
            RECT r{};GetClientRect(w,&r);RECT line{s.scale(16),s.scale(s.dividerY),r.right-s.scale(16),s.scale(s.dividerY)+1};
            HBRUSH pen=CreateSolidBrush(Theme::line);FillRect(dc,&line,pen);DeleteObject(pen);EndPaint(w,&paint);return TRUE;}
        case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:{auto dc=(HDC)wp;auto h=(HWND)lp;
            SetTextColor(dc,h==s.path||h==s.status||h==s.practiceHint||h==s.empty?Theme::muted:Theme::text);
            SetBkColor(dc,h==s.filter?Theme::input:h==s.empty?Theme::panel:Theme::bg);
            return (INT_PTR)(h==s.filter?g.input:h==s.empty?g.panel:g.bg);}
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
