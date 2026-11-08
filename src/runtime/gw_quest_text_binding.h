#pragma once
#include "gw_quest_restore.h"
#include "gw_character_name.h"
namespace fw::gw::quests {
// Explicitly reviewed captured literal argument (107 ... 1). Do not search for
// plain substrings: a resource code can numerically resemble a Unicode letter.
inline std::u16string bind_character_argument(const std::u16string& encoded,
    const std::u16string& captured_name,const std::u16string& current_name){
    (void)character_name_to_utf8(captured_name,19);
    (void)character_name_to_utf8(current_name,19);
    const auto pattern=std::u16string(1,char16_t(0x107))+captured_name+char16_t(1);
    const auto replacement=std::u16string(1,char16_t(0x107))+current_name+char16_t(1);
    auto out=encoded;std::size_t offset=0;unsigned count=0;
    while((offset=out.find(pattern,offset))!=out.npos){
        out.replace(offset,pattern.size(),replacement);offset+=replacement.size();++count;
    }
    if(!count)throw std::invalid_argument("reviewed quest character argument missing");
    if(out.size()>127||out.find(u'\0')!=out.npos)throw std::invalid_argument("bound quest details exceed native limit");
    return out;
}
// Bind a session copy; preserve catalog evidence and source hashes unchanged.
inline Content bind_quest_character_description(const Content& source,
    const std::u16string& captured_name,const std::u16string& current_name){
    if(!source.description)throw std::invalid_argument("quest description not loaded");
    auto bound=source;bound.description=bind_character_argument(*source.description,captured_name,current_name);
    if(!bound.objectives)throw std::invalid_argument("quest objectives not loaded");
    (void)details_2022(bound.entry.id,*bound.description,*bound.objectives);
    return bound;
}
inline Catalog bind_catalog_character(const Catalog& source,const std::u16string& name){
    auto result=source;
    for(auto& [id,content]:result){
        if(content.captured_character_argument){
            content=bind_quest_character_description(content,*content.captured_character_argument,name);
            content.captured_character_argument.reset();
        }
        if(content.completed_variant){
            auto variant=*content.completed_variant;
            if(variant.captured_character_argument){
                variant=bind_quest_character_description(variant,*variant.captured_character_argument,name);
                variant.captured_character_argument.reset();
            }
            content.completed_variant=std::make_shared<Content>(std::move(variant));
        }
        for(auto& [count,source_variant]:content.objective_variants){
            if(!source_variant)throw std::invalid_argument("missing objective variant");
            auto variant=*source_variant;
            if(variant.captured_character_argument){
                variant=bind_quest_character_description(variant,*variant.captured_character_argument,name);
                variant.captured_character_argument.reset();
            }
            source_variant=std::make_shared<Content>(std::move(variant));
        }
    }
    return result;
}
}
