#include "../src/display_values.hpp"
#include "../src/edit_session.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

int main(int argc, char** argv) {
    std::size_t checks = 0;
    auto check = [&](bool ok, const char* why) { ++checks; if (!ok) throw std::runtime_error(why); };
    try {
        const wchar_t* expectedYes[] = {L"是",L"是",L"Yes",L"はい",L"예",L"Ja",L"Oui",L"Sí",L"Sim",L"Да",L"Evet"};
        const wchar_t* expectedNo[] = {L"否",L"否",L"No",L"いいえ",L"아니요",L"Nein",L"Non",L"No",L"Não",L"Нет",L"Hayır"};
        for (int language = 0; language < 11; ++language) {
            check(displayValue("true",true,language)==expectedYes[language], "Wrong true caption");
            check(displayValue("false",true,language)==expectedNo[language], "Wrong false caption");
            check(displayValue(L"true",true,language)==expectedYes[language], "Wide true caption differs");
            check(displayValue(L"false",true,language)==expectedNo[language], "Wide false caption differs");
            check(displayValue("true",true,language)!=displayValue("false",true,language), "Boolean captions collide");
            for (const std::string raw : {"", "0", "1", "-5", "-0.00", "1.2500", "1e+05", "-1.25E-3", "true", "false", "True", "FALSE", " true", "false ", "nil", "unknown"}) {
                const auto original = raw;
                const std::wstring wide(raw.begin(),raw.end());
                check(displayValue(raw,false,language)==wide, "Non-boolean display changed");
                check(raw==original, "Input string mutated");
                if (raw!="true" && raw!="false") check(displayValue(raw,true,language)==wide, "Unknown boolean spelling coerced");
            }
            check(displayValue(L"未识别的值",true,language)==L"未识别的值", "Wide unknown text changed");
            check(displayValue(L"—",false,language)==L"—", "Placeholder changed");
        }
        for (int language : {-100,-1,11,500}) {
            check(displayValue("true",true,language)==L"Yes", "Invalid language true fallback");
            check(displayValue("false",true,language)==L"No", "Invalid language false fallback");
        }
        // Displaying values must not change the editing/serialization contract.
        for (const std::string story : {"StoryPack_Main","StoryPack_Rizia"}) {
            const std::string prefix=rizia::fieldPrefix(story);
            const std::string json="{\"currentStoryPack\":\""+story+"\",\"variables\":\"Variable={ [\\\""+prefix+"Flag\\\"] = true, [\\\""+prefix+"Budget\\\"] = -5 }\"}";
            const auto doc=rizia::Document::parse(json);
            rizia::EditSession session(doc);
            session.applyBatch({{prefix+"Flag","false"}});
            const auto patch=rizia::serializePatch(session.pending(),story);
            const auto rendered=doc.render(session.pending());
            for (int language=0;language<11;++language) {
                for (const auto& field:doc.fields) {
                    const auto shown=displayValue(field.value,field.is_bool,language);
                    check(!shown.empty(), "Empty display value");
                }
                check(session.pending().at(prefix+"Flag")=="false", "Pending value translated");
                check(rizia::serializePatch(session.pending(),story)==patch, "Display changed patch bytes");
                check(doc.render(session.pending())==rendered, "Display changed save bytes");
                check(rizia::parsePatch(patch,story).at(prefix+"Flag")=="false", "Patch raw boolean roundtrip failed");
            }
            check(doc.original==json, "Display changed original bytes");
        }
        // Optional synthetic fixture arguments allow all observed field names/types
        // to be checked without copying or reading actual player values.
        std::size_t fields = 0, booleans = 0, numbers = 0;
        for (int arg = 1; arg < argc; ++arg) {
            std::ifstream input(argv[arg],std::ios::binary);
            check(static_cast<bool>(input), "Cannot open coverage fixture");
            const std::string bytes{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
            const auto doc=rizia::Document::parse(bytes);
            check(rizia::isSupportedStory(doc.story), "Unsupported fixture story");
            check(!doc.fields.empty(), "Empty fixture");
            for (const auto& field:doc.fields) {
                ++fields;
                if (field.is_bool) ++booleans; else ++numbers;
                check(field.key.rfind(rizia::fieldPrefix(doc.story),0)==0, "Foreign field exposed");
                for (int language=0;language<11;++language) {
                    const auto shown=displayValue(field.value,field.is_bool,language);
                    if (field.is_bool) {
                        check(shown==expectedYes[language] || shown==expectedNo[language], "Untranslated boolean field");
                        check(shown!=L"true" && shown!=L"false", "Raw boolean leaked into display");
                    } else check(shown==std::wstring(field.value.begin(),field.value.end()), "Numeric field formatting changed");
                }
            }
            check(doc.original==bytes && doc.render({})==bytes, "Coverage display changed original bytes");
        }
        std::cout << "PASS: " << checks << " display-value checks across 11 languages";
        if (argc>1) std::cout << "; evidence fields=" << fields << ", booleans=" << booleans << ", numbers=" << numbers;
        std::cout << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
