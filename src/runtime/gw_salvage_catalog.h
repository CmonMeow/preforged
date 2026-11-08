#pragma once
#include "gw_salvage.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
namespace fw::gw::salvage {
// Reviewed exact component assets: three FWI items (1 source,2 retained,
// 3 extracted). No name/model heuristic creates an output definition.
class Catalog {
    struct Recipe {std::string provenance;inventory::Item source,retained,output;};
    std::map<inventory::Packet,Recipe> recipes_;
    static inventory::Packet bytes(std::string_view text){
        if(text.empty()||text.size()%2)throw std::invalid_argument("salvage catalog hex length");
        inventory::Packet out;for(std::size_t i=0;i<text.size();i+=2){unsigned v=0;for(unsigned j=0;j<2;++j){const auto c=text[i+j];unsigned n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:99;if(n>15)throw std::invalid_argument("salvage catalog hex character");v=v*16+n;}out.push_back(v);}return out;
    }
    static inventory::Packet signature(inventory::Item item){
        auto p=inventory::baseline();item.definition.id=1;item.bag=1;item.slot=0;p.next_item=2;p.items.emplace(1,std::move(item));return inventory::encode(p);
    }
public:
    static constexpr std::uintmax_t maximum_file=16u*1024u*1024u;
    static constexpr std::size_t maximum_line=65536,maximum_recipes=4096;
    std::size_t size()const{return recipes_.size();}
    void load(const std::filesystem::path& path){
        if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>maximum_file)
            throw std::invalid_argument("salvage catalog file bound exceeded");
        std::ifstream file(path);if(!file)throw std::invalid_argument("salvage catalog unavailable");
        std::map<inventory::Packet,Recipe> next;std::string line;std::uintmax_t consumed=0;
        while(std::getline(file,line)){
            consumed+=line.size()+1;
            if(consumed>maximum_file)throw std::invalid_argument("salvage catalog read bound exceeded");
            if(line.size()>maximum_line)throw std::invalid_argument("salvage catalog line bound exceeded");
            if(line.empty()||line[0]=='#')continue;
            if(next.size()>=maximum_recipes)throw std::invalid_argument("salvage catalog recipe bound exceeded");
            const auto tab=line.find('\t');if(tab!=64)throw std::invalid_argument("salvage provenance SHA256 required");(void)bytes(line.substr(0,64));
            auto p=inventory::decode(bytes(line.substr(tab+1)));if(p.items.size()!=3||!p.items.contains(1)||!p.items.contains(2)||!p.items.contains(3))throw std::invalid_argument("salvage catalog exact triple required");
            Recipe r{line.substr(0,64),p.items.at(1),p.items.at(2),p.items.at(3)};
            if(r.source.definition.quantity!=1||r.retained.definition.quantity!=1||r.output.definition.type!=8||r.output.definition.quantity!=1)throw std::invalid_argument("salvage component definitions invalid");
            auto expected=r.source;expected.definition.modifiers=r.retained.definition.modifiers;
            if(signature(expected)!=signature(r.retained)||r.source.definition.modifiers.size()!=r.retained.definition.modifiers.size()+1)throw std::invalid_argument("salvage retention changes base identity");
            auto words=r.source.definition.modifiers;for(auto value:r.retained.definition.modifiers){auto at=std::find(words.begin(),words.end(),value);if(at==words.end())throw std::invalid_argument("salvage retention adds modifiers");words.erase(at);}if(words.size()!=1)throw std::invalid_argument("salvage component removal ambiguous");
            auto key=signature(r.source);
            if(!next.emplace(std::move(key),std::move(r)).second)throw std::invalid_argument("duplicate salvage source");
        }
        if(file.bad())throw std::invalid_argument("salvage catalog read failed");
        recipes_=std::move(next);
    }
    std::optional<Outcome> component(const inventory::Profile& profile,std::uint32_t target)const{
        const auto found=profile.items.find(target);if(found==profile.items.end()||recipes_.empty())return {};
        const auto recipe=recipes_.find(signature(found->second));if(recipe==recipes_.end())return {};
        const auto& r=recipe->second;
        Outcome out;out.expected_inventory=inventory::encode(profile);out.mode=Mode::component;out.reviewed=true;
        out.retained=found->second;out.retained->definition.modifiers=r.retained.definition.modifiers;
        auto item=r.output;item.definition.id=0;item.bag=0;item.slot=0;out.outputs.push_back(std::move(item));return out;
    }
};
}
