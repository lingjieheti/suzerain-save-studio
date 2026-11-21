#pragma once
#include <windows.h>
#include <charconv>
#include <set>
#include <string>
#include "save_core.hpp"

namespace field_favorites {
using Keys = std::set<std::string>;
constexpr wchar_t settings[] = L"Software\\SuzerainSaveStudio\\Favorites";

inline const wchar_t* valueName(const std::string& story) {
    if (story == "StoryPack_Main") return L"Sordland";
    if (story == "StoryPack_Rizia") return L"Rizia";
    return nullptr;
}

inline bool validKey(const std::string& story, const std::string& key) {
    const auto prefix = rizia::fieldPrefix(story);
    return !prefix.empty() && key.size() > prefix.size() && key.compare(0, prefix.size(), prefix) == 0;
}

// Store each raw, case-sensitive key as byte length + ':' + bytes. Game fields
// can contain punctuation and localized characters, so neither delimiters alone
// nor case-insensitive registry value names are suitable field identifiers.
inline bool read(const std::string& story, Keys& keys, HKEY root = HKEY_CURRENT_USER) {
    const auto name = valueName(story);
    if (!name) return false;
    DWORD bytes = 0;
    auto status = RegGetValueW(root, settings, name, RRF_RT_REG_BINARY, nullptr, nullptr, &bytes);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) { keys.clear(); return true; }
    if (status != ERROR_SUCCESS) return false;
    std::string data(bytes, '\0');
    if (bytes && RegGetValueW(root, settings, name, RRF_RT_REG_BINARY, nullptr, data.data(), &bytes) != ERROR_SUCCESS)
        return false;
    data.resize(bytes);
    Keys parsed;
    for (size_t offset = 0; offset < data.size();) {
        const auto start = data.data() + offset, end = data.data() + data.size();
        size_t length = 0;
        const auto size = std::from_chars(start, end, length);
        if (size.ec != std::errc() || size.ptr == end || *size.ptr != ':' ||
            length > static_cast<size_t>(end - size.ptr - 1)) return false;
        std::string key(size.ptr + 1, length);
        if (!validKey(story, key)) return false;
        parsed.insert(std::move(key));
        offset = static_cast<size_t>(size.ptr + 1 - data.data()) + length;
    }
    keys.swap(parsed);
    return true;
}

inline bool set(const std::string& story, const std::string& key, bool enabled, Keys& keys,
                HKEY root = HKEY_CURRENT_USER) {
    if (!validKey(story, key)) return false;
    // Read current preferences while holding the writer lock, so simultaneous
    // edits in separate windows preserve unrelated favorites.
    struct Guard {
        HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\SuzerainSaveStudio-FieldFavorites");
        bool owns = false;
        ~Guard() { if (owns) ReleaseMutex(mutex); if (mutex) CloseHandle(mutex); }
    } lock;
    if (!lock.mutex) return false;
    const auto wait = WaitForSingleObject(lock.mutex, 1500);
    lock.owns = wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED;
    Keys updated;
    if (!lock.owns || !read(story, updated, root)) return false;
    if (enabled) updated.insert(key); else updated.erase(key);
    std::string data;
    for (const auto& item : updated) data += std::to_string(item.size()) + ":" + item;
    if (data.size() > MAXDWORD ||
        RegSetKeyValueW(root, settings, valueName(story), REG_BINARY, data.data(),
                        static_cast<DWORD>(data.size())) != ERROR_SUCCESS) return false;
    keys.swap(updated);
    return true;
}
}
