#pragma once
#include <windows.h>
#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

namespace field_search {
inline bool space(wchar_t c) {
    return std::iswspace(c) || c == L'\u00a0' || c == L'\u3000';
}

inline std::wstring fold(const std::wstring& text) {
    if (text.empty()) return {};
    const int length = static_cast<int>(text.size());
    const int size = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE,
                                   text.data(), length, nullptr, 0, nullptr, nullptr, 0);
    std::wstring result(size, L'\0');
    if (!size || !LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE,
                                text.data(), length, result.data(), size, nullptr, nullptr, 0))
        result = text;
    result.erase(std::remove_if(result.begin(), result.end(), [](wchar_t c) {
        return space(c) || c == L'_' || c == L'·' || c == L'-';
    }), result.end());
    return result;
}

inline std::vector<std::wstring> terms(const std::wstring& query) {
    std::vector<std::wstring> result;
    for (size_t at = 0; at < query.size();) {
        while (at < query.size() && space(query[at])) ++at;
        const size_t start = at;
        while (at < query.size() && !space(query[at])) ++at;
        if (start < at) {
            auto term = fold(query.substr(start, at - start));
            if (!term.empty()) result.push_back(std::move(term));
        }
    }
    return result;
}

inline bool matches(const std::wstring& index, const std::vector<std::wstring>& terms) {
    return std::all_of(terms.begin(), terms.end(), [&](const std::wstring& term) {
        return index.find(term) != std::wstring::npos;
    });
}
} // namespace field_search
