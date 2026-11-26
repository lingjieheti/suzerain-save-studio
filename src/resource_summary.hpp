#pragma once

#include <string>
#include <string_view>
#include "field_search.hpp"
#include "i18n.hpp"

struct RiziaResource {
    const wchar_t* name;
    const wchar_t* balanceLabel;
    const char* stock;
    const char* turnTotal;
    const char* cap;
    const char* modifierPrefix;
    const wchar_t* turnUnit;
};

// Stock, calculated totals and HUD limits are separate numbers in the save.
inline constexpr RiziaResource riziaResources[] = {
    {L"预算", L"当前预算", "RiziaDLC.Resources_Budget", "RiziaDLC.Resources_BudgetModifiersTotal",
     "RiziaDLC.Rizia_HUDStat_Budget_Max", "RiziaDLC.Budget_Modifier_", L"BPT"},
    {L"权威", L"当前权威", "RiziaDLC.Resources_Authority", "RiziaDLC.Resources_AuthorityModifiersTotal",
     "RiziaDLC.Rizia_HUDStat_Authority_Max", "RiziaDLC.Authority_Modifier_", L"APT"},
    {L"能源", L"当前能源", "RiziaDLC.Resources_Energy", "RiziaDLC.Resources_EnergyModifiersTotal",
     "RiziaDLC.Rizia_HUDStat_Energy_Max", "RiziaDLC.Energy_Modifier_", L"EPT"}
};

// A guide's signed amount describes a contribution; only its unit identifies
// the saved per-turn total. Never interpret the amount as the field's value.
inline const char* riziaTurnSearchKey(std::wstring_view query) {
    if (!query.empty() && (query.front()==L'+' || query.front()==L'-' || query.front()==L'\u2212')) {
        query.remove_prefix(1);
        const auto digits=query.find_first_not_of(L"0123456789");
        if (digits==0 || digits==query.npos) return nullptr;
        query=field_search::trimmed(query.substr(digits));
    }
    if (query.size()!=3) return nullptr;
    for (const auto& resource:riziaResources)
        if (_wcsnicmp(query.data(),resource.turnUnit,3)==0) return resource.turnTotal;
    return nullptr;
}

inline constexpr char sordlandPublicOpinion[] = "BaseGame.Public_Opinion";
inline constexpr char riziaPublicOpinion[] = "RiziaDLC.Country_PublicOpinion";

inline std::wstring resourceSearchAliases(std::string_view key) {
    if (key == "BaseGame.GovernmentBudget") return L"GB";
    if (key == "BaseGame.PersonalWealth") return L"PW";
    if (key == sordlandPublicOpinion || key == riziaPublicOpinion)
        return L"民意|支持率|民调|民調|public approval|PO";
    if (key.compare(0, 9, "RiziaDLC.") != 0) return {};
    for (const auto& resource : riziaResources) {
        const std::wstring name = resource.name;
        if (key == resource.stock)
            return L"当前" + name + L"|现有" + name + L"|" + name + L"余额|" + name + L"餘額|" + name
                 + L"库存|balance|stock|on hand|" + tr(resource.balanceLabel);
        if (key == resource.cap)
            return name + L"容量|最大" + name + L"|" + name + L"最大值|cap|capacity|limit";
        if (key == resource.turnTotal)
            return L"每回合" + name + L"收入|" + name + L"每回合收入|" + name + L"产量|每回合"
                 + name + L"净变化|回合净变化|per turn|income|net change|" + tr(L"每回合");
        if (key.compare(0, std::char_traits<char>::length(resource.modifierPrefix), resource.modifierPrefix) == 0)
            return L"每回合" + name + L"来源|" + name + L"修正项|" + name + L"收支明细|modifier|breakdown";
    }
    return {};
}

inline std::wstring turnDelta(const std::wstring& raw) {
    if (raw.empty()) return L"—";
    if (raw.front() == L'-' || raw.front() == L'+') return raw;
    const auto nonzero = raw.find_first_of(L"123456789");
    return nonzero < raw.find_first_of(L"eE") ? L"+" + raw : raw;
}
