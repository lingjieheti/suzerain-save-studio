#pragma once
#include <string>

// Presentation only. Never feed these captions back into Document, EditSession,
// patches, CSV value columns, or numeric input. Values in those APIs stay raw.
// The boolean flag is required: a coincidental string must not become a caption.
inline std::wstring displayValue(const std::wstring& raw, bool isBoolean, int language) {
    if (!isBoolean || (raw != L"true" && raw != L"false")) return raw;
    // Same language order as i18n::names. General yes/no avoids implying that
    // every game flag is an option that the player can enable or disable.
    static constexpr const wchar_t* yes[] = {
        L"是", L"是", L"Yes", L"はい", L"예", L"Ja", L"Oui", L"Sí", L"Sim", L"Да", L"Evet"
    };
    static constexpr const wchar_t* no[] = {
        L"否", L"否", L"No", L"いいえ", L"아니요", L"Nein", L"Non", L"No", L"Não", L"Нет", L"Hayır"
    };
    const int index = language >= 0 && language < 11 ? language : 2;
    return raw == L"true" ? yes[index] : no[index];
}

// Save-core values are validated ASCII numbers or the ASCII boolean literals.
// The wide overload above is available for already-decoded display strings.
inline std::wstring displayValue(const std::string& raw, bool isBoolean, int language) {
    return displayValue(std::wstring(raw.begin(), raw.end()), isBoolean, language);
}
