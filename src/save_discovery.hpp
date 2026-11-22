#pragma once
// Read-only, bounded discovery of existing Rizia JSON saves. C++17.
#include "save_core.hpp"
#include <atomic>
#include <cwctype>
#include <deque>
#include <filesystem>
#include <fstream>
#include <set>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace rizia { namespace discovery {
namespace fs = std::filesystem;
struct Save {
    fs::path path;
    std::string campaignName, turn, version, story;
    fs::file_time_type mtime{};
    std::uintmax_t size = 0;
    std::wstring modifiedText; // Local timestamp cached by the Windows picker.
};
struct Limits {
    unsigned maxDepth = 4; // Root directory is depth zero.
    std::size_t maxCandidates = 2000, maxEntries = 20000, maxResults = 1000;
    std::uintmax_t maxFileBytes = 50u * 1024u * 1024u;
    std::uintmax_t maxTotalBytes = 512u * 1024u * 1024u;
    const std::atomic_bool* cancel = nullptr;
};
struct Result {
    std::vector<Save> saves;
    std::size_t errors = 0, skipped = 0, candidates = 0, entriesVisited = 0;
    bool truncated = false, cancelled = false;
};
namespace detail {
inline std::wstring folded(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return value;
}
inline fs::path normalize(const fs::path& path, std::error_code& ec) {
    auto absolute = fs::absolute(path, ec);
    return ec ? fs::path{} : fs::weakly_canonical(absolute, ec);
}
inline std::wstring key(const fs::path& path) {
#ifdef _WIN32
    return folded(path.wstring());
#else
    return path.wstring();
#endif
}
inline bool linked(const fs::path& path, const fs::file_status& status) {
    if (fs::is_symlink(status)) return true;
#ifdef _WIN32
    const DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) return true;
#else
    (void)path;
#endif
    return false;
}
inline bool newer(const Save& a, const Save& b) {
    return a.mtime != b.mtime ? a.mtime > b.mtime : key(a.path) < key(b.path);
}
} // namespace detail

inline bool inBackupFolder(const fs::path& path) {
    for (const auto& part : path) if (detail::folded(part.wstring()) == L".rizia-backups") return true;
    return false;
}

// The returned metadata is a snapshot. Callers must parse the selected path again
// before editing, because the game may update it after discovery.
inline Result scan(const std::vector<fs::path>& roots, const Limits& limits = {}) {
    Result result;
    const auto cancelled = [&]() {
        if (limits.cancel && limits.cancel->load(std::memory_order_relaxed)) result.cancelled = true;
        return result.cancelled;
    };
    if (cancelled()) return result;
    struct Directory { fs::path path; unsigned depth; };
    std::deque<Directory> pending;
    std::set<std::wstring> directories, files;
    std::vector<Save> candidates;
    // Normalize only after checking the original path: resolving a symlink first
    // would hide that it points outside the requested directory.
    for (const auto& root : roots) {
        if (cancelled()) return result;
        if (root.empty() || inBackupFolder(root)) { ++result.skipped; continue; }
        std::error_code ec;
        auto status = fs::symlink_status(root, ec);
        if (ec || !fs::is_directory(status)) { ++result.errors; continue; }
        if (detail::linked(root, status)) { ++result.skipped; continue; }
        auto path = detail::normalize(root, ec);
        if (ec) { ++result.errors; continue; }
        if (inBackupFolder(path)) { ++result.skipped; continue; }
        if (directories.insert(detail::key(path)).second) pending.push_back({path, 0});
    }
    bool bounded = false;
    while (!pending.empty() && !bounded && !cancelled()) {
        auto dir = pending.front(); pending.pop_front();
        std::error_code ec;
        fs::directory_iterator it(dir.path, fs::directory_options::none, ec), end;
        if (ec) { ++result.errors; continue; }
        std::vector<fs::directory_entry> entries;
        while (it != end) {
            if (cancelled()) return result;
            if (result.entriesVisited >= limits.maxEntries) { result.truncated = bounded = true; break; }
            ++result.entriesVisited;
            entries.push_back(*it);
            it.increment(ec);
            if (ec) { ++result.errors; break; }
        }
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return detail::key(a.path()) < detail::key(b.path()); });
        for (const auto& entry : entries) {
            if (cancelled()) return result;
            auto status = entry.symlink_status(ec);
            if (ec) { ++result.errors; ec.clear(); continue; }
            if (detail::linked(entry.path(), status) || inBackupFolder(entry.path())) { ++result.skipped; continue; }
            if (fs::is_directory(status)) {
                // These are game logs/cache, not campaign folders. Restrict this
                // exclusion to Suzerain's own directory, never a generic root.
                const auto name = detail::folded(entry.path().filename().wstring());
                if (detail::folded(dir.path.filename().wstring()) == L"suzerain" && (name == L"unity" || name == L"logs")) {
                    ++result.skipped; continue;
                }
                if (dir.depth >= limits.maxDepth) { ++result.skipped; result.truncated = true; continue; }
                if (directories.insert(detail::key(entry.path())).second) pending.push_back({entry.path(), dir.depth + 1});
                continue;
            }
            if (!fs::is_regular_file(status) || detail::folded(entry.path().extension().wstring()) != L".json") continue;
            if (!files.insert(detail::key(entry.path())).second) continue;
            if (result.candidates >= limits.maxCandidates) { result.truncated = bounded = true; break; }
            ++result.candidates;
            Save candidate; candidate.path = entry.path();
            candidate.size = entry.file_size(ec);
            if (ec) { ++result.errors; ec.clear(); continue; }
            if (candidate.size == 0 || candidate.size > limits.maxFileBytes || candidate.size > rizia::detail::kMaxFileSize) { ++result.skipped; continue; }
            candidate.mtime = entry.last_write_time(ec);
            if (ec) { ++result.errors; ec.clear(); continue; }
            candidates.push_back(std::move(candidate));
        }
    }
    if (cancelled()) return result;
    // Parse newest files first so result limits retain the newest valid saves.
    std::sort(candidates.begin(), candidates.end(), detail::newer);
    auto remainingBytes = limits.maxTotalBytes;
    for (auto& candidate : candidates) {
        if (cancelled()) return result;
        if (result.saves.size() >= limits.maxResults || remainingBytes == 0) { result.truncated = true; break; }
        // An older, smaller save may still fit the remaining read budget.
        if (candidate.size > remainingBytes) { result.truncated = true; continue; }
        remainingBytes -= candidate.size;
        try {
            std::ifstream input(candidate.path, std::ios::binary);
            if (!input) { ++result.errors; continue; }
            std::string data(static_cast<std::size_t>(candidate.size), '\0');
            input.read(&data[0], static_cast<std::streamsize>(data.size()));
            // Do not parse a file that grew/shrank while it was being read.
            if (input.gcount() != static_cast<std::streamsize>(data.size()) || input.peek() != std::char_traits<char>::eof()) { ++result.errors; continue; }
            if (cancelled()) return result;
            auto doc = Document::parse(data);
            if (cancelled()) return result;
            if (!isSupportedStory(doc.story)) { ++result.skipped; continue; }
            candidate.story = doc.story;
            candidate.campaignName = std::move(doc.campaignName);
            candidate.turn = std::move(doc.turn);
            candidate.version = std::move(doc.version);
            result.saves.push_back(std::move(candidate));
        } catch (const std::exception&) { ++result.errors; }
    }
    return result;
}
}} // namespace rizia::discovery
