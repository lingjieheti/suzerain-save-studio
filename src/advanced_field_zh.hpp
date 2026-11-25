#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include "advanced_zh_dictionary.hpp"
#include "win_text.hpp"

// A display-only, literal gloss of internal names. Never use it as a save key.
inline bool fieldUpper(char c){return c>='A'&&c<='Z';}
inline bool fieldLower(char c){return c>='a'&&c<='z';}
inline bool fieldDigit(char c){return c>='0'&&c<='9';}
inline std::vector<std::string> advancedTokens(const std::string& segment){
    std::vector<std::string> out;
    for(size_t i=0;i<segment.size();){
        size_t start=i;
        if(fieldDigit(segment[i])){while(i<segment.size()&&fieldDigit(segment[i]))++i;}
        else if(fieldUpper(segment[i])){
            while(i<segment.size()&&fieldUpper(segment[i]))++i;
            if(i-start>1&&i<segment.size()&&fieldLower(segment[i]))--i;
            else while(i<segment.size()&&fieldLower(segment[i]))++i;
        }else if(fieldLower(segment[i])){while(i<segment.size()&&fieldLower(segment[i]))++i;}
        else{++i;continue;}
        out.push_back(segment.substr(start,i-start));
    }
    return out;
}
inline const wchar_t* advancedZhWord(const std::string& word){
    const auto* begin=advancedZhWords;const auto* end=begin+sizeof(advancedZhWords)/sizeof(advancedZhWords[0]);
    auto it=std::lower_bound(begin,end,word,[](const auto& row,const std::string& key){return std::strcmp(row.key,key.c_str())<0;});
    return it!=end&&word==it->key?it->zh:nullptr;
}
inline const wchar_t* advancedZhPhrase(const std::string& phrase){
    const auto* begin=advancedZhPhrases;const auto* end=begin+sizeof(advancedZhPhrases)/sizeof(advancedZhPhrases[0]);
    auto it=std::lower_bound(begin,end,phrase,[](const auto& row,const std::string& key){return std::strcmp(row.key,key.c_str())<0;});
    return it!=end&&phrase==it->key?it->zh:nullptr;
}
inline const wchar_t* advancedZhExactSegment(const std::string& segment){
    const auto* begin=advancedZhSegments;const auto* end=begin+sizeof(advancedZhSegments)/sizeof(advancedZhSegments[0]);
    auto it=std::lower_bound(begin,end,segment,[](const auto& row,const std::string& key){return std::strcmp(row.key,key.c_str())<0;});
    return it!=end&&segment==it->key?it->zh:nullptr;
}
inline std::wstring advancedZhSegment(const std::string& segment){
    if(auto exact=advancedZhExactSegment(segment))return exact;
    // Custom names can mix scripts inside a single segment. The game glossary
    // splits ASCII identifiers only, so keep the whole UTF-8 name in this case.
    if(std::any_of(segment.begin(),segment.end(),[](unsigned char c){return c>=0x80;}))
        return wide(segment);
    // Retain ambiguous script codes as codes; do not invent gameplay meaning.
    if(segment=="InT"||segment=="EnT"||segment=="SnT"||segment=="SnO"||segment=="PSnW"||segment=="FPnT")return L"场景（"+std::wstring(segment.begin(),segment.end())+L"）";
    if(segment.size()>4&&segment.rfind("Turn",0)==0&&std::all_of(segment.begin()+4,segment.end(),fieldDigit))return L"第"+std::wstring(segment.begin()+4,segment.end())+L"回合";
    std::wstring out;
    auto tokens=advancedTokens(segment);
    for(size_t i=0;i<tokens.size();){
        std::string joined;const wchar_t* phrase=nullptr;size_t count=0;
        for(size_t j=i;j<tokens.size()&&j<i+advancedZhMaxPhraseWords;++j){
            if(!joined.empty())joined+='|';joined+=tokens[j];
            if(auto caption=advancedZhPhrase(joined)){phrase=caption;count=j-i+1;}
        }
        if(phrase){out+=phrase;i+=count;continue;}
        const auto& word=tokens[i++];
        if(auto value=advancedZhWord(word))out+=value;
        else out+=std::wstring(word.begin(),word.end());
    }
    return out.empty()?std::wstring(segment.begin(),segment.end()):out;
}
inline std::wstring advancedZhLabel(const std::string& key){
    if(key=="RiziaDLC.Prologue_Summer_Court")return L"序章 · 夏季 · 宫廷";
    auto dot=key.find('.');std::string tail=key.substr(dot==std::string::npos?0:dot+1);
    std::wstring out;size_t at=0;
    while(at<tail.size()){
        auto split=tail.find('_',at);auto segment=tail.substr(at,split==std::string::npos?split:split-at);
        if(!segment.empty()){
            if(!out.empty())out+=L" · ";
            if(tail.rfind("Army_Unit_",0)==0&&segment=="Unit")out+=L"部队";
            else if(tail.rfind("Army_Unit_",0)==0&&segment=="Dead")out+=L"损失";
            else out+=advancedZhSegment(segment);
        }
        if(split==std::string::npos)break;at=split+1;
    }
    return out;
}
inline std::wstring compactFieldSearch(const std::wstring& text){
    std::wstring out;
    for(auto c:text){if(c==L' '||c==L'\t'||c==L'_'||c==L'·'||c==L'-')continue;out+=c;}
    return out;
}
