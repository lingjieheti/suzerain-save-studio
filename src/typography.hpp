#pragma once
#include <windows.h>
#include <string>

namespace typography {
struct Faces {
    const wchar_t* text;
    const wchar_t* emphasis;
};

inline int CALLBACK foundFace(const LOGFONTW*,const TEXTMETRICW*,DWORD,LPARAM result) {
    *reinterpret_cast<bool*>(result)=true;
    return 0;
}

inline bool installed(HDC dc,const wchar_t* face) {
    LOGFONTW filter{};
    filter.lfCharSet=DEFAULT_CHARSET;
    lstrcpynW(filter.lfFaceName,face,LF_FACESIZE);
    bool found=false;
    EnumFontFamiliesExW(dc,&filter,foundFace,reinterpret_cast<LPARAM>(&found),0);
    return found;
}

inline Faces chooseFaces(int language) {
    struct Candidates {const wchar_t* regular; const wchar_t* medium; const wchar_t* fallback;};
    static constexpr Candidates locales[]={
        {L"Noto Sans SC",L"Noto Sans SC Medium",L"Microsoft YaHei UI"},
        {L"Noto Sans TC",L"Noto Sans TC Medium",L"Microsoft JhengHei UI"},
        {L"Segoe UI Variable Text",L"Segoe UI Semibold",L"Segoe UI"},
        {L"Noto Sans JP",L"Noto Sans JP Medium",L"Yu Gothic UI"},
        {L"Noto Sans KR",L"Noto Sans KR Medium",L"Malgun Gothic"}
    };
    const auto& names=locales[language>=0 && language<=4?language:2];
    HDC dc=GetDC(nullptr);
    if(!dc)return {names.fallback,names.fallback};
    const wchar_t* regular=installed(dc,names.regular)?names.regular:names.fallback;
    const wchar_t* medium=installed(dc,names.medium)?names.medium:regular;
    ReleaseDC(nullptr,dc);
    return {regular,medium};
}

inline HFONT make(UINT dpi,int dip,const wchar_t* face) {
    // Named medium faces retain their native weight. GDI commonly turns
    // a request for weight 600 on an East Asian regular family into bold.
    return CreateFontW(-MulDiv(dip,dpi,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
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
