#pragma once
#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

namespace save_backup {
struct Name {
    std::wstring source, timestamp;
    bool belongsTo(const std::wstring& filename) const {
        return !source.empty() && source.size() == filename.size() &&
               CompareStringOrdinal(source.c_str(), -1, filename.c_str(), -1, TRUE) == CSTR_EQUAL;
    }
};

inline bool timestamp(const std::wstring& text) {
    if (text.size() != 19) return false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i == 8 || i == 15) {
            if (text[i] != L'-') return false;
        } else if (text[i] < L'0' || text[i] > L'9') return false;
    }
    return true;
}

// Studio writes <entire filename>.<timestamp>.<GUID>.bak. Read from the
// right so dots, timestamps and similar names in the original remain intact.
inline Name identify(const std::filesystem::path& path) {
    const auto name = path.filename();
    if (_wcsicmp(name.extension().c_str(), L".bak") != 0) return {};
    const auto originalStem = name.stem();
    auto stem = originalStem;
    auto suffix = stem.extension().wstring();
    if (suffix.size() == 37 && suffix[9] == L'-' && suffix[14] == L'-' &&
        suffix[19] == L'-' && suffix[24] == L'-') {
        stem = stem.stem();
        suffix = stem.extension().wstring();
    }
    if (!suffix.empty() && timestamp(suffix.substr(1)))
        return {stem.stem().wstring(), suffix.substr(1)};

    // Also keep manually named <save>.bak or <save.json>.<label>.bak files.
    // The last full .json component identifies the source of a labeled copy.
    auto manual = originalStem;
    while (manual.has_extension()) {
        if (_wcsicmp(manual.extension().c_str(), L".json") == 0)
            return {manual.wstring(), {}};
        manual = manual.stem();
    }
    return {originalStem.wstring(), {}};
}

struct Item {
    std::filesystem::path path;
    std::wstring timestamp;
    std::uintmax_t size;
};

inline std::wstring lastWriteStamp(const std::filesystem::path& path) {
    WIN32_FILE_ATTRIBUTE_DATA attrs{};
    SYSTEMTIME utc{}, local{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attrs) ||
        !FileTimeToSystemTime(&attrs.ftLastWriteTime, &utc) ||
        !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) return {};
    wchar_t value[32];
    swprintf(value, 32, L"%04u%02u%02u-%02u%02u%02u-%03u", local.wYear, local.wMonth,
             local.wDay, local.wHour, local.wMinute, local.wSecond, local.wMilliseconds);
    return value;
}

inline std::vector<Item> list(const std::filesystem::path& save, std::uintmax_t sizeLimit) {
    namespace fs = std::filesystem;
    const auto filename = save.filename().wstring();
    const auto folder = save.parent_path() / L".rizia-backups";
    std::vector<Item> found;
    std::error_code ec;
    for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end;
         it != end; it.increment(ec)) {
        const auto& entry = *it;
        auto name = identify(entry.path());
        if (!name.belongsTo(filename) || !entry.is_regular_file(ec)) continue;
        const auto size = entry.file_size(ec);
        if (ec || !size || size > sizeLimit) continue;
        if (name.timestamp.empty()) name.timestamp = lastWriteStamp(entry.path());
        found.push_back({entry.path(), std::move(name.timestamp), size});
    }
    std::sort(found.begin(), found.end(), [](const Item& a, const Item& b) {
        return a.timestamp != b.timestamp ? a.timestamp > b.timestamp : a.path > b.path;
    });
    if (found.size() > 200) found.resize(200);
    return found;
}
} // namespace save_backup
