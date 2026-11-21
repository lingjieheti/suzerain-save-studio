#pragma once

namespace demo {
constexpr int sordland = 202;
constexpr int rizia = 203;

inline std::string bytes(int id) {
    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(id), RT_RCDATA);
    HGLOBAL handle = resource ? LoadResource(module, resource) : nullptr;
    const auto* data = handle ? static_cast<const char*>(LockResource(handle)) : nullptr;
    DWORD size = resource ? SizeofResource(module, resource) : 0;
    if (!data || !size) throw WinFailure(tr(L"无法读取内置示例。"));
    return std::string(data, size);
}
}
