#pragma once
#include <windows.h>
#include <string>
#include <string_view>

inline std::wstring wide(const std::string& s) {
    if(s.empty()) return {};
    const int count=MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0);
    if(!count) return L"（无法显示的文本）";
    std::wstring out(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), out.data(), count);
    return out;
}

inline std::string utf8(std::wstring_view text) {
    if(text.empty())return {};
    const auto length=static_cast<int>(text.size());
    const int count=WideCharToMultiByte(CP_UTF8,0,text.data(),length,nullptr,0,nullptr,nullptr);
    if(!count)return {};
    std::string out(count,'\0');
    WideCharToMultiByte(CP_UTF8,0,text.data(),length,out.data(),count,nullptr,nullptr);
    return out;
}
