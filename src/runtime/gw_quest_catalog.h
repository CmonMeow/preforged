#pragma once
#include "gw_quest_restore.h"
#include "common.h"
#include <charconv>
#include <filesystem>
#include <fstream>
namespace fw::gw::quests {
// FWQC1 rows: source SHA256, id, flags, x bits, y bits, plane, destination,
// source map, category/name/NPC UTF16LE hex, description/objectives hex.
// '.' is captured empty text; '-' is unloaded details. Never truncate codes.
inline Catalog read_catalog(std::istream& input){
    std::string line;if(!std::getline(input,line))throw std::invalid_argument("quest catalog version");
    if(!line.empty()&&line.back()=='\r')line.pop_back();const bool counted=line=="FWQC3",staged=line=="FWQC2"||counted;if(line!="FWQC1"&&!staged)throw std::invalid_argument("quest catalog version");
    Catalog result;std::size_t total=0;
    auto number=[](const std::string& text,std::uint32_t maximum=UINT32_MAX){std::uint32_t value=0;auto r=std::from_chars(text.data(),text.data()+text.size(),value);if(text.empty()||r.ec!=std::errc{}||r.ptr!=text.data()+text.size()||value>maximum)throw std::invalid_argument("quest catalog number");return value;};
    auto text=[](const std::string& hex,unsigned maximum){std::u16string value;if(hex==".")return value;if(hex.empty()||hex.size()%4||hex.size()>maximum*4)throw std::invalid_argument("quest catalog text bound");
        auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::invalid_argument("quest catalog text encoding");};
        for(std::size_t i=0;i<hex.size();i+=4){auto unit=digit(hex[i])*16+digit(hex[i+1])+(digit(hex[i+2])*16+digit(hex[i+3]))*256;if(!unit)throw std::invalid_argument("quest catalog embedded terminator");value.push_back(static_cast<char16_t>(unit));}return value;};
    while(std::getline(input,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();total+=line.size();if(total>4*1024*1024||result.size()>=4096)throw std::invalid_argument("quest catalog bound");
        if(line.empty())continue;auto fields=fw::split(line);if(fields.size()!=13+unsigned(staged)&&fields.size()!=14+unsigned(staged))throw std::invalid_argument("quest catalog fields");
        if(fields[0].size()!=64||fields[0].find_first_not_of("0123456789abcdef")!=fields[0].npos)throw std::invalid_argument("quest catalog provenance");
        Content content;content.source_sha256=fields[0];auto& entry=content.entry;entry.id=number(fields[1]);entry.state=number(fields[2]);
        entry.x=std::bit_cast<float>(number(fields[3]));entry.y=std::bit_cast<float>(number(fields[4]));
        entry.plane=static_cast<std::uint16_t>(number(fields[5],65535));entry.map_to=static_cast<std::uint16_t>(number(fields[6],65535));entry.map_from=static_cast<std::uint16_t>(number(fields[7],65535));
        entry.location=text(fields[8],7);entry.name=text(fields[9],7);entry.npc=text(fields[10],7);
        if((fields[11]=="-")!=(fields[12]=="-"))throw std::invalid_argument("partial quest catalog details");
        if(fields[11]!="-"){content.description=text(fields[11],127);content.objectives=text(fields[12],127);}
        if(fields.size()==14+unsigned(staged)){
            content.captured_character_argument=text(fields[13+unsigned(staged)],19);
            if(content.captured_character_argument->empty()||!content.description)throw std::invalid_argument("invalid quest character binding");
        }
        (void)add_2022(entry);if(content.description)(void)details_2022(entry.id,*content.description,*content.objectives);
        if(counted&&fields[13].starts_with("count:")){
            const auto credited=number(fields[13].substr(6),63);
            const auto active=result.find(entry.id);
            if(!credited||active==result.end()||active->second.completed_variant||
               credited!=active->second.objective_variants.size()+1||!content.description||!content.objectives)
                throw std::invalid_argument("counted quest requires contiguous captured variants before completion");
            active->second.objective_variants.emplace(credited,std::make_shared<Content>(std::move(content)));
        }else if(staged&&fields[13]=="completed"){
            const auto active=result.find(entry.id);
            if(active==result.end()||active->second.completed_variant)throw std::invalid_argument("completed quest requires one preceding active row");
            active->second.completed_variant=std::make_shared<Content>(std::move(content));
        }else{
            if(staged&&fields[13]!="active")throw std::invalid_argument("invalid quest content stage");
            if(!result.emplace(entry.id,std::move(content)).second)throw std::invalid_argument("duplicate quest catalog identity");
        }
    }
    if(input.bad())throw std::invalid_argument("quest catalog read failure");return result;
}
inline Catalog load_catalog(const std::optional<std::filesystem::path>& path){
    if(!path)return {};std::ifstream input(*path);if(!input)throw std::runtime_error("quest catalog unavailable");return read_catalog(input);
}
}
