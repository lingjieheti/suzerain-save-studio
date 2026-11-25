#include "../src/advanced_field_zh.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

int main(int argc,char** argv) {
    std::size_t checks=0;
    auto check=[&](bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);};
    auto chinese=[](const std::wstring& text){return std::any_of(text.begin(),text.end(),[](wchar_t c){return c>=0x3400&&c<=0x9fff;});};
    try {
        if(argc!=2)throw std::runtime_error("Usage: test_advanced_zh <advanced_field_keys.txt>");
        check(advancedTokens("GovernmentBudget")==std::vector<std::string>{"Government","Budget"},"camel-case tokens");
        check(advancedTokens("HUDStat")==std::vector<std::string>{"HUD","Stat"},"acronym boundary");
        check(advancedTokens("Turn08")==std::vector<std::string>{"Turn","08"},"numeric boundary");
        check(advancedTokens("Tax2Policy")==std::vector<std::string>{"Tax","2","Policy"},"embedded number boundary");
        check(advancedTokens("___").empty(),"punctuation tokens");
        check(advancedZhLabel("BaseGame.Turn08")==L"第08回合","turn semantics");
        check(advancedZhLabel("RiziaDLC.Turn00")==L"第00回合","leading zero preserved");
        check(advancedZhLabel("BaseGame.123_004")==L"123 · 004","numeric segments preserved");
        check(advancedZhLabel("BaseGame.Zzqvzyxw_999")==L"Zzqvzyxw · 999","unknown label fallback");
        check(advancedZhLabel(u8"RiziaDLC.汉字")==L"汉字","Unicode-only custom name");
        check(advancedZhLabel(u8"RiziaDLC.Army_Unit_外交Mod🦊_InTraining")==L"军队 · 部队 · 外交Mod🦊 · 训练中",
              "Mixed-script custom name keeps supplementary characters and adjacent game terms");
        check(advancedZhWord("Zzqvzyxw")==nullptr,"unknown word lookup");
        check(advancedZhLabel("Zzqvzyxw")==L"Zzqvzyxw","unqualified fallback");
        check(advancedZhLabel("").empty(),"empty label");
        check(advancedZhLabel("BaseGame.GovernmentBudget").find(L"预算")!=std::wstring::npos,"budget semantics");
        check(advancedZhLabel("RiziaDLC.Resources_Energy").find(L"能源")!=std::wstring::npos,"energy semantics");
        check(advancedZhLabel("BaseGame.Tax_Policy").find(L"税")!=std::wstring::npos,"tax semantics");
        check(advancedZhSegment("CapitalPunishment")==L"死刑","capital punishment phrase");
        check(advancedZhSegment("CapitalPolice")==L"首都警察","capital police phrase");
        check(advancedZhSegment("CapitalWithdrawals").find(L"资本")==0,"capital withdrawals uses financial sense");
        check(advancedZhSegment("FellOffWagon")==L"故态复萌","idiom phrase");
        check(compactFieldSearch(L"能源 · 生产")==L"能源生产","Chinese multi-word search");
        check(advancedZhSegment("DefendFreeSpeech")==L"维护言论自由","natural free-speech phrase");
        check(advancedZhSegment("DoNothing")==L"不采取行动","natural no-action phrase");
        check(advancedZhSegment("MedicalSchool")==L"医学院","medical school phrase");
        check(advancedZhLabel("RiziaDLC.Prologue_Summer_Court").find(L"宫廷")!=std::wstring::npos,"court context");
        check(advancedZhLabel("RiziaDLC.Army_Unit_Bomber_InTraining")==L"军队 · 部队 · 轰炸机 · 训练中","army training context");
        for(const std::string scene:{"InT","EnT","SnT"}) {
            const auto shown=advancedZhLabel("BaseGame."+scene);
            check(shown.find(L"场景")!=std::wstring::npos,"scene-code context");
            check(shown.find(std::wstring(scene.begin(),scene.end()))!=std::wstring::npos,"ambiguous scene code discarded");
        }
        std::ifstream list(argv[1]);
        check(static_cast<bool>(list),"cannot open key list");
        std::set<std::string> seen,words,missing;
        std::size_t mainCount=0,riziaCount=0,labelsWithoutChinese=0;
        for(std::string key;std::getline(list,key);) {
            if(!key.empty()&&key.back()=='\r')key.pop_back();
            if(key.empty())continue;
            const auto original=key;
            check(seen.insert(key).second,"duplicate key");
            if(key.rfind("BaseGame.",0)==0)++mainCount;
            else if(key.rfind("RiziaDLC.",0)==0)++riziaCount;
            else throw std::runtime_error("unsupported key prefix");
            const auto shown=advancedZhLabel(key);
            check(!shown.empty(),"empty field label");
            check(shown.find(L'_')==std::wstring::npos,"raw underscores leaked");
            const auto tail=key.substr(key.find('.')+1);
            check(shown!=std::wstring(tail.begin(),tail.end()),"raw field name returned");
            if(!chinese(shown)){++labelsWithoutChinese;std::cerr<<"NO_CHINESE "<<key<<'\n';}
            for(std::size_t at=0;at<tail.size();) {
                const auto split=tail.find('_',at);
                const auto segment=tail.substr(at,split==std::string::npos?split:split-at);
                for(const auto& token:advancedTokens(segment)) {
                    if(std::all_of(token.begin(),token.end(),fieldDigit))continue;
                    words.insert(token);
                    const auto* translation=advancedZhWord(token);
                    if(!translation||!*translation)missing.insert(token);
                }
                if(split==std::string::npos)break;
                at=split+1;
            }
            check(key==original,"source key mutated");
            check(advancedZhLabel(key)==shown,"translation is unstable");
        }
        for(const auto& word:missing)std::cerr<<"MISSING_WORD "<<word<<'\n';
        check(mainCount==1480&&riziaCount==2617,"evidence field inventory differs");
        check(missing.empty(),"untranslated tokens remain");
        check(labelsWithoutChinese==0,"field labels without Chinese remain");
        std::cout<<"PASS: "<<checks<<" advanced-label checks; Main="<<mainCount<<", Rizia="<<riziaCount
                 <<", unique words="<<words.size()<<", missing="<<missing.size()<<'\n';
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';return 1;}
}
