#pragma once

struct SaveQuery {
    struct Word { std::wstring raw, folded; };
    std::vector<Word> words;
    std::vector<std::wstring> turns;
    std::vector<std::wstring> numbers;
};

inline bool turnDigits(const std::wstring& term) {
    return !term.empty() && term.find_first_not_of(L"0123456789")==std::wstring::npos;
}

// Recognize 第3回合 and 第 3 回合 before ordinary whitespace tokenization.
inline size_t chineseTurnEnd(const std::wstring& input,size_t at,std::wstring& number) {
    if(input[at]!=L'第')return at;
    size_t start=at+1;
    while(start<input.size()&&field_search::space(input[start]))++start;
    size_t end=start;
    while(end<input.size()&&input[end]>=L'0'&&input[end]<=L'9')++end;
    size_t label=end;
    while(label<input.size()&&field_search::space(input[label]))++label;
    if(start==end || input.compare(label,2,L"回合")!=0)return at;
    const size_t after=label+2;
    if(after<input.size()&&!field_search::space(input[after]))return at;
    number=input.substr(start,end-start);
    return after;
}

inline SaveQuery parseSaveQuery(const std::wstring& input) {
    SaveQuery query;
    const auto localizedTurn=field_search::fold(tr(L"回合"));
    auto isTurnLabel=[&](const std::wstring& word){
        return word==L"turn" || word==L"回合" || word==localizedTurn;
    };
    for(size_t at=0;at<input.size();) {
        if(field_search::space(input[at])){++at;continue;}
        std::wstring chineseTurn;
        const auto end=chineseTurnEnd(input,at,chineseTurn);
        if(end!=at){query.turns.push_back(std::move(chineseTurn));at=end;continue;}
        const size_t start=at;
        while(at<input.size()&&!field_search::space(input[at]))++at;
        const auto raw=input.substr(start,at-start);
        if(turnDigits(raw)) {
            if(!query.words.empty()&&isTurnLabel(query.words.back().folded)){
                query.words.pop_back();query.turns.push_back(raw);
            }else query.numbers.push_back(raw);
        } else {
            const auto word=field_search::fold(raw);
            const auto number=word.find_first_of(L"0123456789");
            if(number!=std::wstring::npos&&isTurnLabel(word.substr(0,number))&&turnDigits(word.substr(number)))
                query.turns.push_back(word.substr(number));
            else if(!word.empty())query.words.push_back({raw,word});
        }
    }
    return query;
}

inline bool fileNumberMatches(const std::wstring& text,const std::wstring& number) {
    // Keep year/date fragments searchable. Short numbers match complete groups
    // so that a query for 3 does not pick the 3 out of "Turn13.json".
    auto digit=[](wchar_t c){return c>=L'0' && c<=L'9';};
    for(auto at=text.find(number);at!=std::wstring::npos;at=text.find(number,at+1)){
        const auto end=at+number.size();
        if(number.size()>=4 || ((!at||!digit(text[at-1])) && (end==text.size()||!digit(text[end]))))return true;
    }
    return false;
}

struct SaveSlot { const wchar_t* prefix;const wchar_t* label;const wchar_t* traditional; };

inline const SaveSlot* saveSlot(const std::wstring& filename) {
    static constexpr SaveSlot types[]={
        {L"active",L"当前存档",L"當前存檔"}, {L"autosave",L"自动存档",L"自動存檔"},
        {L"manualsave",L"手动存档",L"手動存檔"}, {L"turnsave",L"回合存档",L"回合存檔"},
        {L"finalsave",L"最终存档",L"最終存檔"}
    };
    const auto end=std::min(filename.find(L'_'),filename.rfind(L'.'));
    const auto prefix=lower(filename.substr(0,end));
    for(const auto& type:types)if(prefix==type.prefix)return &type;
    return nullptr;
}

inline std::wstring saveSubtitle(const SaveSlot* slot,const std::wstring& campaign) {
    if(!slot)return campaign;
    const auto type=tr(slot->label);
    return campaign.empty()?type:type+L" · "+campaign;
}

inline bool matchesModified(const std::wstring& modified,const std::wstring& term) {
    // Compare against the displayed local date or clock, on component boundaries.
    if((term.size()==7 || term.size()==10) && term[4]==L'-')
        return modified.compare(0,term.size(),term)==0;
    if((term.size()==5 || term.size()==8) && term[2]==L':'){
        const auto clock=modified.find_first_not_of(L' ',modified.find(L' '));
        return clock!=std::wstring::npos && modified.compare(clock,term.size(),term)==0;
    }
    return false;
}

inline bool matchesSave(const rizia::discovery::Save& save,const SaveQuery& query,const SaveSlot* slot) {
    if(query.words.empty()&&query.turns.empty()&&query.numbers.empty())return true;
    const auto turn=wide(save.turn);
    for(const auto& requested:query.turns)if(requested!=turn)return false;
    const auto filename=save.path.filename().wstring();
    const auto campaign=wide(save.campaignName);
    for(const auto& number:query.numbers){
        const bool year=number.size()==4 && save.modifiedText.compare(0,4,number)==0;
        if(!year && number!=turn && !fileNumberMatches(filename,number) && !fileNumberMatches(campaign,number))return false;
    }
    if(query.words.empty())return true;
    std::wstring alias;
    if(slot)alias=std::wstring(slot->label)+L"|"+slot->traditional+L"|"+tr(slot->label);
    const bool sordland=save.story=="StoryPack_Main";
    const auto index=field_search::fold(filename+L"|"+campaign+L"|"+campaignLabel(save.story)+
        (sordland?L"|Sordland|索德兰|索德蘭|":L"|Rizia|里奇亚|里奇亞|")+alias);
    return std::all_of(query.words.begin(),query.words.end(),[&](const SaveQuery::Word& word){
        return index.find(word.folded)!=std::wstring::npos || matchesModified(save.modifiedText,word.raw);
    });
}
