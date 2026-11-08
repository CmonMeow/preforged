#pragma once
#include "gw_identification.h"
#include <filesystem>
#include <fstream>
#include <map>
namespace fw::gw::identification {
class Catalog {
    struct Recipe {std::string provenance;inventory::Item source,revealed;};
    std::map<inventory::Packet,Recipe> recipes_;
    static inventory::Packet bytes(std::string_view text){
        if(text.empty()||text.size()%2)throw std::invalid_argument("identification catalog hex length");
        inventory::Packet out;for(std::size_t i=0;i<text.size();i+=2){unsigned v=0;for(unsigned j=0;j<2;++j){auto c=text[i+j];unsigned n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:99;if(n>15)throw std::invalid_argument("identification catalog hex character");v=v*16+n;}out.push_back(v);}return out;
    }
    static inventory::Item normalized(inventory::Item item){item.definition.id=1;item.bag=1;item.slot=0;return item;}
    static inventory::Packet signature(inventory::Item item){auto p=inventory::baseline();p.next_item=2;p.items.emplace(1,normalized(std::move(item)));return inventory::encode(p);}
public:
    static constexpr std::uintmax_t maximum_file=16u*1024u*1024u;
    static constexpr std::size_t maximum_line=65536,maximum_recipes=4096;
    std::size_t size()const{return recipes_.size();}
    void load(const std::filesystem::path& path){
        if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>maximum_file)throw std::invalid_argument("identification catalog file bound exceeded");
        std::ifstream file(path);if(!file)throw std::invalid_argument("identification catalog unavailable");
        std::map<inventory::Packet,Recipe> next;std::string line;std::uintmax_t consumed=0;
        while(std::getline(file,line)){
            consumed+=line.size()+1;
            if(consumed>maximum_file)throw std::invalid_argument("identification catalog read bound exceeded");
            if(line.size()>maximum_line)throw std::invalid_argument("identification catalog line bound exceeded");
            if(line.empty()||line[0]=='#')continue;
            if(next.size()>=maximum_recipes)throw std::invalid_argument("identification catalog recipe bound exceeded");
            const auto tab=line.find('\t');if(tab!=64)throw std::invalid_argument("identification provenance SHA256 required");(void)bytes(line.substr(0,64));
            auto p=inventory::decode(bytes(line.substr(tab+1)));
            if(p.items.size()!=2||!p.items.contains(1)||!p.items.contains(2))throw std::invalid_argument("identification exact source/reveal pair required");
            Recipe r{line.substr(0,64),normalized(p.items.at(1)),normalized(p.items.at(2))};
            if((r.source.definition.interaction&1)||!preserves_target(r.source,r.revealed)||r.source.reviewed_armor!=r.revealed.reviewed_armor)
                throw std::invalid_argument("identification reveal changes immutable source");
            auto key=signature(r.source);
            if(!next.emplace(std::move(key),std::move(r)).second)throw std::invalid_argument("duplicate identification source");
        }
        if(file.bad())throw std::invalid_argument("identification catalog read failed");
        recipes_=std::move(next);
    }
    std::optional<inventory::Item> reveal(const inventory::Item& source)const{
        if(recipes_.empty())return {};
        const auto recipe=recipes_.find(signature(source));if(recipe==recipes_.end())return {};
        auto result=recipe->second.revealed;result.definition.id=source.definition.id;result.bag=source.bag;result.slot=source.slot;return result;
    }
};
}
