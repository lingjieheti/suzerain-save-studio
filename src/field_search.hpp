#pragma once
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <cwctype>
#include <string>
#include <string_view>
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

struct PastedKey {
    std::string text;
    bool prefix = false;

    bool matches(const std::string& field) const {
        return prefix ? _strnicmp(field.c_str(), text.c_str(), text.size()) == 0
                      : _stricmp(field.c_str(), text.c_str()) == 0;
    }
};

// Guides use both complete assignments and open quoted prefixes for families of keys.
inline PastedKey pastedKey(std::wstring_view query) {
    auto trim = [](std::wstring_view value) {
        while (!value.empty() && space(value.front())) value.remove_prefix(1);
        while (!value.empty() && space(value.back())) value.remove_suffix(1);
        return value;
    };
    query = trim(query);
    const auto equals = query.find(L'=');
    if (equals != query.npos && query.find_first_of(L"=[]\r\n", equals + 1) != query.npos)
        return {};
    auto key = trim(query.substr(0, equals));
    bool prefix = false;
    if (!key.empty() && key.front() == L'[') {
        prefix = key.back() != L']';
        if (prefix && equals != query.npos) return {};
        key = trim(key.substr(1, key.size() - (prefix ? 1 : 2)));
        std::wstring_view quote;
        if (key.substr(0, 2) == L"\\\"") quote = key.substr(0, 2);
        else if (!key.empty() && (key.front() == L'"' || key.front() == L'\'')) quote = key.substr(0, 1);
        else return {};
        key.remove_prefix(quote.size());
        if (!prefix) {
            if (key.size() < quote.size() || key.substr(key.size() - quote.size()) != quote) return {};
            key.remove_suffix(quote.size());
        }
    } else if (equals == query.npos) return {};

    if (key.find(L'.') == key.npos || !std::all_of(key.begin(), key.end(), [](wchar_t c) {
        return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') ||
               (c >= L'0' && c <= L'9') || c == L'_' || c == L'.';
    })) return {};
    return {std::string(key.begin(), key.end()), prefix};
}

inline bool matches(const std::wstring& index, const std::vector<std::wstring>& terms) {
    return std::all_of(terms.begin(), terms.end(), [&](const std::wstring& term) {
        return index.find(term) != std::wstring::npos;
    });
}
} // namespace field_search
