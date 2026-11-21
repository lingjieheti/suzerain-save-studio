#pragma once

namespace Theme {
constexpr int count = 4;
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
    {rgb(0xF9F7F1),rgb(0xF0ECE2),rgb(0xFDFBF6),rgb(0xFFFDF9),rgb(0xD9D3C7),rgb(0x302B26),rgb(0x61584E),rgb(0x70665B),
     rgb(0xF5E0D1),rgb(0xA54D30),rgb(0x8B3E26),rgb(0x843B26),rgb(0xFFFAF3),rgb(0xAB5839),
     rgb(0x373029),rgb(0x655A4D),rgb(0xD8D0C0),rgb(0xECE7DC),rgb(0xF9F6EF),rgb(0xF2EEE4),rgb(0xECE7DC),
     rgb(0x5B6F57),rgb(0x596875),rgb(0x963F31)},
    {rgb(0xEFF4EC),rgb(0xDCE7DB),rgb(0xF8FAF4),rgb(0xFCFDF8),rgb(0xB5C5B5),rgb(0x20372E),rgb(0x425D4E),rgb(0x516854),
     rgb(0xD3E5D4),rgb(0x346247),rgb(0x285138),rgb(0x234D35),rgb(0xF5FAF1),rgb(0x3B714E),
     rgb(0x243D2D),rgb(0x425F49),rgb(0xB3C5B5),rgb(0xCBDBCA),rgb(0xECF0E7),rgb(0xE1EBDB),rgb(0xD8E6D5),
     rgb(0x3C7058),rgb(0x415F65),rgb(0x924536)},
    {rgb(0xEBF1F6),rgb(0x253E52),rgb(0xF6F9FC),rgb(0xFCFDFF),rgb(0xB6C6D4),rgb(0x1E3346),rgb(0x465E72),rgb(0x526A7D),
     rgb(0xD2E5F1),rgb(0x326480),rgb(0x28546C),rgb(0x214D6A),rgb(0xF6FBFF),rgb(0x36769A),
     rgb(0xF2F7FA),rgb(0xC0D1DF),rgb(0x39586F),rgb(0x344F65),rgb(0xE8EFF5),rgb(0xDDE8F0),rgb(0xD8E7F0),
     rgb(0x3D705D),rgb(0x3D6482),rgb(0x984C45)},
    {rgb(0x191C25),rgb(0x14161E),rgb(0x222631),rgb(0x2B303D),rgb(0x454C63),rgb(0xE8E9F3),rgb(0xB1B7CA),rgb(0xA1A8BC),
     rgb(0x3B354E),rgb(0xBBA8EE),rgb(0xCCBDF3),rgb(0xD7C6FF),rgb(0x292235),rgb(0xC1ADF2),
     rgb(0xE7E7F0),rgb(0xAEB3C6),rgb(0x393D4D),rgb(0x2D2D3B),rgb(0x232733),rgb(0x2A2D3B),rgb(0x323544),
     rgb(0x98CDB4),rgb(0xA0C8EE),rgb(0xEFACAD)}
};

inline int selected = 0;
inline Palette colors = palettes[0];
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
    selected = valid(id) ? id : 0;
    colors = palettes[selected];
}
inline int stored() {
    DWORD value = 0, bytes = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, settingsKey, L"Appearance", RRF_RT_REG_DWORD,
                     nullptr, &value, &bytes) != ERROR_SUCCESS || value >= count)
        return 0;
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
inline const wchar_t* name(int id) { return translations[i18n::language].skins[valid(id) ? id : 0]; }
inline const wchar_t* bodyFace() {
    return i18n::language<=1?L"Microsoft YaHei UI":i18n::language==3?L"Yu Gothic UI":i18n::language==4?L"Malgun Gothic":L"Segoe UI";
}
inline const wchar_t* headingFace() {
    if(selected >= 2) return bodyFace();
    return i18n::language<=1?L"SimSun":i18n::language==3?L"Yu Mincho":i18n::language==4?L"Batang":L"Georgia";
}
inline int radius(int paperRadius) {
    if(paperRadius <= 7) return paperRadius;
    if(selected == 2) return 6;
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
