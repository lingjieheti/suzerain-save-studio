// Optional read-only integration run: test_campaign_files <save-directory>
// No user save is included in the distribution; all render/edit results stay in memory.
#include "../src/save_discovery.hpp"
#include "../src/edit_session.hpp"
#include <iostream>
#include <iterator>

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: test_campaign_files <save-directory>\n"; return 2; }
    std::size_t checks = 0, mainCount = 0, riziaCount = 0;
    auto check = [&](bool yes, const char* message) { ++checks; if (!yes) throw std::runtime_error(message); };
    try {
        const auto found = rizia::discovery::scan({std::filesystem::u8path(argv[1])});
        check(!found.saves.empty(), "No saves found");
        check(!found.cancelled && !found.truncated, "Discovery incomplete");
        for (const auto& save : found.saves) {
            std::ifstream in(save.path, std::ios::binary);
            check(static_cast<bool>(in), "Cannot read save");
            const std::string raw{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
            auto doc = rizia::Document::parse(raw);
            check(doc.story == save.story, "Discovery story differs from parser");
            if (doc.story == "StoryPack_Main") ++mainCount; else ++riziaCount;
            const auto prefix = rizia::fieldPrefix(doc.story);
            check(!doc.fields.empty() && std::all_of(doc.fields.begin(), doc.fields.end(), [&](const auto& f) {
                return f.key.rfind(prefix, 0) == 0;
            }), "Foreign story field exposed");
            check(doc.render({}) == raw, "No-op changes original bytes");
            const auto& selected = doc.fields.front();
            const std::string value = selected.is_bool ? (selected.value == "true" ? "false" : "true") : (selected.value == "1" ? "2" : "1");
            rizia::EditSession session(doc), imported(doc);
            check(session.applyBatch({{selected.key, value}}), "Edit failed");
            auto expected = raw; expected.replace(selected.start, selected.end - selected.start, value);
            check(doc.render(session.pending()) == expected, "Edit changed bytes outside selected span");
            auto parsed = rizia::Document::parse(expected);
            check(parsed.story == doc.story && parsed.version == doc.version && parsed.turn == doc.turn && parsed.campaignName == doc.campaignName && parsed.fields.size() == doc.fields.size(), "Edit changed metadata or field inventory");
            const auto patch = rizia::serializePatch(session.pending(), doc.story);
            imported.applyBatch(rizia::parsePatch(patch, doc.story));
            check(imported.pending() == session.pending(), "Patch roundtrip failed");
            bool rejected = false;
            try { rizia::parsePatch(patch, doc.story == "StoryPack_Main" ? "StoryPack_Rizia" : "StoryPack_Main"); }
            catch (const std::runtime_error&) { rejected = true; }
            check(rejected, "Cross-campaign patch accepted");
            check(imported.undo() && doc.render(imported.pending()) == raw, "Undo failed to restore bytes");
            check(imported.redo() && doc.render(imported.pending()) == expected, "Redo failed");
        }
        std::cout << "PASS: " << checks << " read-only campaign checks, Main=" << mainCount << ", Rizia=" << riziaCount
                  << ", discovery candidates=" << found.candidates << ", errors=" << found.errors << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
