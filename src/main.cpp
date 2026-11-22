#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <fstream>
#include <set>
#include <iomanip>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <filesystem>
#include <algorithm>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cerrno>
#include <cstdlib>
#include <cwctype>
#include "save_core.hpp"
#include "save_discovery.hpp"
#include "edit_session.hpp"
#include "i18n.hpp"
#include "advanced_field_zh.hpp"
#include "display_values.hpp"

namespace fs = std::filesystem;
constexpr wchar_t APP_CLASS[] = L"SuzerainSaveStudio0110";
constexpr wchar_t APP_TITLE[] = L"Suzerain Save Studio";
constexpr size_t MAX_FILE = 50u * 1024u * 1024u;
enum { OPEN=101, LATEST, FOLDER, HELP, TABS, SEARCH, GRID, EDITVALUE, BOOLVALUE,
       APPLY, UNDOONE, PRESET99, PRESET999, AUTOCAP, UNDOALL, SAVEAS, SAVE,
       RESTORE, FILTER_CHANGED, PREVIEW_OK=301, PREVIEW_CANCEL };

std::wstring wide(const std::string& s) {
    if(s.empty()) return {};
    int n=MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0);
    if(!n) return L"（无法显示的文本）";
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n); return out;
}
std::string utf8(const std::wstring& s) {
    if(s.empty()) return {};
    int n=WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n, nullptr, nullptr); return out;
}
std::wstring textOf(HWND h) {
    int n=GetWindowTextLengthW(h); std::wstring s(n+1,L'\0');
    GetWindowTextW(h,s.data(),n+1); s.resize(n); return s;
}
std::wstring trim(std::wstring s) {
    auto a=s.find_first_not_of(L" \t\r\n"); if(a==std::wstring::npos)return {};
    return s.substr(a,s.find_last_not_of(L" \t\r\n")-a+1);
}
std::wstring lower(std::wstring s) {
    std::transform(s.begin(),s.end(),s.begin(),[](wchar_t c){return (wchar_t)towlower(c);});return s;
}
std::wstring winError(DWORD code=GetLastError()) {
    LPWSTR buf=nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr,code,0,(LPWSTR)&buf,0,nullptr);
    std::wstring result=buf?trim(buf):L"Windows 错误";if(buf)LocalFree(buf);
    return result+L"（"+std::to_wstring(code)+L"）";
}
struct WinFailure : std::runtime_error { explicit WinFailure(const std::wstring&s):std::runtime_error(utf8(s)){} };
struct Handle { HANDLE h=INVALID_HANDLE_VALUE; ~Handle(){if(h!=INVALID_HANDLE_VALUE&&h)CloseHandle(h);} };

std::string readFile(const fs::path& p) {
    Handle f{CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                         nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)};
    if(f.h==INVALID_HANDLE_VALUE)throw WinFailure(L"无法读取文件："+winError());
    LARGE_INTEGER size{}; if(!GetFileSizeEx(f.h,&size))throw WinFailure(winError());
    if(size.QuadPart<=0 || size.QuadPart>(LONGLONG)MAX_FILE)throw WinFailure(L"请选择非空且不超过 50 MB 的游戏存档。");
    std::string data((size_t)size.QuadPart,'\0'); size_t at=0;
    while(at<data.size()) { DWORD got=0;DWORD want=(DWORD)std::min<size_t>(data.size()-at,1024*1024);
        if(!ReadFile(f.h,data.data()+at,want,&got,nullptr)||!got)throw WinFailure(L"文件读取不完整，请重新打开。"); at+=got;
    } return data;
}
std::wstring stamp() {
    SYSTEMTIME t;GetLocalTime(&t); wchar_t b[80];
    swprintf(b,80,L"%04u%02u%02u-%02u%02u%02u-%03u",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);return b;
}
bool samePath(const fs::path&a,const fs::path&b){if(a.empty()||b.empty())return false;std::error_code ec;if(fs::equivalent(a,b,ec)&&!ec)return true;return lower(fs::absolute(a).lexically_normal().wstring())==lower(fs::absolute(b).lexically_normal().wstring());}
fs::path saveFolder() {
    PWSTR p=nullptr;fs::path base;
    if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow,0,nullptr,&p))){base=p;CoTaskMemFree(p);}
    else {wchar_t profile[32768];DWORD n=GetEnvironmentVariableW(L"USERPROFILE",profile,32768);if(n>0&&n<32768)base=fs::path(profile)/L"AppData"/L"LocalLow";}
    return base/L"Torpor Games"/L"Suzerain";
}
bool gameRunning() {
    Handle snap{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0)};if(snap.h==INVALID_HANDLE_VALUE)return false;
    PROCESSENTRY32W e{};e.dwSize=sizeof(e);
    if(Process32FirstW(snap.h,&e))do{if(lower(e.szExeFile)==L"suzerain.exe")return true;}while(Process32NextW(snap.h,&e));
    return false;
}

std::wstring writeId() {
    GUID guid{};
    wchar_t value[39]{};
    if (FAILED(CoCreateGuid(&guid)) || !StringFromGUID2(guid, value, 39))
        throw WinFailure(L"Cannot allocate a unique filename.");
    return std::wstring(value + 1, 36);
}

// Serialise saves from two instances before either checks the original bytes.
// A hash collision only makes unrelated saves wait for one another.
class SaveLock {
    Handle mutex_;
public:
    explicit SaveLock(const fs::path& target) {
        const auto path = lower(fs::weakly_canonical(fs::absolute(target)).wstring());
        std::uint64_t hash = 14695981039346656037ull;
        for (const wchar_t c : path) hash = (hash ^ c) * 1099511628211ull;
        const auto name = L"Local\\SuzerainSaveStudio-write-" + std::to_wstring(hash);
        mutex_.h = CreateMutexW(nullptr, FALSE, name.c_str());
        if (!mutex_.h) throw WinFailure(winError());
        const auto result = WaitForSingleObject(mutex_.h, 0);
        if (result == WAIT_TIMEOUT) throw WinFailure(tr(L"另一个窗口正在保存此存档，请稍后重试。"));
        if (result != WAIT_OBJECT_0 && result != WAIT_ABANDONED) throw WinFailure(winError());
    }
    ~SaveLock() { ReleaseMutex(mutex_.h); }
    SaveLock(const SaveLock&) = delete;
    SaveLock& operator=(const SaveLock&) = delete;
};

// Write in the same directory, flush, validate the complete bytes, then atomically replace.
// A null baseline means the destination must be new. The pre-write backup is
// retained even if Windows refuses the replacement.
fs::path atomicSave(const fs::path& target,const std::string& output,const std::string* expected) {
    if(gameRunning())throw WinFailure(L"请先退出 Suzerain，再保存或恢复存档。");
    const auto absoluteTarget=fs::absolute(target).lexically_normal();
    if(rizia::discovery::inBackupFolder(absoluteTarget)||rizia::discovery::inBackupFolder(fs::weakly_canonical(absoluteTarget)))
        throw WinFailure(tr(L"请在备份目录外另存一份存档。"));
    auto parsed=rizia::Document::parse(output);
    if(!rizia::isSupportedStory(parsed.story))throw WinFailure(L"Unsupported campaign.");
    SaveLock lock(target);
    const auto id = writeId();
    const bool exists=fs::exists(target);std::string old;
    if(exists && !expected)throw WinFailure(tr(L"目标文件已存在。请选择其他文件名。"));
    if(exists)old=readFile(target);
    if(expected && (!exists || old!=*expected))throw WinFailure(L"原存档在打开后发生了变化。为避免覆盖游戏的新进度，请重新打开该存档再修改。");
    if(exists && rizia::Document::parse(old).story!=parsed.story)throw WinFailure(L"Campaign mismatch.");
    fs::path backup;
    if(exists){
        fs::path folder=target.parent_path()/L".rizia-backups";fs::create_directories(folder);
        backup=folder/(target.filename().wstring()+L"."+stamp()+L"."+id+L".bak");
        if(!CopyFileW(target.c_str(),backup.c_str(),TRUE))throw WinFailure(L"无法创建原档备份，本次没有覆盖存档："+winError());
        if(readFile(backup)!=old)throw WinFailure(L"备份校验失败，本次没有覆盖存档。");
    }
    fs::path temp=target.parent_path()/(L".rizia-write-"+id+L".tmp");
    bool created = false;
    try {
        {Handle f{CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr)};
        if(f.h==INVALID_HANDLE_VALUE)throw WinFailure(L"无法创建临时文件："+winError());
        created = true;
        size_t at=0;while(at<output.size()){DWORD wrote=0,want=(DWORD)std::min<size_t>(output.size()-at,1024*1024);
            if(!WriteFile(f.h,output.data()+at,want,&wrote,nullptr)||!wrote)throw WinFailure(L"写入失败："+winError());at+=wrote;}
        if(!FlushFileBuffers(f.h))throw WinFailure(L"无法完成写入："+winError());}
        if(readFile(temp)!=output)throw WinFailure(L"写入校验失败，原档仍保留。");
        if(exists && readFile(target)!=old)throw WinFailure(L"保存过程中原存档又被其他程序修改，请退出游戏并重新打开。");
        BOOL ok=exists?ReplaceFileW(target.c_str(),temp.c_str(),nullptr,0,nullptr,nullptr):MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH);
        if(!ok)throw WinFailure(L"Windows 无法替换存档，原档和备份仍保留："+winError());
    } catch(...){if(created)DeleteFileW(temp.c_str());throw;}
    return backup;
}

#include "field_catalog.hpp"
std::string activeStory="StoryPack_Rizia";
bool isBase(){return activeStory=="StoryPack_Main";}
const std::vector<Def>& currentDefs(){return isBase()?baseDefs:riziaDefs;}
const Def* definition(const std::string&k){for(const auto&d:currentDefs())if(k==d.key)return &d;return nullptr;}
std::wstring label(const std::string&k){auto*d=definition(k);if(d)return tr(d->label);if(i18n::language<=1){auto caption=advancedZhLabel(k);return i18n::language==1?i18n::traditional(caption):caption;}return wide(k.substr(k.find('.')+1));}
std::wstring campaignLabel(const std::string&story){return tr(story=="StoryPack_Main"?L"Sordland":L"Rizia");}
int localizedMessageBox(HWND owner,const std::wstring& text,UINT flags){return MessageBoxW(owner,tr(text).c_str(),APP_TITLE,flags);}

#include "live_setup.hpp"
#include "ui_app.hpp"
