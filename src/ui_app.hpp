#pragma once
#include "demo_resources.hpp"
#include "field_search.hpp"
#include "resource_summary.hpp"
#include "field_favorites.hpp"
#include "save_backup.hpp"
// Native UI. File bytes and history are handled by the independent core modules.
constexpr wchar_t APP_VERSION[] = L"0.21.1";
enum { NAV_BASE=200, HISTORY_UNDO=220, HISTORY_REDO, CONFIRM_SAVE, PATCH_IMPORT,
       PATCH_EXPORT, BOOL_FALSE, BOOL_TRUE, COPY_KEY, BACKUP_REFRESH, BACKUP_FOLDER,
       BACKUP_RESTORE, BACKUP_CANCEL, HELP_LICENSES, HELP_USAGE, RELOAD, LANGUAGE, EXPORT_CSV, REVIEW_REVERT, APPEARANCE,
       FIELD_FAVORITE, FILTER_FAVORITES, CLEAR_FIELD_SEARCH, SUPPORT_AUTHOR };
#include "appearance.hpp"
#include "glass.hpp"
#include "typography.hpp"
#include "sponsor_text.hpp"
struct FieldPageState {
    std::wstring query;
    std::string preferred;
    bool changedOnly=false;
};
struct FieldCaption { std::wstring label, searchText; };
struct App {
    HWND win{},path{},meta{},grid{},search{},name{},desc{},edit{},oldValue{},valueLabel{},notice{},helpText{},empty{};
    std::map<int,HWND> controls;
    HFONT font{},small{},title{},large{},bold{},mono{},brand{},emphasis{},tableNumber{},compactNumber{};
    HBRUSH bg{},panel{},input{}; HIMAGELIST rowHeight{};
    UINT dpi=96; int tab=0; bool populating=false,editingDirty=false,autoCap=true,changedOnly=false,restoreArmed=false;
    bool boolValue=false,licenseMode=false; int gridTop=350,tableW=0,rightX=0; std::wstring noticeText=L"未打开存档";
    fs::path file; int demoResource=0; std::unique_ptr<rizia::Document> doc; std::unique_ptr<rizia::EditSession> session;
    std::vector<std::string> rows; std::string selected;
    std::array<FieldPageState,6> pages;
    std::map<std::string,FieldCaption> captions;
    std::vector<const rizia::Field*> allFields;
    field_favorites::Keys favorites;
    bool favoritesOnly=false;
    std::wstring reviewError;
    size_t previewCount=0;
    std::map<std::string,std::string> review;
    std::vector<save_backup::Item> backups; int backupIndex=-1;
    std::unique_ptr<rizia::Document> backupDoc;
} g;
int px(int n){return MulDiv(n,g.dpi,96);}
void place(HWND h,int x,int y,int w,int he){if(h)MoveWindow(h,px(x),px(y),px(std::max(1,w)),px(std::max(1,he)),TRUE);}
std::map<HWND,std::wstring> uiText;
void setText(HWND h,const std::wstring&s){if(h){uiText[h]=s;SetWindowTextW(h,tr(s).c_str());}}
void rawText(HWND h,const std::wstring&s){if(h)SetWindowTextW(h,s.c_str());}
HWND ctl(int id){auto it=g.controls.find(id);return it==g.controls.end()?nullptr:it->second;}
void visible(HWND h,bool yes){if(h)ShowWindow(h,yes?SW_SHOW:SW_HIDE);}
void visible(int id,bool yes){visible(ctl(id),yes);}
void message(const std::wstring&s,UINT flags=MB_OK|MB_ICONINFORMATION){localizedMessageBox(g.win,s,flags);}
const std::map<std::string,std::string>& pending(){static const std::map<std::string,std::string> empty;return g.session?g.session->pending():empty;}
std::string effective(const std::string&key){auto i=pending().find(key);if(i!=pending().end())return i->second;auto*f=g.doc?g.doc->find(key):nullptr;return f?f->value:"";}
std::wstring val(const char*key){auto v=effective(key);return v.empty()?L"—":wide(v);}
void toast(const std::wstring&s){g.noticeText=s;setText(g.notice,s);InvalidateRect(g.win,nullptr,FALSE);}
void drawText(HDC dc,const std::wstring&s,RECT r,HFONT font,COLORREF color,UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE){
    auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);auto translated=tr(s);DrawTextW(dc,translated.c_str(),(int)translated.size(),&r,flags|DT_END_ELLIPSIS|DT_NOPREFIX);SelectObject(dc,old);
}
RECT rect(int x,int y,int w,int h){return {px(x),px(y),px(x+w),px(y+h)};}
void rounded(HDC dc,RECT r,COLORREF fill,COLORREF border,int radius=10){
    auto b=CreateSolidBrush(fill);auto p=CreatePen(PS_SOLID,1,border);auto ob=SelectObject(dc,b),op=SelectObject(dc,p);
    int corner=px(Theme::radius(radius));
    RoundRect(dc,r.left,r.top,r.right,r.bottom,corner,corner);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);
}
const wchar_t* pageTitle(int);void updateCampaign();
void layout();void refresh();void updateStatus();void showSelected();void loadDocument(const fs::path&,bool=true);
bool applyInput(bool=true);void navigate(int);void refreshBackups();void selectBackup(int);void command(int,int);
std::map<std::string,std::string> preparedChanges();
void updatePreviewCount();

static HWND pointerButton{};
static POINT pointerPosition{};
POINT glassPointer(HWND h){
    if(pointerButton==h)return pointerPosition;
    RECT r{}; GetClientRect(h,&r);
    return {(r.left+r.right)/2,(r.top+r.bottom)/2};
}
LRESULT CALLBACK buttonProc(HWND h,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
    if(m==WM_MOUSEMOVE){
        const bool entering=!GetPropW(h,L"RiziaHover");
        if(entering){
            SetPropW(h,L"RiziaHover",(HANDLE)1);
            TRACKMOUSEEVENT tracking{sizeof(tracking),TME_LEAVE,h,0};
            TrackMouseEvent(&tracking);
        }
        const POINT position{(short)LOWORD(lp),(short)HIWORD(lp)};
        const int threshold=std::max(2,MulDiv(2,GetDpiForWindow(h),96));
        if(entering || (glass::followsPointer() && (pointerButton!=h ||
           std::abs(position.x-pointerPosition.x)>=threshold || std::abs(position.y-pointerPosition.y)>=threshold))){
            pointerButton=h;
            pointerPosition=position;
            InvalidateRect(h,nullptr,FALSE);
        }
    }
    if(m==WM_MOUSELEAVE || m==WM_NCDESTROY){
        RemovePropW(h,L"RiziaHover");
        if(pointerButton==h)pointerButton=nullptr;
        if(m==WM_MOUSELEAVE)InvalidateRect(h,nullptr,FALSE);
    }
    return DefSubclassProc(h,m,wp,lp);
}
HWND control(const wchar_t*cls,const wchar_t*txt,DWORD style,int id=0){
    HWND h=CreateWindowExW(0,cls,tr(txt).c_str(),WS_CHILD|WS_VISIBLE|style,0,0,0,0,g.win,(HMENU)(INT_PTR)id,GetModuleHandleW(nullptr),nullptr);
    uiText[h]=txt;if(id)g.controls[id]=h;SendMessageW(h,WM_SETFONT,(WPARAM)g.font,TRUE);return h;
}
HWND button(const wchar_t*txt,int id){auto h=control(L"BUTTON",txt,WS_TABSTOP|BS_OWNERDRAW,id);SetWindowSubclass(h,buttonProc,1,0);return h;}
#include "workspace_chrome.hpp"
#include "search_control.hpp"
#include "appearance_menu.hpp"
LRESULT CALLBACK headerProc(HWND h,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
    if(m==WM_ERASEBKGND)return 1;
    if(m==HDM_LAYOUT){
        auto result=DefSubclassProc(h,m,wp,lp);
        auto* hd=reinterpret_cast<HDLAYOUT*>(lp);
        if(result&&hd&&hd->pwpos&&hd->prc){
            int extra=px(30)-hd->pwpos->cy;hd->pwpos->cy+=extra;hd->prc->top+=extra;
        }
        return result;
    }
    if(m==WM_PAINT){PAINTSTRUCT ps{};auto dc=BeginPaint(h,&ps);paintWorkspaceHeader(h,dc);EndPaint(h,&ps);return 0;}
    if(m==WM_PRINTCLIENT){paintWorkspaceHeader(h,(HDC)wp);return 0;}
    return DefSubclassProc(h,m,wp,lp);
}
void fonts(){
    HFONT old[]={g.font,g.small,g.title,g.large,g.bold,g.mono,g.brand,g.emphasis,g.tableNumber,g.compactNumber};
    const auto faces=typography::chooseFaces(i18n::language);
    auto make=[](int dip,const wchar_t* face,LONG weight=FW_NORMAL){return typography::make(g.dpi,dip,face,weight);};
    g.font=make(14,faces.text);g.small=make(13,faces.text);
    g.emphasis=make(14,faces.emphasis,faces.emphasisWeight);
    g.title=make(26,faces.emphasis,faces.emphasisWeight);g.bold=make(17,faces.emphasis,faces.emphasisWeight);
    g.large=make(32,typography::numbers);g.compactNumber=make(19,typography::numbers);
    g.tableNumber=make(15,typography::numbers);g.mono=make(20,typography::numbers);
    g.brand=make(21,L"Segoe UI Semibold");
    if(g.win)EnumChildWindows(g.win,[](HWND h,LPARAM){SendMessageW(h,WM_SETFONT,(WPARAM)g.font,TRUE);return TRUE;},0);
    for(HWND h:{g.path,g.meta,g.notice,g.oldValue,g.valueLabel})if(h)SendMessageW(h,WM_SETFONT,(WPARAM)g.small,TRUE);
    if(g.name)SendMessageW(g.name,WM_SETFONT,(WPARAM)g.bold,TRUE);
    if(g.edit)SendMessageW(g.edit,WM_SETFONT,(WPARAM)g.mono,TRUE);
    if(g.grid){
        auto rows=ImageList_Create(1,px(34),ILC_COLOR32,1,1);
        if(rows){ListView_SetImageList(g.grid,rows,LVSIL_SMALL);if(g.rowHeight)ImageList_Destroy(g.rowHeight);g.rowHeight=rows;}
    }
    for(auto h:old)if(h)DeleteObject(h);
}

void updateAppearanceLabel() {
    rawText(ctl(APPEARANCE),std::wstring(Theme::label())+L" · "+Theme::name(Theme::index()));
}
void recolorWorkspace(int index) {
    if(index==Theme::index())return;
    Theme::use(index);
    if(!g.win)return;
    auto replaceBrush=[](HBRUSH& brush,COLORREF color) {
        auto fresh=CreateSolidBrush(color);
        if(fresh){auto old=brush;brush=fresh;if(old)DeleteObject(old);}
    };
    replaceBrush(g.bg,Theme::bg);replaceBrush(g.panel,Theme::panel);replaceBrush(g.input,Theme::input);
    Theme::caption(g.win);
    if(g.grid) {
        SetWindowTheme(g.grid,Theme::dark()?L"DarkMode_Explorer":L"Explorer",nullptr);
        ListView_SetBkColor(g.grid,Theme::panel);
        ListView_SetTextBkColor(g.grid,Theme::panel);
        ListView_SetTextColor(g.grid,Theme::text);
    }
    for(HWND h:{g.desc,g.helpText,ctl(LANGUAGE)})if(h)SetWindowTheme(h,Theme::dark()?L"DarkMode_Explorer":nullptr,nullptr);
    fonts();
    updateAppearanceLabel();
    RedrawWindow(g.win,nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN);
}
void applyAppearance(int index) {
    if(!Theme::valid(index))return;
    if(!Theme::save(index)) {
        message(i18n::language<=1?L"无法保存外观设置。":L"The appearance setting could not be saved.",MB_ICONWARNING);
        return;
    }
    recolorWorkspace(index);
}
void useChineseFont(typography::ChineseFont font) {
    if(typography::selected==font)return;
    typography::selected=font;
    if(!g.win || !typography::isChinese(i18n::language))return;
    fonts();layout();
    RedrawWindow(g.win,nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN);
}
void applyAppearanceCommand(int command) {
    if(command==glass::menuCommand) {
        if(glass::toggle())RedrawWindow(g.win,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
    }else if(command>=Theme::menuBase && command<fontHeadingCommand) {
        applyAppearance(command-Theme::menuBase);
    }else if(command>=firstFontCommand && command<firstFontCommand+typography::fontCount) {
        const auto font=static_cast<typography::ChineseFont>(command-firstFontCommand);
        if(!typography::isChinese(i18n::language) || font==typography::selected)return;
        if(font!=typography::Automatic && !typography::chineseFamilies(i18n::language)[font].text)return;
        if(typography::save(font))useChineseFont(font);
        else message(i18n::language==0?L"无法保存字体设置。":L"無法儲存字體設定。",MB_ICONWARNING);
    }
}
void syncAppearance() {
    const bool oldGlass=glass::enabled();
    const bool oldReflection=glass::followsPointer();
    glass::refreshSettings();
    recolorWorkspace(Theme::stored());useChineseFont(typography::stored());
    if(oldGlass!=glass::enabled() || oldReflection!=glass::followsPointer())
        RedrawWindow(g.win,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
}
void updateStatus(){
    bool loaded=bool(g.doc),any=!pending().empty()||g.editingDirty;
    for(int id:std::initializer_list<int>{PRESET99,PRESET999,SAVE,PATCH_IMPORT,AUTOCAP,RELOAD})EnableWindow(ctl(id),loaded);
    const bool canWrite=loaded&&!g.review.empty()&&g.reviewError.empty()&&g.tab==5;
    EnableWindow(ctl(CONFIRM_SAVE),canWrite&&!g.demoResource);
    EnableWindow(ctl(SAVEAS),canWrite);
    EnableWindow(ctl(PATCH_EXPORT),canWrite);
    EnableWindow(ctl(UNDOALL),!pending().empty());
    EnableWindow(ctl(HISTORY_UNDO),g.session&&(g.session->canUndo()||g.editingDirty));
    EnableWindow(ctl(HISTORY_REDO),g.session&&g.session->canRedo()&&!g.editingDirty);
    EnableWindow(ctl(BACKUP_RESTORE),g.backupDoc!=nullptr);EnableWindow(ctl(BACKUP_FOLDER),loaded&&!g.demoResource);
    setText(ctl(NAV_BASE+5),L"修改预览"+(!g.previewCount?L"":L"  "+std::to_wstring(g.previewCount)));
    SetWindowTextW(g.win,((any?L"* ":L"")+std::wstring(APP_TITLE)+L"").c_str());
    InvalidateRect(ctl(APPLY),nullptr,FALSE);InvalidateRect(ctl(SAVE),nullptr,FALSE);
    for(int i=0;i<8;++i)InvalidateRect(ctl(NAV_BASE+i),nullptr,FALSE);InvalidateRect(g.win,nullptr,FALSE);
}
void setColumns(const std::vector<std::pair<std::wstring,int>>&columns){
    // Keep the native header and its four columns alive across page changes.
    // Hiding an unused fourth column avoids rebuilding header internals during input.
    auto header=ListView_GetHeader(g.grid);int count=header?Header_GetItemCount(header):0;
    for(int i=std::max(0,count);i<4;++i){LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;c.pszText=const_cast<wchar_t*>(L"");c.cx=0;ListView_InsertColumn(g.grid,i,&c);}
    for(size_t i=0;i<4;++i){LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;auto title=i<columns.size()?tr(columns[i].first):L"";c.pszText=title.data();c.cx=i<columns.size()?px(columns[i].second):0;ListView_SetColumn(g.grid,(int)i,&c);}
    header=ListView_GetHeader(g.grid);if(header)SetWindowSubclass(header,headerProc,1,0);
}
void rowText(int r,int c,const std::wstring&s){auto translated=tr(s);ListView_SetItemText(g.grid,r,c,translated.data());}
void addRow(int index,const std::vector<std::wstring>&values){
    if(values.empty())return;
    LVITEMW item{};item.mask=LVIF_TEXT;item.iItem=index;item.pszText=const_cast<wchar_t*>(values[0].c_str());
    ListView_InsertItem(g.grid,&item);
    for(size_t i=1;i<values.size();++i)ListView_SetItemText(g.grid,index,(int)i,const_cast<wchar_t*>(values[i].c_str()));
}
void showSelected(){
    g.populating=true;g.editingDirty=false;
    const auto*f=g.doc?g.doc->find(g.selected):nullptr;
    if(!f){setText(g.name,L"选择项目");setText(g.desc,L"");setText(g.oldValue,L"");setText(g.edit,L"");}
    else {
        setText(g.name,label(f->key));setText(g.oldValue,L"原值   "+displayValue(f->value,f->is_bool,i18n::language));
        auto*d=definition(f->key);std::wstring detail=d?d->help:L"仅修改此字段，剧情条件不联动。";
        if(f->key.find("Army_Unit_")!=std::string::npos)detail=L"部队状态与部署、阵亡、训练和战棋数据相互关联。军备库存页更适合常规修改。";
        if(f->key.find("ModifiersTotal")!=std::string::npos)detail=L"这是游戏计算的合计值，事件发生时可能重算。修改它不能锁定每回合收入。";
        for (const auto& resource : riziaResources) {
            if (f->key == resource.turnTotal) {
                const bool hasModifiers = std::any_of(g.doc->fields.begin(), g.doc->fields.end(),
                    [&](const auto& item) { return item.key.rfind(resource.modifierPrefix, 0) == 0; });
                detail += hasModifiers
                    ? L"\r\n\r\n在高级字段中搜索以下前缀，可查看收支明细。\r\n" + wide(resource.modifierPrefix)
                    : L"\r\n\r\n这份存档未列出该资源的收支明细。";
                break;
            }
        }
        if(g.tab==4)detail+=L"\r\n\r\n"+wide(f->key);
        setText(g.desc,detail);
        if(f->is_bool)g.boolValue=effective(f->key)=="true";else setText(g.edit,wide(effective(f->key)));
    }
    bool editPage=g.tab<5,has=editPage&&f;
    visible(g.edit,editPage&&(!f||!f->is_bool));visible(BOOL_FALSE,has&&f->is_bool);visible(BOOL_TRUE,has&&f->is_bool);
    for(int id:std::initializer_list<int>{APPLY,UNDOONE,COPY_KEY,FIELD_FAVORITE})EnableWindow(ctl(id),has);EnableWindow(g.edit,has);
    setText(ctl(FIELD_FAVORITE),has&&g.favorites.count(f->key)?L"取消收藏":L"收藏字段");
    InvalidateRect(ctl(FIELD_FAVORITE),nullptr,FALSE);
    rawText(ctl(BOOL_FALSE),displayValue("false",true,i18n::language));rawText(ctl(BOOL_TRUE),displayValue("true",true,i18n::language));
    InvalidateRect(ctl(BOOL_FALSE),nullptr,FALSE);InvalidateRect(ctl(BOOL_TRUE),nullptr,FALSE);
    g.populating=false;updateStatus();
}
bool applyInput(bool redraw){
    if(!g.doc||g.selected.empty()||!g.editingDirty||g.tab>=5)return true;
    const auto* f=g.doc->find(g.selected);if(!f)return false;
    try{
        std::string value=f->is_bool?(g.boolValue?"true":"false"):utf8(trim(textOf(g.edit)));
        g.doc->validateChanges({{f->key,value}});auto*d=definition(f->key);
        if(d&&d->nonnegative){if(value.empty()||!std::all_of(value.begin(),value.end(),[](char c){return c>='0'&&c<='9';}))throw WinFailure(L"资源、上限和军需请输入 0 或正整数，不使用小数、符号或科学计数法。");value=std::to_string(std::stoll(value));}
        g.session->applyBatch({{f->key,value}});g.editingDirty=false;
        if(redraw)refresh();else {for(size_t i=0;i<g.rows.size();++i)if(g.rows[i]==f->key)rowText((int)i,2,pending().count(f->key)?displayValue(value,f->is_bool,i18n::language):L"—");updatePreviewCount();updateStatus();}
        return true;
    }catch(const std::exception&e){
        HWND input=f->is_bool?ctl(g.boolValue?BOOL_TRUE:BOOL_FALSE):g.edit;
        message(L"这个值还没有应用。\r\n\r\n"+wide(e.what()),MB_OK|MB_ICONWARNING);
        SetFocus(input);
        return false;
    }
}
bool discardIfNeeded(){
    return (pending().empty()&&!g.editingDirty)||localizedMessageBox(g.win,L"还有未保存的修改。放弃这些修改并继续吗？",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES;
}
rizia::ChangeMap linkedCapChanges(){
    rizia::ChangeMap caps;
    if(!g.doc||isBase()||pending().empty())return caps;
    for(const char* name:{"Budget","Authority","Energy"}){
        const auto stock=std::string("RiziaDLC.Resources_")+name;
        const auto cap=std::string("RiziaDLC.Rizia_HUDStat_")+name+"_Max";
        if(!pending().count(stock)&&!pending().count(cap))continue;
        const auto stockValue=effective(stock),capValue=effective(cap);
        if(stockValue.empty()||capValue.empty()||std::stod(stockValue)<=std::stod(capValue))continue;
        if(pending().count(cap))throw WinFailure(label(cap)+L"不能低于当前或待保存的库存。请调整库存或上限。");
        if(!g.autoCap)throw WinFailure(label(stock)+L"超过上限。请开启自动同步上限，或手动修改上限。");
        caps[cap]=stockValue;
    }
    return caps;
}
void updatePreviewCount(){
    g.previewCount=pending().size();
    try{g.previewCount+=linkedCapChanges().size();}
    catch(const std::exception&){/* A conflicting review displays explicit edits for correction. */}
}
std::map<std::string,std::string> preparedChanges(){
    if(!g.doc)return {};
    auto changes=pending();
    const auto caps=linkedCapChanges();
    changes.insert(caps.begin(),caps.end());
    g.doc->validateChanges(changes);return changes;
}
rizia::ChangeMap reviewChanges(std::wstring* error=nullptr){
    if(error)error->clear();
    try{return preparedChanges();}
    catch(const std::exception& e){
        // A conflicting draft can still be reviewed and exported with its explicit edits.
        if(error)*error=tr(wide(e.what()));
        return pending();
    }
}
std::wstring formatLocalStamp(const std::wstring& stamp){
    if(stamp.empty())return {};
    return stamp.substr(0,4)+L"-"+stamp.substr(4,2)+L"-"+stamp.substr(6,2)+L"  "+stamp.substr(9,2)+L":"+stamp.substr(11,2)+L":"+stamp.substr(13,2);
}
std::wstring fileTimeText(const fs::path&p,std::wstring stamp={}){
    if(stamp.empty())stamp=save_backup::lastWriteStamp(p);
    return stamp.empty()?L"未知时间":formatLocalStamp(stamp);
}
void selectBackup(int index){
    g.restoreArmed=false;g.backupIndex=index;g.backupDoc.reset();
    if(index>=0&&index<(int)g.backups.size()){
        try{auto doc=std::make_unique<rizia::Document>(rizia::Document::parse(readFile(g.backups[index].path)));
            if(doc->story!=g.doc->story)throw WinFailure(L"Campaign mismatch.");g.backupDoc=std::move(doc);
            setText(g.name,L"备份详情");setText(g.oldValue,fileTimeText(g.backups[index].path,g.backups[index].timestamp));
            auto bv=[](const char*k){auto*f=g.backupDoc->find(k);return f?wide(f->value):L"—";};
            std::wstring s=L"将恢复到当前打开的存档。\r\n恢复前会再次备份当前文件。\r\n\r\n回合  "+wide(g.backupDoc->turn)+L"   /   "+wide(g.backupDoc->version);
            for(const std::string& k:(isBase()?std::vector<std::string>{"BaseGame.GovernmentBudget","BaseGame.PersonalWealth","BaseGame.Economy"}:std::vector<std::string>{"RiziaDLC.Resources_Budget","RiziaDLC.Resources_Authority","RiziaDLC.Resources_Energy"})){s+=L"\r\n"+label(k)+L"    "+wide(g.doc->find(k)?g.doc->find(k)->value:"—")+L"  →  "+bv(k.c_str());}
            if(g.backupDoc->campaignName!=g.doc->campaignName)s+=L"\r\n\r\n注意：战役名称与当前存档不同。";
            s+=L"\r\n\r\n"+g.backups[index].path.filename().wstring();setText(g.desc,s);
        }catch(const std::exception&e){setText(g.name,L"无法读取这份备份");setText(g.oldValue,L"");setText(g.desc,wide(e.what()));}
    }else{setText(g.name,L"选择一份备份");setText(g.oldValue,L"");setText(g.desc,L"");}
    setText(ctl(BACKUP_RESTORE),L"恢复这份备份");visible(BACKUP_CANCEL,false);updateStatus();
}
void refreshBackups(){
    fs::path keep=(g.backupIndex>=0&&g.backupIndex<(int)g.backups.size())?g.backups[g.backupIndex].path:fs::path();
    g.backups.clear();g.backupIndex=-1;g.backupDoc.reset();
    if(g.doc&&!g.demoResource)g.backups=save_backup::list(g.file,MAX_FILE);
    g.populating=true;ListView_DeleteAllItems(g.grid);g.rows.clear();setColumns({{L"备份时间",180},{L"大小",72},{L"原档备份",std::max(170,g.tableW-260)}});
    for(size_t i=0;i<g.backups.size();++i){const auto&b=g.backups[i];addRow((int)i,{fileTimeText(b.path,b.timestamp),std::to_wstring((b.size+1023)/1024)+L" KB",b.path.filename().wstring()});if(b.path==keep)g.backupIndex=(int)i;}
    if(g.backupIndex<0&&!g.backups.empty())g.backupIndex=0;
    if(g.backupIndex>=0){
        ListView_SetItemState(g.grid,g.backupIndex,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
        ListView_EnsureVisible(g.grid,g.backupIndex,FALSE);
    }
    g.populating=false;selectBackup(g.backupIndex);
}
void resetFieldBrowser(){
    g.pages={};
    g.captions.clear();
    g.allFields.clear();
    g.review.clear();
    g.reviewError.clear();
    g.selected.clear();
}

void appendFieldRows(){
    if(!g.doc)return;
    const auto& query=g.pages[g.tab].query;
    const auto key=field_search::pastedKey(query);
    const auto terms=key.text.empty()?field_search::terms(query):std::vector<std::wstring>{};
    const auto number=tr(L"数值"), toggle=tr(L"开关");
    auto append=[&](const rizia::Field& field){
        if(!key.text.empty() && !key.matches(field.key))return;
        if(g.tab==4&&g.favoritesOnly&&!g.favorites.count(field.key))return;
        const auto change=pending().find(field.key);
        if(g.changedOnly&&change==pending().end())return;
        auto& caption=g.captions[field.key];
        if(caption.label.empty())caption.label=label(field.key);
        if(!terms.empty()){
            if(caption.searchText.empty()){
                auto search=caption.label+L"|"+wide(field.key);
                const auto aliases = resourceSearchAliases(field.key);
                if (!aliases.empty()) {
                    search += L"|" + aliases;
                    if (i18n::language == 1) search += L"|" + i18n::traditional(aliases);
                }
                if(i18n::language!=0){
                    const auto* def=definition(field.key);
                    search+=L"|"+(def?std::wstring(fieldZh(def->label)):advancedZhLabel(field.key));
                }
                caption.searchText=field_search::fold(search);
            }
            if(!field_search::matches(caption.searchText,terms))return;
        }
        const auto staged=change==pending().end()?L"—":displayValue(change->second,field.is_bool,i18n::language);
        g.rows.push_back(field.key);
        addRow((int)g.rows.size()-1,{(g.favorites.count(field.key)?L"★  ":L"")+caption.label,displayValue(field.value,field.is_bool,i18n::language),
                                    staged,field.is_bool?toggle:number});
    };
    if(g.tab==4){
        if(!key.text.empty() && !key.prefix){
            if(const auto* field=g.doc->find(key.text)){append(*field);return;}
        }
        if(g.allFields.empty()){
            for(const auto& field:g.doc->fields)g.allFields.push_back(&field);
            std::sort(g.allFields.begin(),g.allFields.end(),[](const auto* a,const auto* b){return a->key<b->key;});
        }
        std::set<const rizia::Field*> common;
        if(!terms.empty()){
            for(const auto& def:currentDefs())if(const auto* field=g.doc->find(def.key)){
                common.insert(field);
                append(*field);
            }
        }
        for(const auto* field:g.allFields)if(!common.count(field))append(*field);
    }else{
        for(const auto& def:currentDefs())if(def.tab==g.tab){
            if(const auto* field=g.doc->find(def.key))append(*field);
        }
    }
}

std::string reviewRevertKey(){
    if(!g.session||g.tab!=5||!g.review.count(g.selected))return {};
    if(pending().count(g.selected))return g.selected;
    // An automatically derived cap follows one stock edit; undoing its source
    // removes both the stock change and the cap from the effective preview.
    for(const char* name:{"Budget","Authority","Energy"}){
        const auto cap=std::string("RiziaDLC.Rizia_HUDStat_")+name+"_Max";
        const auto stock=std::string("RiziaDLC.Resources_")+name;
        if(g.selected==cap&&pending().count(stock))return stock;
    }
    return {};
}

void showReviewSelection(){
    const auto target=reviewRevertKey();
    const bool linked=!target.empty()&&target!=g.selected;
    setText(ctl(REVIEW_REVERT),linked?L"撤回关联库存":L"撤回此项");
    EnableWindow(ctl(REVIEW_REVERT),!target.empty());
    setText(g.name,g.review.empty()?L"没有待保存项":std::to_wstring(g.review.size())+L" 项即将保存");
    if(g.selected.empty())setText(g.oldValue,L"");
    else rawText(g.oldValue,label(g.selected));
    const auto targetText=g.demoResource?tr(L"示例只能另存为新文件。"):tr(L"目标：")+L"\r\n"+(g.doc?g.file.wstring():tr(L"未打开"))+L"\r\n\r\n"+tr(L"保存前自动备份。");
    if(!g.reviewError.empty())rawText(g.desc,g.reviewError+L"\r\n\r\n"+targetText);
    else if(linked)rawText(g.desc,tr(L"此上限随库存自动调整。撤回关联库存可取消这两项修改。")+L"\r\n\r\n"+targetText);
    else rawText(g.desc,targetText);
}

void refresh(){
    if(g.tab!=5)updatePreviewCount();
    if(g.tab==6){refreshBackups();layout();return;}
    if(g.tab==7){layout();updateStatus();return;}
    const int previous=ListView_GetNextItem(g.grid,-1,LVNI_SELECTED);
    g.populating=true;SendMessageW(g.grid,WM_SETREDRAW,FALSE,0);ListView_DeleteAllItems(g.grid);g.rows.clear();
    if(g.tab==5){
        g.review=reviewChanges(&g.reviewError);
        g.previewCount=g.review.size();
        setColumns({{L"将修改的项目",std::max(220,g.tableW-210)},{L"原值",95},{L"保存为",105}});
        for(const auto&[k,v]:g.review){g.rows.push_back(k);addRow((int)g.rows.size()-1,{label(k),displayValue(g.doc->find(k)->value,g.doc->find(k)->is_bool,i18n::language),displayValue(v,g.doc->find(k)->is_bool,i18n::language)});}
    }else{
        setColumns({{L"项目",std::max(205,g.tableW-230)},{L"原值",82},{L"待保存",88},{L"类型",56}});
        appendFieldRows();
    }
    auto& state=g.pages[g.tab];
    auto keep=std::find(g.rows.begin(),g.rows.end(),state.preferred);
    if(keep==g.rows.end())keep=std::find(g.rows.begin(),g.rows.end(),g.selected);
    if(keep==g.rows.end()&&!g.rows.empty()){
        const int next=g.tab==5?std::clamp(previous,0,(int)g.rows.size()-1):0;
        keep=g.rows.begin()+next;
    }
    g.selected=keep==g.rows.end()?"":*keep;
    if(state.preferred.empty())state.preferred=g.selected;
    if(keep!=g.rows.end()){
        const auto at=(int)(keep-g.rows.begin());
        ListView_SetItemState(g.grid,at,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
        ListView_EnsureVisible(g.grid,at,FALSE);
    }
    SendMessageW(g.grid,WM_SETREDRAW,TRUE,0);InvalidateRect(g.grid,nullptr,TRUE);g.populating=false;
    if(g.tab<5)showSelected();else showReviewSelection();
    layout();updateStatus();
}
void setHelp(){
    std::wstring s;
    if(g.licenseMode){auto inst=GetModuleHandleW(nullptr);auto r=FindResourceW(inst,MAKEINTRESOURCEW(201),RT_RCDATA);if(r){auto data=LoadResource(inst,r);auto p=(const char*)LockResource(data);if(p)s=wide(std::string(p,SizeofResource(inst,r)));}}
    else s=L"使用\r\n\r\n"
        L"选择存档 → 修改数值 → 修改预览 → 保存。\r\n"
        L"保存前退出游戏，保存后在游戏中重新读档。\r\n"
        L"备份可在“备份管理”中恢复。\r\n\r\n"
        L"快捷键\r\n\r\n"
        L"Ctrl+L  选择存档     Ctrl+O  手动打开\r\n"
        L"Ctrl+S  修改预览     Ctrl+F  搜索字段\r\n"
        L"Ctrl+R  重新载入     Ctrl+Z / Y  撤销 / 重做\r\n"
        L"Enter  应用输入\r\n\r\n"
        L"0.21.1 Preview · Windows 10 / 11 x64\r\n"
        L"未完成游戏读档验证。";
    std::wstring crlf;for(wchar_t c:s){if(c==L'\n'&&(crlf.empty()||crlf.back()!=L'\r'))crlf+=L'\r';crlf+=c;}setText(g.helpText,crlf);SendMessageW(g.helpText,EM_SETSEL,0,0);SendMessageW(g.helpText,EM_SCROLLCARET,0,0);
}
void navigate(int tab){
    if(tab<0||tab>7||!applyInput(false))return;
    g.tab=tab;g.restoreArmed=false;g.editingDirty=false;
    if(tab<=5)g.selected=g.pages[tab].preferred;
    if(tab<5){
        g.populating=true;rawText(g.search,g.pages[tab].query);g.populating=false;
        g.changedOnly=g.pages[tab].changedOnly;
    }
    if(tab==7)setHelp();
    refresh();
    if(tab==5)SetFocus(g.rows.empty()?ctl(NAV_BASE+5):g.grid);
}
std::wstring documentTitle(){
    return g.demoResource?tr(L"内置练习示例")+L"  /  "+campaignLabel(g.doc->story):g.file.wstring();
}
void loadDocumentBytes(const std::string& bytes,const fs::path& path,int resource){
    auto doc=std::make_unique<rizia::Document>(rizia::Document::parse(bytes));
    if(!rizia::isSupportedStory(doc->story))throw WinFailure(L"Unsupported campaign.");
    auto session=std::make_unique<rizia::EditSession>(*doc);
    activeStory=doc->story;
    g.session=std::move(session);g.doc=std::move(doc);g.file=path;g.demoResource=resource;
    InvalidateRect(ctl(LATEST),nullptr,FALSE);
    resetFieldBrowser();
    g.favorites.clear();g.favoritesOnly=false;
    field_favorites::read(activeStory,g.favorites);
    g.selected.clear();g.editingDirty=false;g.backupDoc.reset();g.backups.clear();g.backupIndex=-1;
    g.populating=true;setText(g.search,L"");g.changedOnly=false;g.populating=false;
    rawText(g.path,documentTitle());rawText(g.meta,campaignLabel(g.doc->story)+L"    /    "+wide(g.doc->version)+L"    /    "+tr(L"回合")+L" "+wide(g.doc->turn)+(g.doc->campaignName.empty()?L"":L"    /    "+wide(g.doc->campaignName)));
    updateCampaign();if(g.tab>=5)g.tab=0;
    refresh();
}
void loadDocument(const fs::path&p,bool ask){
    if(ask&&!discardIfNeeded())return;
    try{
        loadDocumentBytes(readFile(p),fs::absolute(p),0);
        toast(L"已打开  ·  "+g.file.filename().wstring());
    }catch(const std::exception&e){message(L"无法打开这个存档。\r\n\r\n"+wide(e.what()),MB_OK|MB_ICONERROR);}
}
void loadDemo(int resource){
    if(!discardIfNeeded())return;
    loadDocumentBytes(demo::bytes(resource),{},resource);
    toast(L"示例只能另存为新文件。");
}
fs::path pickFile(bool save=false,int kind=0,HWND owner=nullptr){
    std::vector<wchar_t> buffer(32768,0);std::wstring initialName;
    if(save)initialName=kind==1?L"Suzerain-custom.patch.txt":(g.demoResource?(isBase()?L"Sordland-demo.json":L"Rizia-demo.json"):g.doc?g.file.stem().wstring()+L".edited.json":L"Suzerain.edited.json");
    if(!initialName.empty())std::copy(initialName.begin(),initialName.end(),buffer.begin());
    fs::path initial=!g.file.empty()?g.file.parent_path():saveFolder();
    if(save&&g.demoResource){
        PWSTR documents=nullptr;
        if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents,0,nullptr,&documents))){initial=documents;CoTaskMemFree(documents);}
        else initial.clear();
    }
    OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=owner?owner:g.win;o.lpstrFile=buffer.data();o.nMaxFile=(DWORD)buffer.size();
    o.lpstrFilter=kind==1?L"Suzerain patch (*.txt)\0*.txt\0All files\0*.*\0\0":L"Suzerain save (*.json)\0*.json\0All files\0*.*\0\0";
    auto dialogTitle=tr(kind==1?(save?L"导出修改方案":L"导入修改方案"):(save?L"另存修改后的存档":L"选择存档"));o.lpstrTitle=dialogTitle.c_str();
    o.lpstrInitialDir=initial.c_str();o.lpstrDefExt=save?(kind==1?L"txt":L"json"):nullptr;o.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    BOOL ok=save?GetSaveFileNameW(&o):GetOpenFileNameW(&o);return ok?fs::path(buffer.data()):fs::path();
}
#include "sponsor_dialog.hpp"
#include "save_picker.hpp"
void preset(int number){
    if(!g.doc||!applyInput(false))return;std::map<std::string,std::string> batch;
    if(isBase()){
        std::string key=number==99?"BaseGame.GovernmentBudget":"BaseGame.PersonalWealth";
        if(g.doc->find(key)){std::ostringstream value;value<<std::setprecision(15)<<(std::stod(effective(key))+5);batch[key]=value.str();}
    }else
    for(const char*n:{"Budget","Authority","Energy"}){
        const auto key=std::string("RiziaDLC.Resources_")+n;
        if(g.doc->find(key))batch[key]=std::to_string(number);
    }
    if(!g.session->applyBatch(batch)){toast(L"没有可调整的数值");return;}
    g.pages[0].query.clear();
    g.pages[0].changedOnly=false;
    navigate(0);toast(L"资源已调整，待保存");
}
void saveDocument(bool as){
    if(!g.doc||!applyInput(false))return;
    if(g.tab!=5){navigate(5);return;}
    try{
        auto changes=preparedChanges();if(changes.empty()){toast(L"没有需要保存的修改");return;}
        fs::path dest=(as||g.demoResource)?pickFile(true):g.file;if(dest.empty())return;
        if(g.demoResource){
            auto bytes=g.doc->render(changes);
            atomicSave(dest,bytes,nullptr);
            loadDocumentBytes(bytes,{},g.demoResource);
            toast(L"示例副本已保存。");
            return;
        }
        std::string destinationBytes;
        const std::string* expected=nullptr;
        if(samePath(dest,g.file)){
            expected=&g.doc->original;
        }else if(fs::exists(dest)){
            destinationBytes=readFile(dest);
            expected=&destinationBytes;
        }
        auto bytes=g.doc->render(changes);
        atomicSave(dest,bytes,expected);
        loadDocument(dest,false);toast(as?L"已另存。游戏未显示自定义文件名时，请使用原存档名称和位置。":L"已保存并校验备份。请回到游戏主菜单，手动读取这份存档。");
    }catch(const std::exception&e){message(L"保存没有完成。\r\n\r\n"+wide(e.what()),MB_OK|MB_ICONERROR);}
}
void restoreSelected(){
    if(!g.doc||!g.backupDoc||g.backupIndex<0)return;
    if(!g.restoreArmed){if(!discardIfNeeded())return;g.restoreArmed=true;setText(ctl(BACKUP_RESTORE),L"确认恢复并备份当前档");visible(BACKUP_CANCEL,true);
        toast(L"再次确认后，将用选中备份替换当前文件；当前版本仍会另做备份。");InvalidateRect(g.win,nullptr,FALSE);return;}
    try{auto data=readFile(g.backups[g.backupIndex].path);if(data!=g.backupDoc->original)throw WinFailure(L"这份备份在预览后发生了变化。请刷新列表，重新确认后再恢复。");auto d=rizia::Document::parse(data);if(d.story!=g.doc->story)throw WinFailure(L"Campaign mismatch.");
        atomicSave(g.file,data,&g.doc->original);auto target=g.file;g.restoreArmed=false;loadDocument(target,false);toast(L"备份已恢复。恢复前的版本也已保存，可在列表中回退。");
    }catch(const std::exception&e){
        g.restoreArmed=false;setText(ctl(BACKUP_RESTORE),L"恢复这份备份");visible(BACKUP_CANCEL,false);
        message(L"恢复没有完成。\r\n\r\n"+wide(e.what()),MB_OK|MB_ICONERROR);
    }
}
void writePatch(const fs::path&p,const std::string&data){
    if(isBackupDestination(p))throw WinFailure(tr(L"请选择备份目录以外的位置导出文件。"));
    auto temp=p.parent_path()/(L".rizia-patch-"+writeId()+L".tmp");
    bool created=false;
    try{
        {Handle h{CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr)};
        if(h.h==INVALID_HANDLE_VALUE)throw WinFailure(winError());
        created=true;DWORD wrote=0;
        if(!WriteFile(h.h,data.data(),(DWORD)data.size(),&wrote,nullptr)||wrote!=data.size()||!FlushFileBuffers(h.h))throw WinFailure(L"方案写入失败："+winError());}
        if(readFile(temp)!=data)throw WinFailure(L"方案写入校验失败。");
        if(!MoveFileExW(temp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw WinFailure(winError());
    }catch(...){if(created)DeleteFileW(temp.c_str());throw;}
}
void patchFile(bool exportIt){
    if(!g.doc||!applyInput(false))return;
    try{auto p=pickFile(exportIt,1);if(p.empty())return;
        if(exportIt){auto changes=preparedChanges();if(changes.empty()){toast(L"没有可导出的修改，请先编辑项目。");return;}
            if(lower(p.extension().wstring())!=L".txt"||samePath(p,g.file))throw WinFailure(L"请用 .txt 文件导出方案，不能覆盖游戏存档或备份目录。");
            writePatch(p,rizia::serializePatch(changes,g.doc->story));toast(L"修改方案已导出，只包含字段和值。");
        }else{
            const auto changes=rizia::parsePatch(readFile(p),g.doc->story);
            for(const auto& [key,value]:changes){
                const auto* field=definition(key);
                if(field&&field->nonnegative&&(value.empty()||!std::all_of(value.begin(),value.end(),[](char c){return c>='0'&&c<='9';})))
                    throw WinFailure(L"请输入非负整数。\r\n"+wide(key));
            }
            g.session->applyBatch(changes);
            navigate(5);
            toast(L"方案已导入，待保存");
        }
    }catch(const std::exception&e){message(L"修改方案没有处理完成。\r\n\r\n"+wide(e.what()),MB_OK|MB_ICONWARNING);}
}
void copyKey(){
    if(g.selected.empty()||!OpenClipboard(g.win))return;auto s=wide(g.selected);auto memory=GlobalAlloc(GMEM_MOVEABLE,(s.size()+1)*sizeof(wchar_t));
    if(memory){auto p=GlobalLock(memory);if(p){memcpy(p,s.c_str(),(s.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);EmptyClipboard();if(!SetClipboardData(CF_UNICODETEXT,memory))GlobalFree(memory);}else GlobalFree(memory);}
    CloseClipboard();toast(L"字段名已复制");
}
const wchar_t* pageTitle(int page){
    const wchar_t*r[]={L"资源与经济",L"军备库存",L"家族与人物",L"外交谈判",L"高级字段",L"修改预览",L"备份管理",L"使用指南"};
    const wchar_t*b[]={L"Economy",L"Politics",L"People",L"Diplomacy & military",L"高级字段",L"修改预览",L"备份管理",L"使用指南"};
    return isBase()?b[page]:r[page];
}
void updateCampaign(){
    for(int i=0;i<8;++i)setText(ctl(NAV_BASE+i),pageTitle(i));
    setText(ctl(PRESET99),isBase()?L"Budget +5":L"资源设为 99");
    setText(ctl(PRESET999),isBase()?L"Wealth +5":L"资源设为 999");
}
void changeLanguage(){
    if(!applyInput(false)){SendMessageW(ctl(LANGUAGE),CB_SETCURSEL,i18n::language,0);return;}
    auto language=(int)SendMessageW(ctl(LANGUAGE),CB_GETCURSEL,0,0);i18n::setLanguage(language);i18n::savePreferences();fonts();
    g.captions.clear();
    g.populating=true;
    for(const auto& item:uiText)if(IsWindow(item.first)&&IsChild(g.win,item.first)&&item.first!=g.edit&&item.first!=g.search&&item.first!=g.path&&item.first!=g.meta&&item.first!=ctl(LANGUAGE))SetWindowTextW(item.first,tr(item.second).c_str());
    g.populating=false;
    SendMessageW(g.search,EM_SETCUEBANNER,TRUE,(LPARAM)tr(L"搜索项目或存档字段").c_str());
    updateAppearanceLabel();
    updateCampaign();if(g.doc){rawText(g.path,documentTitle());rawText(g.meta,campaignLabel(g.doc->story)+L" / "+wide(g.doc->version)+L" / "+tr(L"回合")+L" "+wide(g.doc->turn)+L" / "+wide(g.doc->campaignName));}
    rawText(ctl(SUPPORT_AUTHOR),sponsor_text::get(sponsor_text::Key::Title));
    if(g.tab==7)setHelp();refresh();
}
std::string renderFieldCsv(const rizia::Document& doc,const rizia::ChangeMap& changes){
    auto cell=[](const std::string& value,bool numeric=false){
        std::string out="\"";
        const auto first=value.find_first_not_of(' ');
        if(first!=value.npos && std::string_view("=+-@\t\r\n").find(value[first])!=std::string_view::npos &&
           !(numeric && rizia::detail::validNumberSyntax(value)))out+="'";
        for(char c:value){
            if(c=='"')out+='"';
            out+=c;
        }
        return out+'"';
    };
    std::string data="\xEF\xBB\xBF" "Campaign,Field,Label,Original,Current,Type\r\n";
    for(const auto& field:doc.fields){
        const auto edit=changes.find(field.key);
        const auto& current=edit==changes.end()?field.value:edit->second;
        const bool numeric=!field.is_bool;
        data+=cell(doc.story)+","+cell(field.key)+","+cell(utf8(label(field.key)))+","+
              cell(field.value,numeric)+","+cell(current,numeric)+","+cell(numeric?"number":"boolean")+"\r\n";
    }
    return data;
}
void writeFieldCsv(const fs::path& path){
    if(lower(path.extension().wstring())!=L".csv"||samePath(path,g.file))throw WinFailure(L"Choose a .csv file outside the backup folder.");
    writePatch(path,renderFieldCsv(*g.doc,reviewChanges()));
}
void exportCSV(){
    if(!g.doc||!applyInput(false))return;
    std::vector<wchar_t> buffer(32768);std::wstring name=L"Suzerain-fields.csv";std::copy(name.begin(),name.end(),buffer.begin());
    OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=g.win;o.lpstrFile=buffer.data();o.nMaxFile=(DWORD)buffer.size();o.lpstrFilter=L"CSV (*.csv)\0*.csv\0\0";o.lpstrDefExt=L"csv";o.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(!GetSaveFileNameW(&o))return;
    writeFieldCsv(fs::path(buffer.data()));toast(L"CSV exported.");
}
#include "workspace_view.hpp"
LRESULT CALLBACK editProc(HWND w,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
    if(m==WM_GETDLGCODE){auto flags=DefSubclassProc(w,m,wp,lp);auto*msg=(MSG*)lp;return (wp==VK_RETURN||(msg&&msg->wParam==VK_RETURN))?flags|DLGC_WANTALLKEYS:flags;}
    if(m==WM_KEYDOWN&&wp==VK_RETURN){applyInput();return 0;}return DefSubclassProc(w,m,wp,lp);
}
LRESULT CALLBACK fieldGridProc(HWND grid,UINT message,WPARAM key,LPARAM detail,UINT_PTR,DWORD_PTR){
    const auto result=DefSubclassProc(grid,message,key,detail);
    if(message==WM_GETDLGCODE && detail && key==VK_RETURN && g.tab<5)
        return result|DLGC_WANTMESSAGE;
    return result;
}
void createControls(){
    auto languages=control(L"COMBOBOX",L"",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,LANGUAGE);for(const auto*name:i18n::names)SendMessageW(languages,CB_ADDSTRING,0,(LPARAM)name);SendMessageW(languages,CB_SETCURSEL,i18n::language,0);
    SetWindowSubclass(languages,languageFaceProc,1,0);
    button(L"",APPEARANCE);updateAppearanceLabel();
    button(L"手动打开",OPEN);button(L"选择存档",LATEST);button(L"文件夹",FOLDER);button(L"重新载入",RELOAD);
    const wchar_t*nav[]={L"资源与经济",L"军备库存",L"家族与人物",L"外交谈判",L"高级字段",L"修改预览",L"备份管理",L"使用指南"};
    for(int i=0;i<8;++i)button(nav[i],NAV_BASE+i);
    rawText(button(L"",SUPPORT_AUTHOR),sponsor_text::get(sponsor_text::Key::Title));
    g.path=control(L"STATIC",L"未打开存档",SS_PATHELLIPSIS|SS_NOTIFY|SS_NOPREFIX);g.meta=control(L"STATIC",L"",SS_ENDELLIPSIS|SS_NOPREFIX);
    g.search=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,SEARCH);
    button(L"清除搜索",CLEAR_FIELD_SEARCH);
    button(L"只看待保存",FILTER_CHANGED);button(L"只看收藏",FILTER_FAVORITES);button(L"导入方案",PATCH_IMPORT);
    g.grid=control(WC_LISTVIEWW,L"",WS_TABSTOP|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS,GRID);
    SetWindowSubclass(g.grid,fieldGridProc,1,0);
    configureSearchField(g.search,g.grid,L"搜索项目或存档字段");
    SetWindowTheme(g.grid,Theme::dark()?L"DarkMode_Explorer":L"Explorer",nullptr);
    ListView_SetExtendedListViewStyle(g.grid,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_LABELTIP);
    ListView_SetBkColor(g.grid,Theme::panel);ListView_SetTextBkColor(g.grid,Theme::panel);ListView_SetTextColor(g.grid,Theme::text);
    
    g.empty=control(L"STATIC",L"",SS_CENTER);
    g.name=control(L"STATIC",L"选择一个项目",SS_NOPREFIX);g.oldValue=control(L"STATIC",L"",SS_ENDELLIPSIS|SS_NOPREFIX);g.valueLabel=control(L"STATIC",L"新值",0);g.desc=control(L"EDIT",L"",WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL);
    button(L"收藏字段",FIELD_FAVORITE);
    SetWindowSubclass(g.desc,detailProc,1,0);
    g.edit=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,EDITVALUE);SendMessageW(g.edit,EM_SETLIMITTEXT,80,0);SetWindowSubclass(g.edit,editProc,1,0);
    button(L"false / 否",BOOL_FALSE);button(L"true / 是",BOOL_TRUE);button(L"应用修改",APPLY);button(L"还原该项",UNDOONE);button(L"复制字段名",COPY_KEY);button(L"Export CSV",EXPORT_CSV);
    button(L"资源设为 99",PRESET99);button(L"资源设为 999",PRESET999);button(L"自动同步资源上限",AUTOCAP);
    button(L"保存到原档",CONFIRM_SAVE);button(L"另存为…",SAVEAS);button(L"清空待保存项",UNDOALL);button(L"导出修改方案",PATCH_EXPORT);
    button(L"撤回此项",REVIEW_REVERT);
    button(L"刷新列表",BACKUP_REFRESH);button(L"打开备份目录",BACKUP_FOLDER);button(L"恢复这份备份",BACKUP_RESTORE);button(L"取消恢复",BACKUP_CANCEL);
    button(L"撤销",HISTORY_UNDO);button(L"重做",HISTORY_REDO);button(L"检查修改 →",SAVE);
    button(L"使用指南",HELP_USAGE);button(L"组件许可",HELP_LICENSES);
    g.helpText=control(L"EDIT",L"",WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,0);SendMessageW(g.helpText,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(px(6),px(6)));
    g.notice=control(L"STATIC",g.noticeText.c_str(),SS_ENDELLIPSIS|SS_NOPREFIX);
    if(Theme::dark())for(HWND h:{g.desc,g.helpText,languages})SetWindowTheme(h,L"DarkMode_Explorer",nullptr);
    enableHints();fonts();layout();refresh();
}
void command(int id,int code){
    if(id==SUPPORT_AUTHOR && code==BN_CLICKED){sponsor::show(g.win);return;}
    if(id==APPEARANCE&&code==BN_CLICKED){applyAppearanceCommand(pickAppearance(g.win,ctl(APPEARANCE)));return;}
    if(id==LANGUAGE&&code==CBN_SELCHANGE){changeLanguage();return;}
    if(id==EDITVALUE&&code==EN_CHANGE&&!g.populating){g.editingDirty=true;toast(L"未保存");updateStatus();return;}
    if(id==SEARCH&&code==EN_CHANGE&&!g.populating&&g.tab<5){
        if(applyInput(false)){
            g.pages[g.tab].query=textOf(g.search);
            refresh();
        }else{
            g.populating=true;rawText(g.search,g.pages[g.tab].query);g.populating=false;
            SetFocus(g.edit);SendMessageW(g.edit,EM_SETSEL,0,-1);
        }
        return;
    }
    if(id>=NAV_BASE&&id<NAV_BASE+8){navigate(id-NAV_BASE);return;}
    switch(id){
    case CLEAR_FIELD_SEARCH:clearSearchField(g.search);break;
    case OPEN:{auto p=pickFile();if(!p.empty())loadDocument(p);break;}case LATEST:chooseSave();break;
    case RELOAD:if(g.demoResource)loadDemo(g.demoResource);else if(g.doc){auto current=g.file;loadDocument(current);}break;
    case FOLDER:{auto p=!g.file.empty()?g.file.parent_path():saveFolder();if(fs::exists(p))ShellExecuteW(g.win,L"open",p.c_str(),nullptr,nullptr,SW_SHOWNORMAL);else message(L"默认目录不存在，请手动打开存档。\r\n"+p.wstring());break;}
    case APPLY:if(applyInput())toast(L"已应用，待保存");break;
    case UNDOONE:if(g.session&&!g.selected.empty()){g.session->revertField(g.selected);g.editingDirty=false;refresh();}break;
    case REVIEW_REVERT:{
        const auto key=reviewRevertKey();
        if(!key.empty()&&g.session->revertField(key)){refresh();toast(L"已撤回所选修改");}
        break;
    }
    case BOOL_FALSE:case BOOL_TRUE:g.boolValue=id==BOOL_TRUE;g.editingDirty=true;rawText(ctl(BOOL_FALSE),displayValue("false",true,i18n::language));rawText(ctl(BOOL_TRUE),displayValue("true",true,i18n::language));
    InvalidateRect(ctl(BOOL_FALSE),nullptr,FALSE);InvalidateRect(ctl(BOOL_TRUE),nullptr,FALSE);updateStatus();break;
    case EXPORT_CSV:exportCSV();break;case COPY_KEY:copyKey();break;case PRESET99:preset(99);break;case PRESET999:preset(999);break;
    case AUTOCAP:g.autoCap=!g.autoCap;updatePreviewCount();updateStatus();InvalidateRect(ctl(id),nullptr,FALSE);break;
    case FILTER_CHANGED:if(g.tab<5&&applyInput(false)){g.changedOnly=!g.changedOnly;g.pages[g.tab].changedOnly=g.changedOnly;refresh();}break;
    case FILTER_FAVORITES:
        if(g.tab==4&&applyInput(false)){g.favoritesOnly=!g.favoritesOnly;refresh();}
        break;
    case FIELD_FAVORITE:{
        if(g.tab>=5||!g.doc||!g.doc->find(g.selected))break;
        const bool remove=g.favorites.count(g.selected)!=0;
        const bool removingVisibleRow=remove&&g.tab==4&&g.favoritesOnly;
        if(removingVisibleRow&&!applyInput(false))break;
        if(!field_favorites::set(activeStory,g.selected,!remove,g.favorites)){
            message(L"无法保存字段收藏。",MB_OK|MB_ICONWARNING);
            break;
        }
        if(removingVisibleRow){refresh();break;}
        // Updating just the selected label leaves an uncommitted value in its
        // input control until the player actually applies or navigates away.
        int row=ListView_GetNextItem(g.grid,-1,LVNI_SELECTED);
        if(row>=0)rowText(row,0,(remove?L"":L"★  ")+label(g.selected));
        setText(ctl(FIELD_FAVORITE),remove?L"收藏字段":L"取消收藏");
        InvalidateRect(ctl(FIELD_FAVORITE),nullptr,FALSE);
        break;
    }
    case HISTORY_UNDO:if(g.session){if(g.editingDirty){g.editingDirty=false;showSelected();}else g.session->undo();refresh();toast(L"已撤销上一步修改");}break;
    case HISTORY_REDO:if(g.session&&!g.editingDirty){g.session->redo();refresh();toast(L"已重做上一步修改");}break;
    case UNDOALL:if(g.session){g.session->clearChanges();g.editingDirty=false;refresh();toast(L"待保存项已清空，点击“撤销”可恢复");}break;
    case SAVE:navigate(5);break;case CONFIRM_SAVE:saveDocument(false);break;case SAVEAS:saveDocument(true);break;
    case PATCH_IMPORT:patchFile(false);break;case PATCH_EXPORT:patchFile(true);break;
    case BACKUP_REFRESH:refreshBackups();layout();break;
    case BACKUP_FOLDER:if(!g.file.empty()){auto p=g.file.parent_path()/L".rizia-backups";if(fs::exists(p))ShellExecuteW(g.win,L"open",p.c_str(),nullptr,nullptr,SW_SHOWNORMAL);else toast(L"首次保存后将创建备份目录");}break;
    case BACKUP_RESTORE:restoreSelected();break;case BACKUP_CANCEL:g.restoreArmed=false;setText(ctl(BACKUP_RESTORE),L"恢复这份备份");visible(BACKUP_CANCEL,false);toast(L"已取消恢复");break;
    case HELP_LICENSES:g.licenseMode=true;setHelp();break;case HELP_USAGE:g.licenseMode=false;setHelp();break;
    }
}
LRESULT CALLBACK windowProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
    try{
    if(themeMenuMessage(m,wp,lp))return TRUE;
    switch(m){
    case WM_CREATE:g.win=w;g.dpi=GetDpiForWindow(w);if(!g.dpi)g.dpi=96;createControls();DragAcceptFiles(w,TRUE);return 0;
    case WM_SIZE:layout();return 0;
    case WM_ACTIVATE:if(LOWORD(wp)!=WA_INACTIVE&&g.grid)syncAppearance();break;
    case WM_SETTINGCHANGE:case WM_SYSCOLORCHANGE:if(g.grid)syncAppearance();break;
    case WM_GETMINMAXINFO:{auto*i=(MINMAXINFO*)lp;RECT area{};SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);i->ptMinTrackSize={std::min<LONG>(px(680),area.right-area.left),std::min<LONG>(px(485),area.bottom-area.top)};return 0;}
    case WM_DPICHANGED:g.dpi=HIWORD(wp);{auto*r=(RECT*)lp;SetWindowPos(w,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);}fonts();layout();return 0;
    case WM_PAINT:paint();return 0;case WM_ERASEBKGND:return 1;
    case WM_DRAWITEM:if(((DRAWITEMSTRUCT*)lp)->CtlType==ODT_BUTTON){
        const auto& item=*reinterpret_cast<DRAWITEMSTRUCT*>(lp);
        if(item.CtlID==CLEAR_FIELD_SEARCH)drawClearSearch(item,g.dpi);else drawButton(&item);
        return TRUE;}break;
    case WM_CTLCOLORLISTBOX:SetTextColor((HDC)wp,Theme::text);SetBkColor((HDC)wp,Theme::input);return (LRESULT)g.input;
    case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:{auto dc=(HDC)wp;auto h=(HWND)lp;SetTextColor(dc,h==g.path||h==g.meta||h==g.notice||h==g.desc||h==g.valueLabel?Theme::muted:Theme::text);
        if(h==g.path || h==g.meta || h==g.notice){
            paintWorkspaceChildBackdrop(dc,h); SetBkMode(dc,TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
        }
        bool edit=h==g.search||h==g.edit;bool panel=h==g.name||h==g.oldValue||h==g.desc||h==g.valueLabel||h==g.helpText||h==g.empty;SetBkColor(dc,edit?Theme::input:panel?Theme::panel:Theme::bg);return (LRESULT)(edit?g.input:panel?g.panel:g.bg);}
    case WM_COMMAND:
        if(((HWND)lp==g.edit || (HWND)lp==g.search) &&
           (HIWORD(wp)==EN_SETFOCUS || HIWORD(wp)==EN_KILLFOCUS))
            InvalidateRect(w,nullptr,FALSE);
        // Complete the native button's input callback before rebuilding child controls.
        if(HIWORD(wp)==BN_CLICKED){PostMessageW(w,WM_APP+1,wp,lp);return 0;}
        command(LOWORD(wp),HIWORD(wp));return 0;
    case WM_APP+1:command(LOWORD(wp),HIWORD(wp));return 0;
    case WM_APP+2:chooseSave();return 0;
    case WM_DROPFILES:{HDROP drop=(HDROP)wp;wchar_t path[32768]{};if(DragQueryFileW(drop,0,path,32768)){fs::path p(path);if(lower(p.extension().wstring())==L".json")loadDocument(p);else message(L"Drop a Suzerain JSON save here.");}DragFinish(drop);return 0;}
    case WM_NOTIFY:{auto*n=(NMHDR*)lp;
        if(n->hwndFrom==workspaceTips&&n->code==TTN_GETDISPINFOW){
            static std::wstring caption;caption=textOf(reinterpret_cast<HWND>(n->idFrom));
            reinterpret_cast<NMTTDISPINFOW*>(lp)->lpszText=caption.data();return 0;
        }
        if(n->idFrom==GRID){
        if(n->code==LVN_ITEMCHANGING&&!g.populating&&g.tab<5){
            auto*x=(NMLISTVIEW*)lp;
            if((x->uChanged&LVIF_STATE)&&((x->uOldState^x->uNewState)&LVIS_SELECTED)&&!applyInput(false))return TRUE;
        }
        if(n->code==LVN_ITEMCHANGED&&!g.populating){
            const auto*x=(NMLISTVIEW*)lp;
            if(!(x->uChanged&LVIF_STATE)||!((x->uOldState^x->uNewState)&LVIS_SELECTED))return 0;
            const int selected=ListView_GetNextItem(g.grid,-1,LVNI_SELECTED);
            if(g.tab==6){selectBackup(selected);return 0;}
            if(g.tab<=5){
                g.selected=selected>=0&&selected<(int)g.rows.size()?g.rows[selected]:"";
                if(!g.selected.empty())g.pages[g.tab].preferred=g.selected;
                if(g.tab<5)showSelected();else showReviewSelection();
            }
            return 0;
        }
        if((n->code==NM_RETURN || n->code==NM_DBLCLK) && g.tab<5){
            const int row=ListView_GetNextItem(g.grid,-1,LVNI_SELECTED);
            if(row<0 || (n->code==NM_DBLCLK && reinterpret_cast<NMITEMACTIVATE*>(lp)->iItem!=row))return 0;
            const auto* field=g.doc?g.doc->find(g.selected):nullptr;
            if(field){
                if(field->is_bool)SetFocus(ctl(g.boolValue?BOOL_TRUE:BOOL_FALSE));
                else{SetFocus(g.edit);SendMessageW(g.edit,EM_SETSEL,0,-1);}
            }
            return 0;
        }
        if(n->code==NM_CUSTOMDRAW){auto*c=(NMLVCUSTOMDRAW*)lp;if(c->nmcd.dwDrawStage==CDDS_PREPAINT)return CDRF_NOTIFYITEMDRAW|CDRF_NOTIFYPOSTPAINT;
            if(c->nmcd.dwDrawStage==CDDS_POSTPAINT){fillGridRemainder(c->nmcd.hdc);return CDRF_DODEFAULT;}
            if(c->nmcd.dwDrawStage==CDDS_ITEMPREPAINT){drawWorkspaceRow(*c);return CDRF_SKIPDEFAULT;}}
        }break;}
    case WM_CLOSE:if(discardIfNeeded())DestroyWindow(w);return 0;
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }}catch(const std::exception&e){message(L"操作没有完成。\r\n\r\n"+wide(e.what()),MB_OK|MB_ICONERROR);}catch(...){message(L"操作没有完成。请重新打开存档后重试。",MB_OK|MB_ICONERROR);}return DefWindowProcW(w,m,wp,lp);
}
bool handleEditorShortcut(WPARAM key, HWND focus){
    switch(key){
    case 'O': command(OPEN,0); return true;
    case 'S': navigate(5); return true;
    case 'L': command(LATEST,0); return true;
    case 'R': command(RELOAD,0); return true;
    case 'F':
        if(g.tab>=5)return false;
        SetFocus(g.search);
        SendMessageW(g.search,EM_SETSEL,0,-1);
        return true;
    case 'Z':
    case 'Y':
        if(focus==g.edit || focus==g.search)return false;
        command(key=='Z'?HISTORY_UNDO:HISTORY_REDO,0);
        return true;
    default: return false;
    }
}

int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,PWSTR,int show){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);i18n::loadPreferences();Theme::load();
    glass::refreshSettings();
    typography::selected=typography::stored();
    INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_WIN95_CLASSES};InitCommonControlsEx(&ic);
    g.bg=CreateSolidBrush(Theme::bg);g.panel=CreateSolidBrush(Theme::panel);g.input=CreateSolidBrush(Theme::input);fonts();
    WNDCLASSEXW c{};c.cbSize=sizeof(c);c.style=CS_HREDRAW|CS_VREDRAW;c.lpfnWndProc=windowProc;c.hInstance=inst;c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.lpszClassName=APP_CLASS;c.hIcon=LoadIconW(inst,MAKEINTRESOURCEW(101));c.hIconSm=c.hIcon;RegisterClassExW(&c);
    RECT area{};SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);UINT dpi=GetDpiForSystem();int width=std::min<int>(MulDiv(1280,dpi,96),area.right-area.left),height=std::min<int>(MulDiv(900,dpi,96),area.bottom-area.top);
    HWND w=CreateWindowExW(WS_EX_ACCEPTFILES,APP_CLASS,APP_TITLE,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,area.left+(area.right-area.left-width)/2,area.top+(area.bottom-area.top-height)/2,width,height,nullptr,nullptr,inst,nullptr);
    if(!w)return 1;
    Theme::caption(w);
    ShowWindow(w,show);UpdateWindow(w);
    int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(argv&&argc>1){if(std::wstring(argv[1])==L"--licenses"){g.licenseMode=true;navigate(7);}else loadDocument(fs::path(argv[1]),false);}else PostMessageW(w,WM_APP+2,0,0);if(argv)LocalFree(argv);
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){
        if(msg.message==WM_KEYDOWN && (GetKeyState(VK_CONTROL)&0x8000) &&
           handleEditorShortcut(msg.wParam,GetFocus()))continue;
        if(!IsDialogMessageW(w,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    if(g.rowHeight)ImageList_Destroy(g.rowHeight);for(auto f:{g.font,g.small,g.title,g.large,g.bold,g.mono,g.brand,g.emphasis,g.tableNumber,g.compactNumber})if(f)DeleteObject(f);DeleteObject(g.bg);DeleteObject(g.panel);DeleteObject(g.input);CoUninitialize();return (int)msg.wParam;
}
