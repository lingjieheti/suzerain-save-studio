#include "../src/save_discovery.hpp"
#include <chrono>
#include <iostream>

namespace fs = std::filesystem;
int checks = 0;
void check(bool yes, const char* message) { ++checks; if (!yes) throw std::runtime_error(message); }
void write(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary); out << bytes;
    if (!out) throw std::runtime_error("fixture write failed");
}
std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
int main() {
    const auto root = fs::temp_directory_path() / ("rizia-discovery-test-" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    try {
        const std::string valid = "{\"version\":\"3.1.0.1.175\",\"currentStoryPack\":\"StoryPack_Rizia\",\"campaignName\":\"Test campaign\",\"turnNo\":3,\"variables\":\"Variable={ [\\\"RiziaDLC.Resources_Budget\\\"]=5 };\"}";
        auto other = valid; other.replace(other.find("StoryPack_Rizia"), 14, "StoryPack_Sordland");
        write(root / "a.json", valid);
        write(root / "nested" / "b.JSON", valid);
        write(root / "basegame.json", other);
        write(root / "bad.json", "{broken}");
        write(root / ".rizia-backups" / "backup.json", valid);
        write(root / "ignored.txt", valid);
        write(root / "empty.json", "");
        const auto now = fs::file_time_type::clock::now();
        fs::last_write_time(root / "a.json", now - std::chrono::hours(1));
        fs::last_write_time(root / "nested" / "b.JSON", now);
        auto result = rizia::discovery::scan({root, root / "nested", root});
        check(result.saves.size() == 2, "Only valid Rizia JSON saves, without duplicate roots");
        check(result.saves[0].path.filename() == "b.JSON", "Newest first, uppercase extension accepted");
        check(result.saves[1].path.filename() == "a.json", "Older save retained");
        check(result.saves[0].campaignName == "Test campaign" && result.saves[0].turn == "3", "Metadata preserved");
        check(result.saves[0].version == "3.1.0.1.175" && result.saves[0].size == valid.size(), "Version and size available");
        check(result.errors == 1 && result.skipped >= 3, "Malformed, unrelated, empty and backup files counted");
        check(!result.truncated && !result.cancelled, "Ordinary scan complete");
        check(read(root / "a.json") == valid && read(root / "nested" / "b.JSON") == valid, "Scan does not modify saves");
        auto limits = rizia::discovery::Limits{}; limits.maxResults = 1;
        auto limited = rizia::discovery::scan({root}, limits);
        check(limited.saves.size() == 1 && limited.saves[0].path.filename() == "b.JSON" && limited.truncated, "Result limit retains newest valid save");
        limits = {}; limits.maxDepth = 0;
        auto shallow = rizia::discovery::scan({root}, limits);
        check(shallow.saves.size() == 1 && shallow.saves[0].path.filename() == "a.json" && shallow.truncated, "Depth bound excludes nested saves");
        limits = {}; limits.maxCandidates = 1;
        auto bounded = rizia::discovery::scan({root}, limits);
        check(bounded.candidates == 1 && bounded.truncated, "Candidate count is bounded");
        limits = {}; limits.maxEntries = 1;
        check(rizia::discovery::scan({root}, limits).entriesVisited == 1, "Directory entry count is bounded");
        limits = {}; limits.maxTotalBytes = 1;
        check(rizia::discovery::scan({root}, limits).truncated, "Read budget is bounded");
        limits = {}; limits.maxFileBytes = 1;
        check(rizia::discovery::scan({root}, limits).saves.empty(), "Per-file size limit excludes oversized saves");
        fs::last_write_time(root / "a.json", now);
        auto tied = rizia::discovery::scan({root});
        check(tied.saves.size() == 2 && tied.saves[0].path.filename() == "a.json", "Equal timestamps use deterministic path order");
        auto normalized = rizia::discovery::scan({root / "nested" / "..", root / "nested"});
        check(normalized.saves.size() == 2, "Lexically different overlapping paths deduplicate");
        std::atomic_bool cancel{true}; limits = {}; limits.cancel = &cancel;
        auto cancelled = rizia::discovery::scan({root}, limits);
        check(cancelled.cancelled && cancelled.saves.empty() && cancelled.entriesVisited == 0, "Cancellation avoids scanning");
        auto missing = rizia::discovery::scan({root / "does-not-exist"});
        check(missing.errors == 1 && missing.saves.empty(), "Missing folder reports error");
        auto backup = rizia::discovery::scan({root / ".rizia-backups"});
        check(backup.saves.empty(), "Explicit backup root is excluded");
        write(root / "Suzerain" / "real.json", valid);
        write(root / "Suzerain" / "Unity" / "cache.json", "{not-a-save}");
        write(root / "Suzerain" / "Logs" / "log.json", valid);
        auto game = rizia::discovery::scan({root / "Suzerain"});
        check(game.saves.size() == 1 && game.errors == 0 && !game.truncated, "Known Suzerain caches and logs excluded");
        write(root / "Custom" / "Unity" / "save.json", valid);
        write(root / "Custom" / "Logs" / "save.json", valid);
        check(rizia::discovery::scan({root / "Custom"}).saves.size() == 2, "Generic roots retain folders named Unity and Logs");
        const auto gameDir = root / "Suzerain";
        const auto modDir = gameDir / "ModdingKit";
        const std::string sidecar = R"({"Variables":{"ZilleWar.Branch":true}})";
        write(modDir / "real.json", sidecar);
        write(modDir / "active.json", sidecar);
        fs::last_write_time(gameDir / "real.json", now - std::chrono::hours(1));
        limits = {}; limits.maxCandidates = 1;
        const auto modded = rizia::discovery::scan({gameDir}, limits);
        check(modded.candidates == 1 && modded.errors == 0 && !modded.truncated && modded.saves.size() == 1 &&
              fs::equivalent(modded.saves.front().path, gameDir / "real.json"),
              "Newer Modding Kit sidecars do not displace their main save or trigger an unreadable-file warning");
        write(gameDir / "broken.json", "{broken}");
        check(rizia::discovery::scan({gameDir}).errors == 1, "Malformed main saves are still reported alongside mod data");
        write(modDir / "manual-copy.json", valid);
        const auto explicitModRoot = rizia::discovery::scan({gameDir, modDir});
        check(explicitModRoot.saves.size() == 2 && explicitModRoot.errors == 3,
              "An explicitly selected mod folder is scanned even when its parent is another root");
        write(root / "Custom" / "moddingkit" / "save.json", valid);
        check(rizia::discovery::scan({root / "Custom"}).saves.size() == 3,
              "A custom library can store ordinary saves in a folder named moddingkit");
        auto mainSave = valid;
        mainSave.replace(mainSave.find("StoryPack_Rizia"), std::string("StoryPack_Rizia").size(), "StoryPack_Main");
        mainSave.replace(mainSave.find("RiziaDLC.Resources_Budget"), std::string("RiziaDLC.Resources_Budget").size(), "BaseGame.GovernmentBudget");
        write(root / "mixed" / "rizia.json", valid);
        write(root / "mixed" / "main.json", mainSave);
        write(root / "mixed" / "unknown.json", other);
        const auto mixed = rizia::discovery::scan({root / "mixed"});
        check(mixed.saves.size() == 2 && mixed.skipped == 1, "Discovery accepts both supported campaigns and excludes unknown stories");
        bool sawMain = false, sawRizia = false;
        for (const auto& save : mixed.saves) { sawMain |= save.story == "StoryPack_Main"; sawRizia |= save.story == "StoryPack_Rizia"; }
        check(sawMain && sawRizia, "Discovery results include exact story identifier");
        const auto library = root / "library";
        write(library / "latest.json", valid);
        write(library / "larger.json", valid + " ");
        write(library / "older.json", valid);
        fs::last_write_time(library / "latest.json", now);
        fs::last_write_time(library / "larger.json", now - std::chrono::hours(1));
        fs::last_write_time(library / "older.json", now - std::chrono::hours(2));
        limits = {}; limits.maxTotalBytes = 2 * valid.size();
        const auto exactFit = rizia::discovery::scan({library}, limits);
        check(exactFit.truncated && exactFit.saves.size() == 2 &&
              exactFit.saves[0].path.filename() == "latest.json" && exactFit.saves[1].path.filename() == "older.json",
              "Read budget skips a larger middle save and retains a smaller exact fit in newest-first order");
        --limits.maxTotalBytes;
        const auto oneByteShort = rizia::discovery::scan({library}, limits);
        check(oneByteShort.truncated && oneByteShort.saves.size() == 1 && oneByteShort.saves[0].path.filename() == "latest.json",
              "An older save one byte over the remaining budget is excluded");
        const auto firstRoot = root / "archive";
        const auto secondRoot = root / "current";
        const auto old = firstRoot / "old.json";
        const auto recent = firstRoot / "recent.json";
        const auto newest = secondRoot / "latest.json";
        write(old, valid); write(recent, valid); write(newest, valid);
        fs::last_write_time(old, now - std::chrono::hours(2));
        fs::last_write_time(recent, now - std::chrono::hours(1));
        fs::last_write_time(newest, now);
        limits = {}; limits.maxCandidates = 2;
        for (const auto& roots : {std::vector<fs::path>{firstRoot, secondRoot}, std::vector<fs::path>{secondRoot, firstRoot}}) {
            const auto capped = rizia::discovery::scan(roots, limits);
            check(capped.saves.size() == 2 && fs::equivalent(capped.saves[0].path, newest) && fs::equivalent(capped.saves[1].path, recent),
                  "Candidate limit retains newest eligible files across roots in either order");
            check(capped.candidates == 2 && capped.entriesVisited == 3 && capped.truncated,
                  "Candidate limit caps the files selected for reading while scanning all available entries");
        }
        limits.maxEntries = 2;
        const auto entryCap = rizia::discovery::scan({firstRoot, secondRoot}, limits);
        check(entryCap.entriesVisited == 2 && entryCap.candidates == 2 && entryCap.truncated &&
              entryCap.saves.size() == 2 && fs::equivalent(entryCap.saves[0].path, recent) && fs::equivalent(entryCap.saves[1].path, old),
              "Entry limit constrains discovery to the roots actually visited");
        limits.maxEntries = 100;
        fs::last_write_time(newest, now - std::chrono::hours(1));
        limits.maxCandidates = 1;
        const auto tiedCap = rizia::discovery::scan({secondRoot, firstRoot}, limits);
        check(tiedCap.truncated && tiedCap.candidates == 1 && tiedCap.saves.size() == 1 && fs::equivalent(tiedCap.saves[0].path, recent),
              "Equal timestamps at the candidate cutoff retain the lexically earlier path even when encountered last");
        const auto ineligible = root / "ineligible";
        write(ineligible / "empty.json", "");
        write(ineligible / "oversized.json", valid + " ");
        limits.maxFileBytes = valid.size();
        const auto eligible = rizia::discovery::scan({ineligible, secondRoot}, limits);
        check(eligible.candidates == 1 && eligible.skipped == 2 && !eligible.truncated &&
              eligible.saves.size() == 1 && fs::equivalent(eligible.saves[0].path, newest),
              "Empty and oversized JSON files do not consume a candidate slot or truncate an exact fit");
        limits.maxCandidates = 0;
        const auto noCandidates = rizia::discovery::scan({secondRoot}, limits);
        check(noCandidates.truncated && noCandidates.candidates == 0 && noCandidates.saves.empty(),
              "Zero candidate budget never reads a save");
        std::error_code ec;
        fs::create_directory_symlink(root / "nested", root / "link", ec);
        if (!ec) check(rizia::discovery::scan({root / "link"}).saves.empty(), "Symlink roots excluded");
        else std::cout << "SKIP: symlink creation unavailable in this environment\n";
        // Delete only the uniquely created test directory.
        fs::remove_all(root);
        std::cout << checks << " discovery checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\nFixtures retained at " << root << '\n';
        return 1;
    }
}
