#pragma once
#include <windows.h>
#include <array>
#include <initializer_list>
#include <string>

namespace typography {
struct Faces {
    const wchar_t* text=nullptr;
    const wchar_t* emphasis=nullptr;
    LONG emphasisWeight=FW_NORMAL;
};

enum ChineseFont { Automatic, System, Noto, PingFang, fontCount };
inline ChineseFont selected=Automatic;
inline bool isChinese(int language) { return language==0 || language==1; }
inline ChineseFont stored() {
    DWORD value=0,size=sizeof(value);
    if(RegGetValueW(HKEY_CURRENT_USER,Theme::settingsKey,L"ChineseUIFont",RRF_RT_REG_DWORD,
                    nullptr,&value,&size)!=ERROR_SUCCESS || value>=fontCount)return Automatic;
    return static_cast<ChineseFont>(value);
}
inline bool save(ChineseFont font) {
    if(font<Automatic || font>=fontCount)return false;
    DWORD value=font;
    return RegSetKeyValueW(HKEY_CURRENT_USER,Theme::settingsKey,L"ChineseUIFont",REG_DWORD,
                           &value,sizeof(value))==ERROR_SUCCESS;
}
struct FontStyles { bool installed=false,medium=false; };
inline int CALLBACK enumerateStyles(const LOGFONTW* face,const TEXTMETRICW*,DWORD,LPARAM data) {
    auto& styles=*reinterpret_cast<FontStyles*>(data);
    styles.installed=true;
    styles.medium=styles.medium || (!face->lfItalic && face->lfWeight==FW_MEDIUM);
    return !styles.medium;
}
inline FontStyles stylesOf(HDC dc,const wchar_t* face) {
    LOGFONTW filter{};
    filter.lfCharSet=DEFAULT_CHARSET;
    lstrcpynW(filter.lfFaceName,face,LF_FACESIZE);
    FontStyles styles;
    if(dc)EnumFontFamiliesExW(dc,&filter,enumerateStyles,reinterpret_cast<LPARAM>(&styles),0);
    return styles;
}
inline Faces available(HDC dc,std::initializer_list<const wchar_t*> families,
                       std::initializer_list<const wchar_t*> mediumFaces) {
    for(auto family:families) {
        const auto styles=stylesOf(dc,family);
        if(!styles.installed)continue;
        for(auto medium:mediumFaces)if(stylesOf(dc,medium).installed)return {family,medium};
        return {family,family,styles.medium?FW_MEDIUM:FW_NORMAL};
    }
    return {};
}
using ChineseFamilies=std::array<Faces,fontCount>;
inline ChineseFamilies chineseFamilies(int language) {
    const bool simplified=language==0;
    HDC dc=GetDC(nullptr);
    ChineseFamilies families{};
    families[PingFang]=simplified
        ?available(dc,{L"PingFang SC",L"PingFang SC Regular",L"苹方-简",L"蘋方-簡"},
                      {L"PingFang SC Medium",L"苹方-简 中黑体",L"蘋方-簡 中黑體"})
        :available(dc,{L"PingFang TC",L"PingFang TC Regular",L"蘋方-繁",L"苹方-繁"},
                      {L"PingFang TC Medium",L"蘋方-繁 中黑體",L"苹方-繁 中黑体"});
    families[System]=simplified?available(dc,{L"Microsoft YaHei UI",L"微软雅黑 UI"},{})
                               :available(dc,{L"Microsoft JhengHei UI",L"微軟正黑體 UI"},{});
    families[Noto]=simplified?available(dc,{L"Noto Sans SC"},{L"Noto Sans SC Medium"})
                             :available(dc,{L"Noto Sans TC"},{L"Noto Sans TC Medium"});
    if(dc)ReleaseDC(nullptr,dc);
    return families;
}
inline ChineseFont effectiveFont(const ChineseFamilies& families,ChineseFont preference) {
    if(preference>Automatic && preference<fontCount && families[preference].text)return preference;
    for(auto font:{PingFang,System,Noto})if(families[font].text)return font;
    return Automatic;
}

inline Faces chooseFaces(int language) {
    struct Candidates {const wchar_t* regular; const wchar_t* medium; const wchar_t* fallback;};
    static constexpr Candidates locales[]={
        {L"Segoe UI Variable Text",L"Segoe UI Semibold",L"Segoe UI"},
        {L"Noto Sans JP",L"Noto Sans JP Medium",L"Yu Gothic UI"},
        {L"Noto Sans KR",L"Noto Sans KR Medium",L"Malgun Gothic"}
    };
    if(isChinese(language)) {
        auto families=chineseFamilies(language);
        const auto effective=effectiveFont(families,selected);
        if(effective!=Automatic)return families[effective];
        return {L"Segoe UI",L"Segoe UI Semibold"};
    }
    const auto& names=locales[language==3?1:language==4?2:0];
    HDC dc=GetDC(nullptr);
    if(!dc)return {names.fallback,names.fallback};
    const wchar_t* regular=stylesOf(dc,names.regular).installed?names.regular:names.fallback;
    const wchar_t* medium=stylesOf(dc,names.medium).installed?names.medium:regular;
    ReleaseDC(nullptr,dc);
    return {regular,medium};
}

inline HFONT make(UINT dpi,int dip,const wchar_t* face,LONG weight=FW_NORMAL) {
    // Use a native named face or an enumerated weight. GDI can turn an
    // unsupported East Asian medium weight into synthetic bold.
    return CreateFontW(-MulDiv(dip,dpi,96),0,0,0,weight,FALSE,FALSE,FALSE,
                       DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY,DEFAULT_PITCH,face);
}

// Segoe UI's lining digits have equal advances, including on Windows 10.
inline constexpr wchar_t numbers[]=L"Segoe UI";

inline int width(HDC dc,HFONT font,const std::wstring& value) {
    auto old=SelectObject(dc,font);
    SIZE size{};
    GetTextExtentPoint32W(dc,value.data(),static_cast<int>(value.size()),&size);
    SelectObject(dc,old);
    return size.cx;
}
}
