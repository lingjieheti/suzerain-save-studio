#include "../src/field_catalog.hpp"
#include "../src/field_labels_west.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

int main() {
    try {
        std::set<std::wstring> labels;
        for (const auto* rows : {&baseDefs, &riziaDefs})
            for (const auto& row : *rows) labels.insert(row.label);
        const auto& table = field_labels_west::catalog();
        if (table.size() != labels.size()) throw std::runtime_error("Stale or missing label entries");
        int translated = 0;
        for (const auto& label : labels) {
            auto entry = table.find(label);
            if (entry == table.end()) throw std::runtime_error("Catalog label missing");
            for (int language = 5; language <= 8; ++language) {
                if (!entry->second[language - 5] || !*entry->second[language - 5])
                    throw std::runtime_error("Empty translation");
                if (fieldWest(label, language) != entry->second[language - 5])
                    throw std::runtime_error("Incorrect language column");
                ++translated;
            }
        }
        if (fieldWest(L"Energy", 5) != L"Energie" ||
            fieldWest(L"Energy", 6) != L"\u00c9nergie" ||
            fieldWest(L"Energy", 7) != L"Energ\u00eda" ||
            fieldWest(L"Energy", 8) != L"Energia")
            throw std::runtime_error("Language mapping or Unicode regression");
        for (int language : {-1, 0, 1, 2, 3, 4, 9, 10, 11})
            if (fieldWest(L"Energy", language) != L"Energy")
                throw std::runtime_error("Other language must fall back");
        for (int language = 5; language <= 8; ++language) {
            if (fieldWest(L"Unknown label", language) != L"Unknown label" ||
                !fieldWest(L"", language).empty() ||
                fieldWest(L"Current resource stock; income is separate.", language) != L"Current resource stock; income is separate.")
                throw std::runtime_error("Unknown, empty or help fallback regression");
        }
        std::cout << labels.size() << " labels, " << translated
                  << " translations; language mapping, Unicode and fallback checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
