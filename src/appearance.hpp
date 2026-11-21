#pragma once

namespace Theme {
constexpr int count = 4;
constexpr int fjordIndex = 2;
constexpr int menuBase = 16320;
constexpr wchar_t settingsKey[] = L"Software\\SuzerainSaveStudio";

struct Palette {
    COLORREF bg, sidebar, panel, input, line, text, muted, dim;
    COLORREF select, accent, accentHover, accentInk, onAccent, focus;
    COLORREF sideText, sideMuted, sideLine, sideHover, stripe, head, hover;
    COLORREF status, blue, red;
};
constexpr COLORREF rgb(unsigned hex) { return RGB(hex >> 16, hex >> 8, hex); }

inline constexpr Palette palettes[count] = {
    {rgb(0xFAF9F6),rgb(0xF2EFE8),rgb(0xFFFEFC),rgb(0xFFFFFF),rgb(0xDEDAD2),rgb(0x2C2926),rgb(0x655E56),rgb(0x6F685F),
     rgb(0xF4E6DA),rgb(0xA44D32),rgb(0x89402A),rgb(0x814029),rgb(0xFFFFFF),rgb(0x98462F),
     rgb(0x38332D),rgb(0x625B51),rgb(0xE1DCD2),rgb(0xEAE4D9),rgb(0xFAF8F4),rgb(0xF3F0E9),rgb(0xF1EDE5),
     rgb(0x486957),rgb(0x496779),rgb(0x994B40)},
    {rgb(0xF7FAF8),rgb(0xEFF6F1),rgb(0xFFFFFF),rgb(0xFFFFFF),rgb(0xDDE7E0),rgb(0x24332C),rgb(0x52645A),rgb(0x617268),
     rgb(0xE3F1E8),rgb(0x327055),rgb(0x275B44),rgb(0x285D46),rgb(0xFFFFFF),rgb(0x347558),
     rgb(0x243C2E),rgb(0x52655A),rgb(0xDDE8DF),rgb(0xE3EEE6),rgb(0xF7FAF8),rgb(0xF0F6F2),rgb(0xEAF2EC),
     rgb(0x366A4B),rgb(0x466A76),rgb(0x944B40)},
    {rgb(0xF9FBFC),rgb(0xF3F6F7),rgb(0xFFFFFF),rgb(0xFFFFFF),rgb(0xDFE6EA),rgb(0x25323B),rgb(0x536570),rgb(0x606E79),
     rgb(0xE8F1F5),rgb(0x336C85),rgb(0x295A72),rgb(0x27576B),rgb(0xFFFFFF),rgb(0x336C85),
     rgb(0x30414C),rgb(0x546773),rgb(0xDFE6EA),rgb(0xEAF0F2),rgb(0xFAFBFC),rgb(0xF2F5F6),rgb(0xF0F4F6),
     rgb(0x3A6B58),rgb(0x395F80),rgb(0x974C48)},
    {rgb(0x1D2228),rgb(0x191E24),rgb(0x242A32),rgb(0x2B323B),rgb(0x404A55),rgb(0xEBEFF2),rgb(0xB2BBC5),rgb(0xA1ACB8),
     rgb(0x343E4A),rgb(0xB8C8E9),rgb(0xCDDAF0),rgb(0xD3E1F6),rgb(0x202832),rgb(0xB5CBE3),
     rgb(0xE8EDF1),rgb(0xB0BAC4),rgb(0x343D47),rgb(0x2A323B),rgb(0x252C34),rgb(0x29313A),rgb(0x303943),
     rgb(0x99CCAD),rgb(0xA8C9E7),rgb(0xECAEA7)}
};

inline int selected = fjordIndex;
inline Palette colors = palettes[fjordIndex];
inline const COLORREF &bg=colors.bg, &sidebar=colors.sidebar, &panel=colors.panel, &input=colors.input,
    &line=colors.line, &text=colors.text, &muted=colors.muted, &dim=colors.dim,
    &accent=colors.accent, &accentHover=colors.accentHover, &accentInk=colors.accentInk, &onAccent=colors.onAccent,
    &focus=colors.focus, &sideText=colors.sideText, &sideMuted=colors.sideMuted, &sideLine=colors.sideLine,
    &sideHover=colors.sideHover, &stripe=colors.stripe, &head=colors.head, &hover=colors.hover, &status=colors.status,
    &gold=colors.accent, &goldDark=colors.accentInk, &blue=colors.blue, &green=colors.status, &red=colors.red;
inline const COLORREF& select = colors.select;

inline bool valid(int id) { return id >= 0 && id < count; }
inline int index() { return selected; }
inline bool dark() { return selected == 3; }
inline void use(int id) {
    selected = valid(id) ? id : fjordIndex;
    colors = palettes[selected];
}
inline int stored() {
    DWORD value = 0, bytes = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, settingsKey, L"Appearance", RRF_RT_REG_DWORD,
                     nullptr, &value, &bytes) != ERROR_SUCCESS || value >= count)
        return fjordIndex;
    return static_cast<int>(value);
}
inline void load() { use(stored()); }
inline bool save(int id) {
    if (!valid(id)) return false;
    DWORD value = static_cast<DWORD>(id);
    return RegSetKeyValueW(HKEY_CURRENT_USER, settingsKey, L"Appearance", REG_DWORD,
                           &value, sizeof(value)) == ERROR_SUCCESS;
}

struct Names { const wchar_t* label; const wchar_t* skins[count]; };
inline constexpr Names translations[] = {
    {L"外观",{L"暖纸",L"林间",L"雾蓝",L"午夜"}},
    {L"外觀",{L"暖紙",L"林間",L"霧藍",L"午夜"}},
    {L"Appearance",{L"Paper",L"Forest",L"Fjord",L"Midnight"}},
    {L"外観",{L"ペーパー",L"森林",L"フィヨルド",L"真夜中"}},
    {L"테마",{L"종이",L"숲",L"피오르",L"한밤"}},
    {L"Design",{L"Papier",L"Wald",L"Fjord",L"Mitternacht"}},
    {L"Apparence",{L"Papier",L"Forêt",L"Fjord",L"Minuit"}},
    {L"Apariencia",{L"Papel",L"Bosque",L"Fiordo",L"Medianoche"}},
    {L"Aparência",{L"Papel",L"Floresta",L"Fiorde",L"Meia-noite"}},
    {L"Оформление",{L"Бумага",L"Лес",L"Фьорд",L"Полночь"}},
    {L"Görünüm",{L"Kâğıt",L"Orman",L"Fiyort",L"Gece yarısı"}}
};
inline const wchar_t* label() { return translations[i18n::language].label; }
inline const wchar_t* name(int id) { return translations[i18n::language].skins[valid(id) ? id : fjordIndex]; }
inline const wchar_t* bodyFace() {
    return i18n::language<=1?L"Microsoft YaHei UI":i18n::language==3?L"Yu Gothic UI":i18n::language==4?L"Malgun Gothic":L"Segoe UI";
}
inline const wchar_t* headingFace() {
    if(selected >= 2) return bodyFace();
    return i18n::language<=1?L"SimSun":i18n::language==3?L"Yu Mincho":i18n::language==4?L"Batang":L"Georgia";
}
inline int radius(int paperRadius) {
    if(paperRadius <= 7) return paperRadius;
    if(selected == fjordIndex) return std::clamp(paperRadius - 2, 7, 14);
    if(selected == 1) return paperRadius + 3;
    return paperRadius;
}
inline void caption(HWND w) {
    BOOL useDark = dark();
    DwmSetWindowAttribute(w,DWMWA_USE_IMMERSIVE_DARK_MODE,&useDark,sizeof(useDark));
    DwmSetWindowAttribute(w,DWMWA_CAPTION_COLOR,&bg,sizeof(bg));
    DwmSetWindowAttribute(w,DWMWA_TEXT_COLOR,&text,sizeof(text));
}
}
