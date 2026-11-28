#pragma once

namespace live_setup {
constexpr wchar_t pluginName[] = L"SuzerainSaveStudio.Live.dll";
constexpr int chooseGameAction=701;

inline constexpr wchar_t panelShortcut[]=L"Ctrl + F8";
inline const wchar_t* inGameLabel(){
    static const wchar_t* const labels[]={
        L"游戏内",L"遊戲內",L"In game",L"ゲーム内",L"게임 내",L"Im Spiel",
        L"En jeu",L"En el juego",L"No jogo",L"В игре",L"Oyunda"
    };
    return labels[i18n::language];
}

inline std::wstring text(const wchar_t* chinese, const wchar_t* english) {
    if (i18n::language == 0) return chinese;
    if (i18n::language == 1) return i18n::traditional(chinese);
    return english;
}

inline fs::path bundledPlugin() {
    std::vector<wchar_t> path(32768);
    const DWORD count = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!count || count >= path.size()) throw WinFailure(winError());
    return fs::path(path.data()).parent_path() / pluginName;
}

// Publication uses a sidecar DLL so that building the save editor does not
// require a licensed game installation or ship any game libraries.
inline bool install(const fs::path& source, const fs::path& gameExe) {
    const auto root = gameExe.parent_path();
    const auto host = root / L"BepInEx";
    if (lower(gameExe.filename().wstring()) != L"suzerain.exe" || !fs::is_regular_file(gameExe) ||
        !fs::is_regular_file(root / L"GameAssembly.dll") ||
        !fs::is_regular_file(host / L"core" / L"BepInEx.Unity.IL2CPP.dll") ||
        !fs::is_regular_file(host / L"interop" / L"Assembly-CSharp.dll"))
        throw WinFailure(text(L"请选择已配置 BepInEx 6 IL2CPP、且已完成首次启动的 Suzerain 游戏。",
                              L"Choose a Suzerain installation that has already run with BepInEx 6 IL2CPP."));
    const auto payload = readFile(source);
    const auto folder = host / L"plugins" / L"SuzerainSaveStudio";
    const auto target = folder / pluginName;
    if (fs::exists(target)) {
        if (readFile(target) == payload) return false;
        throw WinFailure(text(L"此位置已有其他版本。退出游戏后，请先将原插件移出 plugins，再安装。",
                              L"Another version is already installed. Close the game and move the old plugin out of plugins before installing."));
    }
    fs::create_directories(folder);
    const auto temporary = folder / (writeId() + L".tmp");
    if (!CopyFileW(source.c_str(), temporary.c_str(), TRUE)) throw WinFailure(winError());
    try {
        if (readFile(temporary) != payload) throw WinFailure(text(L"插件复制校验失败。", L"Plugin copy verification failed."));
        if (!MoveFileExW(temporary.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH)) throw WinFailure(winError());
    } catch (...) {
        DeleteFileW(temporary.c_str());
        throw;
    }
    return true;
}

}
#include "live_intro.hpp"
namespace live_setup {
inline void show(HWND owner) {
    const auto title = tr(L"实时模式");
    try {
        const auto source = bundledPlugin();
        if(!openIntro(owner,fs::is_regular_file(source)))return;
        if (gameRunning()) throw WinFailure(text(L"请先退出 Suzerain，再安装插件。",L"Close Suzerain before installing the plugin."));
        std::vector<wchar_t> selected(32768);
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = owner;
        dialog.lpstrFile = selected.data(); dialog.nMaxFile = static_cast<DWORD>(selected.size());
        dialog.lpstrFilter = L"Suzerain.exe\0Suzerain.exe\0"; dialog.lpstrTitle = title.c_str();
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (!GetOpenFileNameW(&dialog)) return;
        const bool installed = install(source, fs::path(selected.data()));
        auto done = installed
            ? text(L"插件已安装。启动游戏，读取战役后按 Ctrl+F8。",L"Plugin installed. Start the game, load a campaign and press Ctrl+F8.")
            : text(L"此版本已安装。读取战役后按 Ctrl+F8。",L"This version is already installed. Load a campaign and press Ctrl+F8.");
        MessageBoxW(owner, done.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& error) {
        MessageBoxW(owner, wide(error.what()).c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
    }
}
}
