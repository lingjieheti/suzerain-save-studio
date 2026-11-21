#include "../src/field_catalog.hpp"
#include "../src/field_labels_east.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

int main() {
    try {
        std::set<std::wstring> labels;
        for (const auto& field : baseDefs) labels.insert(field.label);
        for (const auto& field : riziaDefs) labels.insert(field.label);
        size_t covered = 0, checks = 0;
        for (const auto& source : labels) {
            for (int language : {3, 4, 9, 10}) {
                const auto translated = fieldEast(source, language); ++checks;
                if (translated.empty() || translated == source) {
                    std::wcerr << L"Missing field label: " << source << L"; language " << language << L'\n';
                    return 1;
                }
                if (translated.find(L'\n') != std::wstring::npos || translated.find(L'\r') != std::wstring::npos)
                    throw std::runtime_error("Unexpected multiline label");
                ++covered;
            }
            for (int language : {-1, 0, 1, 2, 5, 6, 7, 8, 11}) {
                ++checks;
                if (fieldEast(source, language) != source) throw std::runtime_error("Unsupported language should preserve source");
            }
        }
        for (int language : {3, 4, 9, 10}) {
            for (const auto* source : {L"", L"Unknown custom label", L"BaseGame.GovernmentBudget", L"RiziaDLC.Resources_Budget", L"C:\\Saves\\Budget.json"}) {
                ++checks;
                if (fieldEast(source, language) != source) throw std::runtime_error("Unknown source must remain unchanged");
            }
        }
        std::cout << "PASS: " << labels.size() << " unique labels x 4 languages = " << covered
                  << " translated labels; " << checks << " coverage/fallback checks\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
