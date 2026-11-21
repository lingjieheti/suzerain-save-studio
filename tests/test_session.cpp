#include "../src/edit_session.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>

namespace {
int checks = 0;
constexpr const char* kBudget = "RiziaDLC.Resources_Budget";
constexpr const char* kAuthority = "RiziaDLC.Resources_Authority";
constexpr const char* kEnergy = "RiziaDLC.Resources_Energy";
constexpr const char* kFlag = "RiziaDLC.Flag";
constexpr const char* kDecimal = "RiziaDLC.Decimal";

void check(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
void rejects(const std::function<void()>& operation, const char* message) {
    bool rejected = false;
    try { operation(); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, message);
}
std::string quote(const std::string& text) {
    std::string result = "\"";
    for (char c : text) {
        if (c == '"' || c == '\\') { result += '\\'; result += c; }
        else if (c == '\n') result += "\\n";
        else result += c;
    }
    return result + '"';
}
rizia::Document document() {
    const std::string lua =
        "Variable={[\"RiziaDLC.Resources_Budget\"]=12,"
        "[\"RiziaDLC.Resources_Authority\"]=8,"
        "[\"RiziaDLC.Resources_Energy\"]=14,"
        "[\"RiziaDLC.Flag\"]=true,[\"RiziaDLC.Decimal\"]=1.25e2,"
        "[\"RiziaDLC.Extreme\"]=1e9999,[\"RiziaDLC.Text\"]=\"preserve\","
        "[\"BaseGame.Budget\"]=10};";
    return rizia::Document::parse(
        "{\"currentStoryPack\":\"StoryPack_Rizia\",\"version\":\"synthetic\","
        "\"campaignName\":\"SYNTHETIC_METADATA_NOT_FOR_EXPORT\",\"turnNo\":3,"
        "\"variables\":" + quote(lua) + "}");
}

void editing() {
    auto doc = document();
    rizia::EditSession s(doc);
    check(s.pending().empty() && s.changedCount() == 0, "Session starts empty");
    check(!s.canUndo() && !s.canRedo() && !s.undo() && !s.redo(), "Initial history is empty");
    check(!s.applyBatch({}) && !s.canUndo(), "Empty batch does not create history");
    check(!s.applyBatch({{kBudget, "12"}}), "Original lexical value is a no-op");
    check(s.applyBatch({{kBudget, "50"}, {kAuthority, "25"}, {kFlag, "false"}}), "Batch applied");
    check(s.changedCount() == 3 && s.canUndo() && !s.canRedo(), "Batch makes one undo step");
    auto first = s.pending();
    check(s.undo() && s.pending().empty() && !s.canUndo() && s.canRedo(), "Undo reverts entire batch");
    check(s.redo() && s.pending() == first && !s.canRedo(), "Redo restores entire batch");
    check(s.applyBatch({{kEnergy, "20"}}) && s.changedCount() == 4, "Later batch merges pending values");
    check(s.undo() && s.pending() == first && s.canRedo(), "Undo later batch preserves earlier values");
    check(!s.applyBatch({{kBudget, "50"}}) && s.canRedo(), "No-op preserves redo branch");
    check(!s.applyBatch({}) && s.canRedo(), "Empty batch preserves redo branch");
    check(s.applyBatch({{kBudget, "55"}}) && !s.canRedo(), "New edit discards redo branch");
    check(s.undo() && s.pending() == first, "Branch edit is undoable");
    check(s.redo() && s.pending().at(kBudget) == "55", "Branch edit is redoable");
    check(s.applyBatch({{kBudget, "12"}}) && !s.pending().count(kBudget), "Returning to original removes pending entry");
    check(s.changedCount() == 2, "Changed count tracks normalized map");
    check(s.undo() && s.pending().at(kBudget) == "55", "Normalization itself is undoable");
    auto beforeClear = s.pending();
    check(s.clearChanges() && s.pending().empty() && s.canUndo(), "Clear is undoable");
    check(s.undo() && s.pending() == beforeClear, "Undo clear restores all changes");
    check(s.redo() && s.pending().empty(), "Redo clear removes all changes");
    check(!s.clearChanges(), "Clear of empty state is a no-op");
    s.reset();
    check(s.pending().empty() && !s.canUndo() && !s.canRedo(), "Reset discards all history");
    check(s.applyBatch({{kBudget, "12.0"}}), "Numerically equal but lexically different is an edit");
    check(s.pending().at(kBudget) == "12.0", "Lexical value preserved");
    check(s.applyBatch({{kBudget, "12"}}) && s.pending().empty(), "Original lexeme normalizes");
    check(s.applyBatch({{kDecimal, "125"}}), "Scientific original can be edited");
    check(s.applyBatch({{kDecimal, "1.25e2"}}) && !s.pending().count(kDecimal), "Scientific original lexeme normalizes");
}

void atomicity() {
    auto doc = document();
    rizia::EditSession s(doc);
    s.applyBatch({{kBudget, "99"}});
    s.applyBatch({{kEnergy, "60"}});
    s.undo();
    const auto baseline = s.pending();
    check(s.canUndo() && s.canRedo(), "Rollback setup has both history directions");
    const std::vector<rizia::ChangeMap> invalid = {
        {{kAuthority, "100"}, {"RiziaDLC.DoesNotExist", "4"}},
        {{kAuthority, "100"}, {"BaseGame.Budget", "4"}},
        {{kAuthority, "100"}, {"RiziaDLC.Text", "4"}},
        {{kAuthority, "100"}, {kFlag, "1"}},
        {{kAuthority, "100"}, {kBudget, "false"}},
        {{kAuthority, "100"}, {kBudget, "2147483648"}},
        {{kAuthority, "100"}, {kBudget, "-2147483648"}},
        {{kAuthority, "100"}, {kBudget, "2147483647.0000000000000000001"}},
        {{kAuthority, "100"}, {kBudget, "NaN"}},
        {{kAuthority, "100"}, {kBudget, "inf"}},
        {{kAuthority, "100"}, {kBudget, "1e9999"}},
        {{kAuthority, "100"}, {kBudget, "1; evil()"}},
        {{kAuthority, "100"}, {kBudget, "1+2"}}
    };
    for (const auto& batch : invalid) {
        rejects([&] { s.applyBatch(batch); }, "Invalid batch rejected");
        check(s.pending() == baseline && s.canUndo() && s.canRedo(), "Failed batch preserves state and history");
    }
    check(s.redo() && s.pending().at(kEnergy) == "60", "Redo remains operational after failed batches");
    check(s.undo() && s.pending() == baseline, "Undo remains operational after failed batches");
    check(s.applyBatch({{"RiziaDLC.Extreme", "1"}}), "Valid replacement of extreme original accepted");
    auto current = s.pending();
    rejects([&] { s.applyBatch({{"RiziaDLC.Extreme", "1e9999"}}); }, "Invalid original lexeme still validated on explicit entry");
    check(s.pending() == current, "Invalid normalized entry did not mutate session");
    check(s.revertField("RiziaDLC.Extreme") && !s.pending().count("RiziaDLC.Extreme"),
          "Individual revert restores the out-of-range original without re-entering it");
    check(s.undo() && s.pending() == current, "Individual revert preserves other edits and can be undone");
    check(!s.revertField("RiziaDLC.Missing") && s.canRedo(), "Reverting an unmodified field preserves redo");
    check(s.clearChanges() && s.pending().empty(), "Clear restores baseline without re-entering extreme original");
    check(s.undo() && s.pending() == current, "Clear with extreme baseline is undoable");

    auto foreign = document();
    foreign.story = "StoryPack_Sordland";
    rizia::EditSession blocked(foreign);
    rejects([&] { blocked.applyBatch({{kBudget, "99"}}); }, "Other story blocked by render validation");
    rejects([&] { blocked.applyBatch({}); }, "Other story empty batch also blocked");
    check(blocked.pending().empty() && !blocked.canUndo(), "Blocked story remains untouched");
}

void patchBasics() {
    auto doc = document();
    rizia::EditSession source(doc), target(doc);
    source.applyBatch({{kBudget, "99"}, {kFlag, "false"}, {kDecimal, "-1.5e-2"}});
    const auto text = rizia::serializePatch(source.pending());
    check(text.rfind("# SuzerainPatch 1 StoryPack_Rizia\n", 0) == 0, "Patch has version header and story binding");
    check(text.find("campaignName") == std::string::npos &&
          text.find("SYNTHETIC_METADATA_NOT_FOR_EXPORT") == std::string::npos &&
          text.find("turnNo") == std::string::npos, "Patch contains no save metadata");
    check(rizia::parsePatch(text) == source.pending(), "Patch roundtrip preserves values");
    check(rizia::serializePatch(rizia::parsePatch(text)) == text, "Patch serialization is deterministic");
    check(target.applyBatch(rizia::parsePatch(text)) && target.pending() == source.pending(), "Patch import forms one batch");
    check(target.undo() && target.pending().empty() && !target.canUndo(), "Whole imported patch is one undo step");
    check(rizia::parsePatch("# RiziaPatch 1").empty(), "Header-only patch without newline accepted");
    check(rizia::parsePatch(rizia::serializePatch({})).empty(), "Empty patch roundtrip");
    const std::string comments =
        "\xef\xbb\xbf \r\n# Shared settings; no metadata\r\n  # RiziaPatch 1 \r\n"
        "# [\"RiziaDLC.Fake\"]=4\r\n"
        "  RiziaDLC.Resources_Budget \t= 50  # budget\r\n"
        "\tRiziaDLC.Flag=false#flag\r\n"
        "RiziaDLC.Decimal = 2.5E+2\r\n";
    auto parsed = rizia::parsePatch(comments);
    check(parsed.size() == 3 && parsed.at(kBudget) == "50" &&
          parsed.at(kFlag) == "false" && parsed.at(kDecimal) == "2.5E+2", "BOM, whitespace and comments accepted");
    check(rizia::parsePatch("# RiziaPatch 1\rRiziaDLC.Flag=true\r").at(kFlag) == "true", "CR-only line endings accepted");
    check(rizia::parsePatch("# RiziaPatch 1\nRiziaDLC.NewVersionField=9\n").size() == 1,
          "Parsing is independent of target document membership");
    rejects([&] { target.applyBatch(rizia::parsePatch("# RiziaPatch 1\nRiziaDLC.NewVersionField=9\n")); },
            "Unknown imported field rejected atomically");
    check(target.pending().empty() && target.canRedo(), "Failed import preserves redo");
    auto tooLarge = rizia::parsePatch("# RiziaPatch 1\nRiziaDLC.Resources_Budget=1e9999\n");
    rejects([&] { target.applyBatch(tooLarge); }, "Numeric range validated at application");
    auto wrongType = rizia::parsePatch("# RiziaPatch 1\nRiziaDLC.Flag=42\n");
    rejects([&] { target.applyBatch(wrongType); }, "Field type validated at application");
}

void maliciousPatches() {
    const std::vector<std::string> invalid = {
        "", " \n# comment only\n", "RiziaDLC.Flag=true\n",
        "# RiziaPatch 2\nRiziaDLC.Flag=true\n",
        "# RiziaPatch 1 extra\nRiziaDLC.Flag=true\n",
        "# RiziaPatch 1\n# RiziaPatch 1\n",
        "# RiziaPatch 1\nRiziaDLC.Flag=true\nRiziaDLC.Flag=false\n",
        "# RiziaPatch 1\n RiziaDLC.Flag = true\nRiziaDLC.Flag=true\n",
        "# RiziaPatch 1\nBaseGame.Budget=99\n",
        "# RiziaPatch 1\ncampaignName=test\n",
        "# RiziaPatch 1\nRiziaDLC.=1\n",
        "# RiziaPatch 1\nRiziaDLC.9invalid=1\n",
        "# RiziaPatch 1\nRiziaDLC.A B=1\n",
        "# RiziaPatch 1\nRiziaDLC.A[0]=1\n",
        "# RiziaPatch 1\nRiziaDLC.A\";evil()=1\n",
        "# RiziaPatch 1\nRiziaDLC.Flag=TRUE\n",
        "# RiziaPatch 1\nRiziaDLC.Flag=true false\n",
        "# RiziaPatch 1\nRiziaDLC.Flag=\"true\"\n",
        "# RiziaPatch 1\nRiziaDLC.A=1;evil()\n",
        "# RiziaPatch 1\nRiziaDLC.A=1+2\n",
        "# RiziaPatch 1\nRiziaDLC.A={x=1}\n",
        "# RiziaPatch 1\nRiziaDLC.A=0xFF\n",
        "# RiziaPatch 1\nRiziaDLC.A=NaN\n",
        "# RiziaPatch 1\nRiziaDLC.A=inf\n",
        "# RiziaPatch 1\nRiziaDLC.A=1e\n",
        "# RiziaPatch 1\nRiziaDLC.A==1\n",
        "# RiziaPatch 1\nRiziaDLC.A=\n",
        "# RiziaPatch 1\nRiziaDLC.A=1\nos.execute(1)\n",
        "# RiziaPatch 1\n[\"RiziaDLC.A\"]=1\n",
        "# RiziaPatch 1\nRiziaDLC.A=1\\nRiziaDLC.B=2\n",
        "# RiziaPatch 1\nRiziaDLC.A=1\t2\n",
        "# RiziaPatch 1\nRiziaDLC.A=\xef\xbb\xbf" "1\n"
    };
    for (const auto& text : invalid)
        rejects([&] { rizia::parsePatch(text); }, "Malformed/injected patch rejected");
    auto nul = std::string("# RiziaPatch 1\n# hidden");
    nul.push_back('\0');
    rejects([&] { rizia::parsePatch(nul); }, "NUL in comments is rejected");
    for (const auto& pair : std::vector<std::pair<std::string, std::string>>{
             {"BaseGame.A", "1"}, {"RiziaDLC.A\nRiziaDLC.B", "1"},
             {"RiziaDLC.A=1", "2"}, {"RiziaDLC.A#comment", "2"},
             {"RiziaDLC.A", "1\nRiziaDLC.B=2"}, {"RiziaDLC.A", "1;evil()"},
             {"RiziaDLC.A", "false#comment"}, {"RiziaDLC.A", "nan"}})
        rejects([&] { rizia::serializePatch({pair}); }, "Serializer rejects line/value injection");
}

void patchLimits() {
    rizia::ChangeMap many;
    for (std::size_t i = 0; i < rizia::kMaxPatchEntries; ++i)
        many["RiziaDLC.Field_" + std::to_string(i)] = "1";
    const auto valid = rizia::serializePatch(many);
    check(rizia::parsePatch(valid) == many, "Exactly 2000 entries accepted");
    auto excess = valid + "RiziaDLC.OneTooMany=1\n";
    rejects([&] { rizia::parsePatch(excess); }, "Parser rejects 2001 entries");
    many["RiziaDLC.OneTooMany"] = "1";
    rejects([&] { rizia::serializePatch(many); }, "Serializer rejects 2001 entries");
    std::string maxBytes = "# RiziaPatch 1\n#";
    maxBytes.append(rizia::kMaxPatchBytes - maxBytes.size(), 'a');
    check(rizia::parsePatch(maxBytes).empty(), "Exactly 256 KiB accepted");
    maxBytes += 'a';
    rejects([&] { rizia::parsePatch(maxBytes); }, "Parser rejects over 256 KiB");

    const std::size_t headerSize = rizia::serializePatch({}).size();
    const std::size_t keySize = rizia::kMaxPatchBytes - headerSize - 3;
    std::string hugeKey = "RiziaDLC.";
    hugeKey.append(keySize - hugeKey.size(), 'A');
    auto exact = rizia::serializePatch({{hugeKey, "0"}});
    check(exact.size() == rizia::kMaxPatchBytes && rizia::parsePatch(exact).size() == 1,
          "Serializer exact byte limit accepted");
    hugeKey += 'A';
    rejects([&] { rizia::serializePatch({{hugeKey, "0"}}); }, "Serializer rejects over byte limit");
}

void fixtureIntegration(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Could not open integration fixture");
    const std::string raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    auto doc = rizia::Document::parse(raw);
    rizia::EditSession source(doc), imported(doc);
    const std::string budgetKey = doc.story == "StoryPack_Main" ? "BaseGame.GovernmentBudget" : kBudget;
    const auto* budget = doc.find(budgetKey);
    check(budget && !budget->is_bool, "Integration fixture has budget");
    const auto replacement = budget->value == "999" ? "998" : "999";
    check(source.applyBatch({{budgetKey, replacement}}), "Integration change applied");
    imported.applyBatch(rizia::parsePatch(rizia::serializePatch(source.pending(), doc.story), doc.story));
    check(imported.pending() == source.pending(), "Fixture patch shares cleanly");
    check(doc.render(source.pending()) == doc.render(imported.pending()), "Imported patch renders identical result");
    check(imported.undo() && doc.render(imported.pending()) == raw, "Undo restores byte-identical original");
    check(imported.redo() && imported.pending() == source.pending(), "Fixture import redo works");
    std::cout << "Fixture: " << raw.size() << " bytes, " << doc.fields.size() << " editable fields\n";
}
void patchStories() {
    auto raw = document().original;
    raw.replace(raw.find("StoryPack_Rizia"), std::string("StoryPack_Rizia").size(), "StoryPack_Main");
    auto mainDoc = rizia::Document::parse(raw);
    rizia::EditSession mainSession(mainDoc);
    check(mainSession.applyBatch({{"BaseGame.Budget", "99"}}), "Main session supports editing");
    const auto mainPatch = rizia::serializePatch(mainSession.pending(), "StoryPack_Main");
    check(mainPatch.rfind("# SuzerainPatch 1 StoryPack_Main\n", 0) == 0, "Main export binds actual story");
    check(rizia::parsePatch(mainPatch, "StoryPack_Main") == mainSession.pending(), "Main patch roundtrip");
    check(mainSession.undo() && mainDoc.render(mainSession.pending()) == raw, "Main undo restores original bytes");
    check(mainSession.redo() && mainSession.changedCount() == 1, "Main redo restores pending edit");
    rejects([&] { rizia::parsePatch(mainPatch); }, "Main patch rejected by default Rizia target");
    rejects([&] { rizia::parsePatch(rizia::serializePatch({{kBudget, "5"}}), "StoryPack_Main"); }, "New Rizia patch rejected by Main target");
    rejects([&] { rizia::parsePatch("# RiziaPatch 1\nRiziaDLC.Resources_Budget=5\n", "StoryPack_Main"); }, "Legacy Rizia patch rejected by Main");
    check(rizia::parsePatch("# RiziaPatch 1\nRiziaDLC.Resources_Budget=5\n", "StoryPack_Rizia").size() == 1, "Legacy Rizia import retained");
    rejects([&] { rizia::serializePatch({{"BaseGame.Budget", "1"}}, "StoryPack_Rizia"); }, "Serializer rejects foreign Main field");
    rejects([&] { rizia::serializePatch({{kBudget, "1"}}, "StoryPack_Main"); }, "Serializer rejects foreign Rizia field");
    rejects([&] { rizia::serializePatch({}, "StoryPack_Unknown"); }, "Unknown patch story rejected");
    rejects([&] { rizia::parsePatch(mainPatch, "StoryPack_Unknown"); }, "Unknown target story rejected");
    for (const auto& text : {
        "# SuzerainPatch 1\nBaseGame.Budget=5\n",
        "# SuzerainPatch 1 StoryPack_Rizia\nBaseGame.Budget=5\n",
        "# SuzerainPatch 1 StoryPack_Main\nRiziaDLC.Resources_Budget=5\n",
        "# SuzerainPatch 1 StoryPack_Main\n# RiziaPatch 1\n",
        "# SuzerainPatch 1 StoryPack_Main\n# SuzerainPatch 1 StoryPack_Main\n",
        "# SuzerainPatch 1 StoryPack_Main extra\nBaseGame.Budget=5\n",
        "# SuzerainPatch 2 StoryPack_Main\nBaseGame.Budget=5\n"})
        rejects([&] { rizia::parsePatch(text, "StoryPack_Main"); }, "Unbound, mixed or malformed Main patch rejected");
    rejects([&] { mainSession.applyBatch({{kBudget, "5"}}); }, "Main session rejects foreign-story fields");
}
} // namespace

int main(int argc, char** argv) {
    try {
        editing();
        atomicity();
        patchBasics();
        maliciousPatches();
        patchLimits();
        patchStories();
        if (argc > 1) fixtureIntegration(argv[1]);
        std::cout << "PASS: " << checks << " session/patch checks\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << " checks: " << e.what() << "\n";
        return 1;
    }
}
