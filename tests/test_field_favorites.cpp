// Exercises on-disk preferences under a private registry key without a desktop or game.
#include "../src/field_favorites.hpp"
#include <iostream>

namespace {
int checks = 0;
void check(bool passes, const char* message) {
    ++checks;
    if (!passes) throw std::runtime_error(message);
}
struct TemporaryRoot {
    std::wstring path = L"Software\\SuzerainSaveStudio-Favorites-Test-" +
                        std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    HKEY key = nullptr;
    TemporaryRoot() {
        if (RegCreateKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, nullptr, &key, nullptr) != ERROR_SUCCESS)
            throw std::runtime_error("Create isolated test preferences");
    }
    ~TemporaryRoot() {
        if (key) RegCloseKey(key);
        RegDeleteTreeW(HKEY_CURRENT_USER, path.c_str());
    }
};
}
int main() {
    using namespace field_favorites;
    try {
        TemporaryRoot isolated;
        const std::string main = "StoryPack_Main", rizia = "StoryPack_Rizia";
        const std::string budget = "BaseGame.GovernmentBudget", event = "BaseGame.事件:Budget/Changed";
        const std::string alternate = "BaseGame.governmentbudget", stock = "RiziaDLC.Resources_Budget";
        Keys first, other, restarted;
        check(read(main, first, isolated.key) && first.empty(), "Start with empty preferences");
        check(set(main, budget, true, first, isolated.key), "Favorite the main resource");
        check(set(main, event, true, other, isolated.key) && other.size()==2, "Merge a stale second instance");
        check(set(main, alternate, true, first, isolated.key), "Case-sensitive raw keys");
        check(read(main, restarted, isolated.key) && restarted==Keys{budget,event,alternate}, "Fresh process state recovers the exact raw keys");
        check(set(main, budget, true, first, isolated.key) && first.size()==3, "Adding the same field is idempotent");
        check(!set(main, stock, true, first, isolated.key), "Reject cross-campaign keys");
        check(!set("Unknown", budget, true, first, isolated.key), "Reject unknown campaigns");
        check(set(rizia, stock, true, other, isolated.key) && other==Keys{stock}, "DLC has independent favorites");
        check(set(main, budget, false, first, isolated.key) && first==Keys{event,alternate}, "Remove a favorite");
        check(set(main, budget, false, other, isolated.key) && other==first, "Repeated removal preserves other fields");
        check(read(rizia, restarted, isolated.key) && restarted==Keys{stock}, "Removing a main field preserves DLC");
        check(set(rizia, stock, false, other, isolated.key) && read(rizia, restarted, isolated.key) && restarted.empty(), "Removing the last favorite persists");
        const char corrupt[]="12345:x";
        RegSetKeyValueW(isolated.key,settings,valueName(main),REG_BINARY,corrupt,sizeof(corrupt)-1);
        check(!read(main, first, isolated.key) && first==Keys{event,alternate}, "Incomplete data preserves the previous in-memory snapshot");
        check(!set(main, event, false, first, isolated.key), "Do not overwrite unreadable preferences");
        std::cout << "PASS: " << checks << " field favorite checks\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << " checks: " << e.what() << "\n";
        return 1;
    }
}
