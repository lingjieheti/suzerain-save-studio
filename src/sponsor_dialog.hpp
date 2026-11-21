#pragma once
#include "sponsor_text.hpp"

namespace sponsor {
using sponsor_text::Key;
using sponsor_text::get;

inline constexpr wchar_t tronAddress[]=L"TXE2VuxUSFsenJvmz3Z8oSm7dqPLAUWfRE";
inline constexpr wchar_t alipayUrl[]=L"https://qr.alipay.com/fkx13023nip9yxdtfe6qqbe";
inline constexpr wchar_t paypalUrl[]=L"https://www.paypal.com/qrcodes/p2pqrc/8CZSV4VY73PB2";
struct Method {
    Key name, hint;
    int bitmapId;
    const wchar_t* page;

    const wchar_t* actionLabel() const {
        if(page)return get(Key::OpenPage);
        return name==Key::USDT?get(Key::CopyAddress):L"";
    }
};
inline constexpr Method methods[]={
    {Key::WeChat,Key::ScanWeChat,204,nullptr},
    {Key::Alipay,Key::ScanAlipay,205,alipayUrl},
    {Key::PayPal,Key::ScanPayPal,206,paypalUrl},
    {Key::USDT,Key::ScanUSDT,207,nullptr}
};
inline constexpr int methodCount=static_cast<int>(std::size(methods));
enum Control { FirstMethod=610, LastMethod=FirstMethod+methodCount-1, Qr, MethodName, Hint, Address, Action, Heading, Introduction };

struct State {
    HWND win{};
    UINT dpi=96;
    int method=0;
    HBITMAP bitmap{};
    HFONT body{},heading{},methodFont{},addressFont{};
    int scale(int dip) const {return MulDiv(dip,dpi,96);}
    ~State(){
        if(bitmap)DeleteObject(bitmap);
        for(auto font:{body,heading,methodFont,addressFont})if(font)DeleteObject(font);
    }
};

inline void fonts(State& s) {
    const auto chosen=typography::chooseFaces(i18n::language);
    HFONT old[]={s.body,s.heading,s.methodFont,s.addressFont};
    s.body=typography::make(s.dpi,14,chosen.text);
    s.heading=typography::make(s.dpi,25,chosen.emphasis,chosen.emphasisWeight);
    s.methodFont=typography::make(s.dpi,20,chosen.emphasis,chosen.emphasisWeight);
    s.addressFont=typography::make(s.dpi,14,L"Consolas");
    for(HWND h=GetWindow(s.win,GW_CHILD);h;h=GetWindow(h,GW_HWNDNEXT)){
        int id=GetDlgCtrlID(h);
        HFONT f=id==Heading?s.heading:id==MethodName?s.methodFont:id==Address?s.addressFont:s.body;
        SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(f),TRUE);
    }
    for(auto f:old)if(f)DeleteObject(f);
}

inline void layout(const State& s) {
    RECT bounds{};GetClientRect(s.win,&bounds);
    int w=MulDiv(bounds.right,96,s.dpi),h=MulDiv(bounds.bottom,96,s.dpi);
    const bool compact=h<365;
    const int pad=w<500?16:25, gap=w<500?12:24;
    const int top=compact?10:20;
    HDC dc=GetDC(s.win);
    auto textHeight=[&](const wchar_t* text,HFONT font,int width){
        if(!dc || !font)return 22;
        RECT area{0,0,s.scale(std::max(width,1)),0};
        auto previous=SelectObject(dc,font);
        DrawTextW(dc,text,-1,&area,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
        SelectObject(dc,previous);
        return static_cast<int>((area.bottom*96+s.dpi-1)/s.dpi);
    };
    const int headingHeight=std::max(35,textHeight(get(Key::Title),s.heading,w-2*pad)+1);
    const int introHeight=textHeight(get(Key::Intro),s.body,w-2*pad)+1;
    const int tabs=top+headingHeight+introHeight+(compact?6:12);
    const int bodyTop=tabs+(compact?41:50), bodyEnd=h-(compact?43:55);
    const int code=std::max(48,std::min({212,bodyEnd-bodyTop,(w-2*pad-gap)*2/5}));
    const int col=pad+code+gap, detailWidth=w-pad-col;
    auto move=[&](int id,int x,int y,int width,int height){
        MoveWindow(GetDlgItem(s.win,id),s.scale(x),s.scale(y),s.scale(x+width)-s.scale(x),
                   s.scale(y+height)-s.scale(y),TRUE);
    };
    move(Heading,pad,top,w-2*pad,headingHeight);
    move(Introduction,pad,top+headingHeight,w-2*pad,introHeight);
    for(int i=0;i<methodCount;++i){
        int x=pad+(w-2*pad+5)*i/methodCount, next=pad+(w-2*pad+5)*(i+1)/methodCount;
        move(FirstMethod+i,x,tabs,next-x-5,34);
    }
    move(Qr,pad,bodyTop+(bodyEnd-bodyTop-code)/2,code,code);
    const auto& method=methods[s.method];
    const auto actionLabel=method.actionLabel();
    const int titleHeight=textHeight(get(method.name),s.methodFont,detailWidth)+1;
    const int hintHeight=textHeight(get(method.hint),s.body,detailWidth)+1;
    const int addressLine=textHeight(L"A",s.addressFont,detailWidth);
    int addressHeight=2*addressLine+4;
    const int closeWidth=104, footerY=h-(compact?37:43);
    const int maxActionWidth=compact?w-2*pad-closeWidth-gap:detailWidth;
    int actionWidth=maxActionWidth;
    if(dc){
        const int addressWidth=typography::width(dc,s.addressFont,tronAddress);
        const int innerWidth=std::max(1,s.scale(detailWidth-5));
        addressHeight=((addressWidth+innerWidth-1)/innerWidth)*addressLine+4;
        int labelWidth=typography::width(dc,s.body,actionLabel);
        if(method.name==Key::USDT)labelWidth=std::max(labelWidth,typography::width(dc,s.body,get(Key::Copied)));
        actionWidth=std::min(maxActionWidth,std::max(116,MulDiv(labelWidth,96,s.dpi)+28));
        ReleaseDC(s.win,dc);
    }
    const int spacing=compact?4:8;
    int groupHeight=hintHeight;
    if(method.name==Key::USDT)groupHeight+=spacing+addressHeight;
    if(!compact && *actionLabel)groupHeight+=spacing+4+32;
    // The selected radio already names the method on short work areas.
    const bool showName=!compact || titleHeight+spacing+groupHeight<=bodyEnd-bodyTop;
    if(showName)groupHeight+=titleHeight+spacing;
    ShowWindow(GetDlgItem(s.win,MethodName),showName?SW_SHOW:SW_HIDE);
    int y=bodyTop+std::max(0,(bodyEnd-bodyTop-groupHeight)/2);
    if(showName){
        move(MethodName,col,y,detailWidth,titleHeight);
        y+=titleHeight+spacing;
    }
    move(Hint,col,y,detailWidth,hintHeight);
    y+=hintHeight;
    if(method.name==Key::USDT){
        y+=spacing;
        move(Address,col,y,detailWidth,addressHeight);
        y+=addressHeight;
    }
    if(compact)move(Action,pad,footerY,actionWidth,32);
    else move(Action,col,y+spacing+4,actionWidth,32);
    move(IDCANCEL,w-pad-closeWidth,footerY,closeWidth,32);
}

inline void fitWindow(const State& s, RECT anchor) {
    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfoW(MonitorFromRect(&anchor,MONITOR_DEFAULTTONEAREST),&monitor);
    RECT bounds{0,0,s.scale(604),s.scale(410)};
    AdjustWindowRectExForDpi(&bounds,static_cast<DWORD>(GetWindowLongPtrW(s.win,GWL_STYLE)),FALSE,
                             static_cast<DWORD>(GetWindowLongPtrW(s.win,GWL_EXSTYLE)),s.dpi);
    const RECT work=monitor.rcWork;
    int w=std::min(bounds.right-bounds.left,work.right-work.left);
    int h=std::min(bounds.bottom-bounds.top,work.bottom-work.top);
    int x=std::clamp<LONG>((anchor.left+anchor.right-w)/2,work.left,work.right-w);
    int y=std::clamp<LONG>((anchor.top+anchor.bottom-h)/2,work.top,work.bottom-h);
    SetWindowPos(s.win,nullptr,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
}

inline void selectMethod(State& s,int method) {
    s.method=method;
    CheckRadioButton(s.win,FirstMethod,LastMethod,FirstMethod+method);
    if(s.bitmap)DeleteObject(s.bitmap);
    s.bitmap=static_cast<HBITMAP>(LoadImageW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(methods[method].bitmapId),
                                             IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION));
    SetDlgItemTextW(s.win,MethodName,get(methods[method].name));
    SetDlgItemTextW(s.win,Hint,get(methods[method].hint));
    SetDlgItemTextW(s.win,Qr,get(methods[method].name));
    const auto actionLabel=methods[method].actionLabel();
    SetDlgItemTextW(s.win,Action,actionLabel);
    ShowWindow(GetDlgItem(s.win,Action),*actionLabel?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(s.win,Address),methods[method].name==Key::USDT?SW_SHOW:SW_HIDE);
    layout(s);
    RedrawWindow(s.win,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
}

inline bool copyAddress(HWND owner) {
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,sizeof(tronAddress));
    if(!memory)return false;
    void* data=GlobalLock(memory);
    if(data){
        std::copy_n(tronAddress,std::size(tronAddress),static_cast<wchar_t*>(data));
        GlobalUnlock(memory);
        if(OpenClipboard(owner)){
            const bool copied=EmptyClipboard() && SetClipboardData(CF_UNICODETEXT,memory);
            CloseClipboard();
            if(copied)return true; // Windows now owns the allocation.
        }
    }
    GlobalFree(memory);
    return false;
}

inline void act(State& s) {
    const auto& method=methods[s.method];
    if(method.name==Key::USDT){
        if(copyAddress(s.win)){
            SetDlgItemTextW(s.win,Action,get(Key::Copied));
            layout(s);
        }else MessageBoxW(s.win,get(Key::CopyFailed),get(Key::Title),MB_OK|MB_ICONWARNING);
    }else if(method.page){
        const auto result=ShellExecuteW(s.win,L"open",method.page,
                                        nullptr,nullptr,SW_SHOWNORMAL);
        if(reinterpret_cast<INT_PTR>(result)<=32)
            MessageBoxW(s.win,get(Key::OpenFailed),get(Key::Title),MB_OK|MB_ICONWARNING);
    }
}

inline void drawQr(const State& s,const DRAWITEMSTRUCT& item) {
    const RECT box=item.rcItem;
    HDC dc=item.hDC;
    FillRect(dc,&box,static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    if(!s.bitmap){
        RECT text=box;
        SetTextColor(dc,RGB(25,25,25));SetBkMode(dc,TRANSPARENT);
        DrawTextW(dc,get(Key::QrUnavailable),-1,&text,DT_CENTER|DT_WORDBREAK|DT_NOPREFIX);
        return;
    }
    BITMAP bm{};
    if(!GetObjectW(s.bitmap,sizeof(bm),&bm) || !bm.bmWidth || !bm.bmHeight)return;
    // Keep every QR module square, including the four-module white border in the resource.
    const int scale=std::min((box.right-box.left)/bm.bmWidth,(box.bottom-box.top)/bm.bmHeight);
    const int side=scale>0?scale*bm.bmWidth:std::min(box.right-box.left,box.bottom-box.top);
    HDC source=CreateCompatibleDC(dc);
    if(!source)return;
    auto old=SelectObject(source,s.bitmap);
    int saved=SaveDC(dc);
    SetStretchBltMode(dc,COLORONCOLOR);
    SetTextColor(dc,RGB(0,0,0));SetBkColor(dc,RGB(255,255,255));
    StretchBlt(dc,(box.left+box.right-side)/2,(box.top+box.bottom-side)/2,side,side,
               source,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
    RestoreDC(dc,saved);SelectObject(source,old);DeleteDC(source);
}

// Native radio/push buttons keep their keyboard handling, roles and checked state.
inline LRESULT paintButton(const State& s,const NMCUSTOMDRAW& item) {
    if(item.dwDrawStage!=CDDS_PREPAINT)return CDRF_DODEFAULT;
    HWND button=item.hdr.hwndFrom;
    const int id=GetDlgCtrlID(button);
    const bool checked=id==FirstMethod+s.method;
    const bool over=(item.uItemState&(CDIS_HOT|CDIS_SELECTED))!=0;
    const COLORREF bg=checked?Theme::accent:over?Theme::hover:Theme::panel;
    const COLORREF ink=checked?Theme::onAccent:Theme::text;
    HDC dc=item.hdc;RECT box=item.rc;
    const int saved=SaveDC(dc);
    SetDCBrushColor(dc,Theme::bg);FillRect(dc,&box,static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SelectObject(dc,GetStockObject(DC_PEN));SelectObject(dc,GetStockObject(DC_BRUSH));
    const bool focus=(item.uItemState&CDIS_FOCUS) && !(SendMessageW(button,WM_QUERYUISTATE,0,0)&UISF_HIDEFOCUS);
    SetDCPenColor(dc,focus?Theme::focus:checked?Theme::accent:Theme::line);SetDCBrushColor(dc,bg);
    const int corner=s.scale(Theme::radius(10));
    RoundRect(dc,box.left,box.top,box.right,box.bottom,corner,corner);
    if(focus){RECT ring=box;InflateRect(&ring,-s.scale(3),-s.scale(3));DrawFocusRect(dc,&ring);}
    SetTextColor(dc,ink);SetBkMode(dc,TRANSPARENT);SelectObject(dc,s.body);
    auto label=textOf(button);
    DrawTextW(dc,label.c_str(),-1,&box,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    RestoreDC(dc,saved);
    return CDRF_SKIPDEFAULT;
}

inline INT_PTR CALLBACK proc(HWND w,UINT msg,WPARAM wp,LPARAM lp) {
    State* state=reinterpret_cast<State*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    if(msg==WM_INITDIALOG){
        state=reinterpret_cast<State*>(lp);
        SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(state));
        auto& s=*state;s.win=w;s.dpi=GetDpiForWindow(w);
        SetWindowTextW(w,get(Key::Title));Theme::caption(w);
        auto add=[&](int id,const wchar_t* cls,const wchar_t* name,DWORD style){
            return CreateWindowExW(0,cls,name,WS_CHILD|WS_VISIBLE|style,0,0,0,0,w,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
        };
        add(Heading,L"STATIC",get(Key::Title),SS_NOPREFIX);
        add(Introduction,L"STATIC",get(Key::Intro),SS_NOPREFIX);
        for(int i=0;i<methodCount;++i)
            add(FirstMethod+i,L"BUTTON",get(methods[i].name),BS_AUTORADIOBUTTON|BS_PUSHLIKE|
                (i==0?WS_GROUP|WS_TABSTOP:0));
        add(Qr,L"STATIC",get(methods[0].name),WS_GROUP|SS_OWNERDRAW);
        add(MethodName,L"STATIC",L"",SS_NOPREFIX);
        add(Hint,L"STATIC",L"",SS_NOPREFIX);
        add(Address,L"EDIT",tronAddress,ES_MULTILINE|ES_READONLY|WS_TABSTOP);
        add(Action,L"BUTTON",L"",WS_TABSTOP|BS_PUSHBUTTON);
        add(IDCANCEL,L"BUTTON",get(Key::Close),WS_TABSTOP|BS_PUSHBUTTON);
        fonts(s);
        RECT anchor{};GetWindowRect(GetParent(w),&anchor);
        fitWindow(s,anchor);selectMethod(s,0);
        SetFocus(GetDlgItem(w,FirstMethod));
        return FALSE;
    }
    if(!state)return FALSE;
    auto& s=*state;
    switch(msg){
    case WM_SIZE:layout(s);return TRUE;
    case WM_DPICHANGED:{
        s.dpi=HIWORD(wp);fonts(s);fitWindow(s,*reinterpret_cast<RECT*>(lp));layout(s);return TRUE;}
    case WM_DRAWITEM:
        if(wp==Qr){drawQr(s,*reinterpret_cast<DRAWITEMSTRUCT*>(lp));return TRUE;}
        break;
    case WM_NOTIFY:{
        const auto& header=*reinterpret_cast<NMHDR*>(lp);
        if(header.code==NM_CUSTOMDRAW && (header.idFrom==Action || header.idFrom==IDCANCEL ||
           (header.idFrom>=FirstMethod && header.idFrom<=LastMethod))){
            SetWindowLongPtrW(w,DWLP_MSGRESULT,paintButton(s,*reinterpret_cast<NMCUSTOMDRAW*>(lp)));
            return TRUE;
        }
        break;}
    case WM_CTLCOLORDLG:case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:{
        auto dc=reinterpret_cast<HDC>(wp);
        const auto child=reinterpret_cast<HWND>(lp);
        const int id=msg==WM_CTLCOLORDLG?0:GetDlgCtrlID(child);
        SetTextColor(dc,(id==Introduction||id==Hint)?Theme::muted:Theme::text);
        SetBkColor(dc,Theme::bg);SetDCBrushColor(dc,Theme::bg);
        return reinterpret_cast<INT_PTR>(GetStockObject(DC_BRUSH));}
    case WM_COMMAND:{
        const int id=LOWORD(wp);
        if(id==IDCANCEL){EndDialog(w,IDCANCEL);return TRUE;}
        if(id>=FirstMethod && id<=LastMethod && HIWORD(wp)==BN_CLICKED){
            selectMethod(s,id-FirstMethod);return TRUE;
        }
        if(id==Action && HIWORD(wp)==BN_CLICKED){act(s);return TRUE;}
        break;}
    case WM_CLOSE:EndDialog(w,IDCANCEL);return TRUE;
    }
    return FALSE;
}

inline void show(HWND owner) {
    struct alignas(DWORD) Template {DLGTEMPLATE dlg;WORD menu=0,windowClass=0,title=0;} t{};
    t.dlg.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN|DS_MODALFRAME;
    t.dlg.cx=340;t.dlg.cy=250;
    State state;
    if(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),&t.dlg,owner,proc,reinterpret_cast<LPARAM>(&state))==-1)
        MessageBoxW(owner,winError().c_str(),get(Key::Title),MB_OK|MB_ICONERROR);
}
}
