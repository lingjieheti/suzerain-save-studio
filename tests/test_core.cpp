#include "../src/save_core.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>

namespace {
int checks = 0;
void check(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
void rejects(const std::function<void()>& operation, const char* message) {
    bool rejected = false;
    try { operation(); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, message);
}
std::string quote(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 32) {
                const char* h = "0123456789abcdef";
                out += "\\u00";
                out += h[c >> 4]; out += h[c & 15];
            } else out += static_cast<char>(c);
        }
    }
    return out + "\"";
}
std::string wrap(const std::string& lua, const std::string& extra = "") {
    return " \n{\"version\":\"3.1.0.1.175\", \"currentStoryPack\":\"StoryPack_Rizia\","
           "\"campaignName\":\"测试 😀\", \"turnNo\":1, \"variables\":" + quote(lua) + extra + "}\r\n";
}
void basics() {
    const auto text = wrap("Variable={ [\"RiziaDLC.Resources_Budget\"]=-2.5e+1, "
                           "[\"RiziaDLC.Flag\"]=false, [\"BaseGame.Budget\"]=7, "
                           "[\"RiziaDLCText.Test\"]=\"[\\\"RiziaDLC.Fake\\\"]=99\","
                           "[\"RiziaDLC.Name\"]=\"你好\",[\"RiziaDLC.Table\"]={ [\"RiziaDLC.Nested\"]=81 }}; ");
    auto d = rizia::Document::parse(text);
    auto copy = d;
    auto moved = std::move(copy);
    check(moved.find("RiziaDLC.Flag") == &moved.fields.back() &&
          moved.find("RiziaDLC.Flag") != d.find("RiziaDLC.Flag"),
          "Copied and moved documents resolve fields in their own storage");
    moved.validateChanges({{"RiziaDLC.Flag", "true"}});
    rejects([&] { moved.validateChanges({{"RiziaDLC.Flag", "10"}}); },
            "Staging rejects the same invalid boolean as serialization");
    check(d.fields.size() == 2, "Only top-level Rizia numbers and booleans are exposed");
    check(d.version == "3.1.0.1.175" && d.turn == "1", "Metadata parsed");
    check(d.campaignName == "测试 😀", "UTF8 metadata retained");
    check(d.find("RiziaDLC.Resources_Budget")->value == "-2.5e+1", "Numeric lexeme retained");
    check(d.find("RiziaDLC.Flag")->is_bool, "Boolean recognized");
    check(!d.find("BaseGame.Budget") && !d.find("RiziaDLC.Nested") && !d.find("RiziaDLC.Fake"),
          "Base game, nested fields and fake string keys ignored");
    check(d.render({}) == text, "No edits must be byte-identical");
    check(d.render({{"RiziaDLC.Resources_Budget", "-2.5e+1"}}) == text, "Same value must be byte-identical");
    auto budget = d.find("RiziaDLC.Resources_Budget");
    auto expected = text.substr(0, budget->start) + "50" + text.substr(budget->end);
    auto output = d.render({{"RiziaDLC.Resources_Budget", "50"}});
    check(output == expected, "Only selected byte span changes");
    auto roundTrip = rizia::Document::parse(output);
    check(roundTrip.find(budget->key)->value == "50", "Patched document parses again");
    auto both = rizia::Document::parse(d.render({{budget->key, "-1.25e2"}, {"RiziaDLC.Flag", "true"}}));
    check(both.find(budget->key)->value == "-1.25e2" && both.find("RiziaDLC.Flag")->value == "true",
          "Multiple patches preserve their correct spans");
    rejects([&] { d.render({{"BaseGame.Budget", "88"}}); }, "Unknown/basegame key rejected");
    rejects([&] { d.render({{"RiziaDLC.Flag", "1"}}); }, "Boolean numeric value rejected");
    rejects([&] { d.render({{budget->key, "false"}}); }, "Numeric boolean value rejected");
    for (const auto& value : {"nan", "NaN", "inf", "-inf", "1e9999", "2147483648",
                             "-2147483648", "2147483647.0000000000000000001",
                             "-2.14748364700000000000000000001e9",
                             "1+2", "2;alert()", "\"7\"", " 7", "7 ", "0x10", "", "1e", "--1"}) {
        rejects([&] { d.render({{budget->key, value}}); }, "Invalid/overflow numeric value rejected");
    }
    for (const auto& value : {"-2147483647", "2147483647", "0", "-0", ".5", "1.", "2.147483647e9", "1e-3",
                             "2147483647.0000000000000000000", "-0002147483647.00"}) {
        check(!d.render({{budget->key, value}}).empty(), "Valid finite decimal accepted");
    }
    std::string other = text;
    auto p = other.find("StoryPack_Rizia");
    other.replace(p, 15, "StoryPack_Sordland");
    auto foreign = rizia::Document::parse(other);
    rejects([&] { foreign.render({}); }, "Other story saving blocked even without changes");
}
void escapingAndComments() {
    const auto lua = std::string(
        "-- [\"RiziaDLC.Comment\"]=999\n"
        "Variable = { [\"RiziaDLC.Real\"] = 7, "
        "[\"Text\"]='fake [\"RiziaDLC.Fake\"]=99', "
        "[\"Long\"]=[==[ [\"RiziaDLC.LongFake\"]=88 ]==], "
        "--[=[ [\"RiziaDLC.BlockFake\"]=77 ]=]\n"
        "[\"RiziaDLC.Bool\"] = true, "
        "[\"RiziaDLC.\\x58\"] = -4.2E-3,"
        "[\"RiziaDLC.Table\"] = {x={y=\"fake [\\\"RiziaDLC.DeepFake\\\"]=8\"}},"
        "[\"RiziaDLC.Expr\"] = 1+2 }; -- done");
    auto d = rizia::Document::parse(wrap(lua));
    check(d.fields.size() == 3, "Comments, long strings and expressions never create fields");
    check(d.find("RiziaDLC.X") && d.find("RiziaDLC.X")->value == "-4.2E-3", "Lua key escape decoded");
    auto raw = wrap("Variable={[\"RiziaDLC.Unicode\"]=12};");
    const auto n = raw.find("=12");
    raw.replace(n + 1, 2, "\\u0031\\u0032");
    const auto key = raw.find("RiziaDLC.Unicode");
    raw.replace(key, 1, "\\u0052");
    raw.insert(0, "\xef\xbb\xbf");
    auto escaped = rizia::Document::parse(raw);
    check(escaped.render({}) == raw, "BOM and escaped JSON lexemes retained");
    const auto f = escaped.find("RiziaDLC.Unicode");
    check(f && f->value == "12" && raw.substr(f->start, f->end - f->start) == "\\u0031\\u0032",
          "Escaped value maps to complete original span");
    check(escaped.render({{f->key, "20"}}) == raw.substr(0, f->start) + "20" + raw.substr(f->end),
          "Escaped value replaced without changing key encoding or BOM");
    auto surrogate = wrap("Variable={[\"RiziaDLC.Emoji😀\"]=1};");
    auto emoji = surrogate.find("😀", surrogate.find("\"variables\""));
    surrogate.replace(emoji, 4, "\\ud83d\\ude00");
    auto sd = rizia::Document::parse(surrogate);
    check(sd.find("RiziaDLC.Emoji😀") != nullptr, "JSON surrogate pair in Lua key decoded");
    check(sd.render({{"RiziaDLC.Emoji😀", "2"}}).find("\\ud83d\\ude00") != std::string::npos,
          "Original surrogate encoding retained");
    auto boolRaw = wrap("Variable={[\"RiziaDLC.B\"]=true};");
    boolRaw.replace(boolRaw.find("=true") + 1, 4, "\\u0074r\\u0075e");
    auto boolDoc = rizia::Document::parse(boolRaw);
    const auto bf = boolDoc.find("RiziaDLC.B");
    check(bf && bf->is_bool && bf->value == "true", "Escaped JSON boolean recognized");
    check(boolDoc.render({{"RiziaDLC.B", "false"}}) ==
          boolRaw.substr(0, bf->start) + "false" + boolRaw.substr(bf->end), "Escaped boolean exact span patch");
}
void storyIsolation() {
    auto raw = wrap("Variable={ [\"RiziaDLC.Resources_Budget\"]=5,[\"BaseGame.GovernmentBudget\"]=7,[\"BaseGame.Flag\"]=false,[\"OtherMod.Number\"]=42,[\"BaseGame.Text\"]=\"keep\",[\"BaseGame.Table\"]={[\"BaseGame.Nested\"]=9}};");
    auto riziaDoc = rizia::Document::parse(raw);
    check(riziaDoc.fields.size() == 1 && !riziaDoc.find("BaseGame.GovernmentBudget"), "Rizia excludes every base-game field");
    raw.replace(raw.find("StoryPack_Rizia"), std::string("StoryPack_Rizia").size(), "StoryPack_Main");
    auto mainDoc = rizia::Document::parse(raw);
    check(mainDoc.fields.size() == 2 && !mainDoc.find("RiziaDLC.Resources_Budget"), "Main exposes only its own top-level scalar fields");
    check(mainDoc.render({}) == raw, "Main no-op preserves every byte");
    const auto* budget = mainDoc.find("BaseGame.GovernmentBudget");
    auto expected = raw; expected.replace(budget->start, budget->end - budget->start, "-10");
    check(mainDoc.render({{budget->key, "-10"}}) == expected, "Main editing changes exactly the selected byte span");
    rejects([&] { mainDoc.render({{"RiziaDLC.Resources_Budget", "99"}}); }, "Main cannot edit Rizia fields");
    rejects([&] { riziaDoc.render({{"BaseGame.GovernmentBudget", "99"}}); }, "Rizia cannot edit Main fields");
    rejects([&] { mainDoc.render({{"OtherMod.Number", "99"}}); }, "Main cannot edit unknown namespace");
    rejects([&] { mainDoc.render({{"BaseGame.Flag", "1"}}); }, "Main boolean validation enforced");
    check(rizia::Document::parse(mainDoc.render({{"BaseGame.Flag", "true"}})).find("BaseGame.Flag")->value == "true", "Main booleans roundtrip");
    auto forged = riziaDoc; forged.story = "StoryPack_Main";
    rejects([&] { forged.render({{"RiziaDLC.Resources_Budget", "99"}}); }, "Changed story metadata cannot unlock foreign fields");
    raw.replace(raw.find("StoryPack_Main"), std::string("StoryPack_Main").size(), "StoryPack_Unknown");
    auto unknown = rizia::Document::parse(raw);
    check(unknown.fields.empty(), "Unknown story exposes no editable fields");
    rejects([&] { unknown.render({}); }, "Unknown story cannot render even an empty edit");
    check(rizia::isSupportedStory("StoryPack_Main") && rizia::isSupportedStory("StoryPack_Rizia") && !rizia::isSupportedStory("StoryPack_Sordland"), "Only verified story identifiers supported");
    check(rizia::fieldPrefix("StoryPack_Main") == "BaseGame." && rizia::fieldPrefix("StoryPack_Rizia") == "RiziaDLC." && rizia::fieldPrefix("unknown").empty(), "Prefix mapping is exact");
}
void malformed() {
    for (const auto& raw : {"{}", "[]", "{\"variables\":0}", "{\"variables\":\"\"}",
                           "{\"x\":1,\"x\":2}", "{\"x\":1,}", "{\"x\":[1,]}",
                           "{\"x\":01}", "{\"x\":1.}", "{\"x\":1e}", "{\"x\":+1}",
                           "{\"x\":NaN}", "{\"x\":truefalse}", "{\"x\":\"\\q\"}",
                           "{\"x\":\"\\uD800\"}", "{\"x\":\"\\uDC00\"}", "{\"x\":\"\\uD800\\u0041\"}"}) {
        rejects([&] { rizia::Document::parse(raw); }, "Malformed/ambiguous JSON rejected");
    }
    const auto good = wrap("Variable={[\"RiziaDLC.A\"]=1};");
    for (std::size_t n = 0; n < good.find_last_of('}') + 1; ++n)
        rejects([&] { rizia::Document::parse(good.substr(0, n)); }, "Every truncated JSON prefix rejected");
    rejects([&] { rizia::Document::parse(good + "x"); }, "Trailing JSON junk rejected");
    rejects([&] { rizia::Document::parse(wrap("Variable={[\"RiziaDLC.A\"]=1};", ",\"variables\":\"Variable={};\"")); },
            "Multiple top-level variables strings rejected");
    rejects([&] { rizia::Document::parse(wrap("Variable={[\"RiziaDLC.A\"]=1};", ",\"\\u0076ariables\":\"Variable={};\"")); },
            "Unicode disguised duplicate JSON key rejected");
    for (const auto& lua : {
        "Variable={[\"RiziaDLC.A\"]=1,[\"RiziaDLC.A\"]=2};",
        "Variable={[\"RiziaDLC.A\"]=1,[\"RiziaDLC.\\065\"]=\"other\"};",
        "Variable={};Variable={[\"RiziaDLC.A\"]=2};",
        "Variable={[\"RiziaDLC.A\"]=};",
        "Variable={[\"RiziaDLC.A\"]=1",
        "Variable={[\"RiziaDLC.A\"]=1e};",
        "Variable={Text=[=[unfinished};",
        "Variable={Text='unfinished};",
        "Variable={Text={x=1};"
    }) rejects([&] { rizia::Document::parse(wrap(lua)); }, "Malformed/duplicate Lua assignment rejected");
    auto invalidUtf = good;
    invalidUtf.insert(invalidUtf.find("测试"), "\xc0\xaf");
    rejects([&] { rizia::Document::parse(invalidUtf); }, "Overlong raw UTF8 rejected");
    auto control = good;
    control.insert(control.find("测试"), 1, '\1');
    rejects([&] { rizia::Document::parse(control); }, "Raw control character rejected");
    auto nested = good;
    nested.insert(nested.find(", \"variables\""), ",\"deep\":" + std::string(128, '[') + "0" + std::string(128, ']'));
    rejects([&] { rizia::Document::parse(nested); }, "JSON depth limit enforced");
    auto luaDeep = wrap("Variable={Text=" + std::string(129, '{') + std::string(129, '}') + "};");
    rejects([&] { rizia::Document::parse(luaDeep); }, "Lua depth limit enforced");
    std::string oversized(50u * 1024u * 1024u + 1u, ' ');
    rejects([&] { rizia::Document::parse(oversized); }, "File size limit enforced");
    auto nestedVariables = rizia::Document::parse(wrap("Variable={[\"RiziaDLC.A\"]=1};",
        ",\"unrelated\":{\"variables\":\"Variable={[\\\"RiziaDLC.A\\\"]=999};\"}"));
    check(nestedVariables.find("RiziaDLC.A")->value == "1", "Only top-level variables container used");
}
void fixtureSave(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Could not open save fixture");
    const std::string raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto d = rizia::Document::parse(raw);
    check(!d.version.empty(), "Fixture version metadata exists");
    check(rizia::isSupportedStory(d.story) && !d.turn.empty(), "Fixture story and turn metadata");
    check(d.fields.size() >= 3, "Fixture exposes resource fields");
    check(d.render({}) == raw, "Fixture no-op is byte-identical");
    const bool main = d.story == "StoryPack_Main";
    auto b = d.find(main ? "BaseGame.GovernmentBudget" : "RiziaDLC.Resources_Budget");
    auto a = d.find(main ? "BaseGame.PersonalWealth" : "RiziaDLC.Resources_Authority");
    const rizia::Field* e = d.find("RiziaDLC.Resources_Energy");
    if (main) for (const auto& field : d.fields) if (!field.is_bool && &field != b && &field != a) { e = &field; break; }
    check(b && a && e, "Fixture resource fields found");
    check(!b->is_bool && !a->is_bool && !e->is_bool, "Fixture resource fields are numeric");
    const std::string budget = b->value == "999" ? "998" : "999";
    const std::string authority = a->value == "500" ? "499" : "500";
    const std::string energy = e->value == "200" ? "199" : "200";
    const auto out = d.render({{b->key, budget}, {a->key, authority}, {e->key, energy}});
    std::vector<const rizia::Field*> order{b, a, e};
    std::sort(order.begin(), order.end(), [](auto lhs, auto rhs) { return lhs->start > rhs->start; });
    auto expected = raw;
    for (const auto* f : order)
        expected.replace(f->start, f->end - f->start,
                         f == b ? budget : f == a ? authority : energy);
    check(out == expected, "Fixture changes exactly the three selected value spans");
    auto patched = rizia::Document::parse(out);
    check(patched.find(b->key)->value == budget && patched.find(a->key)->value == authority &&
          patched.find(e->key)->value == energy, "Fixture patch roundtrip");
    check(patched.fields.size() == d.fields.size(), "All fixture fields preserved");
    check(patched.version == d.version && patched.story == d.story && patched.turn == d.turn &&
          patched.campaignName == d.campaignName, "All fixture metadata preserved");
    std::cout << "Fixture: " << raw.size() << " bytes, version " << d.version
              << ", " << d.fields.size() << " editable fields\n";
}
} // namespace

int main(int argc, char** argv) {
    try {
        basics();
        storyIsolation();
        escapingAndComments();
        malformed();
        if (argc > 1) fixtureSave(argv[1]);
        std::cout << "PASS: " << checks << " core checks\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << " checks: " << e.what() << "\n";
        return 1;
    }
}
