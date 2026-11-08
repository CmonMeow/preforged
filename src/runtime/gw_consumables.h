#pragma once
#include "gw_inventory.h"
#include <filesystem>
#include <fstream>
#include <map>
namespace fw::gw::consumables {
struct MoralePlan {unsigned increase=0;bool party=true;bool recharge=false;};
inline bool valid(MoralePlan p){return (p.party&&!p.recharge&&(p.increase==5||p.increase==10))||(!p.party&&p.recharge&&p.increase==10);}
inline std::optional<MoralePlan> morale_plan(unsigned model){if(model==26784||model==21489)return MoralePlan{5};if(model==21227)return MoralePlan{10};if(model==30211)return MoralePlan{10,false,true};return {};}
inline bool honeycomb_model(unsigned model){return model==26784||model==21489;}
inline unsigned morale_next(unsigned value,unsigned increase=5){if(value<40||value>110||(increase!=5&&increase!=10))throw std::invalid_argument("morale range");return std::min(110u,value+increase);}
inline unsigned capacity(unsigned base,unsigned morale,int equipment){
    if(morale<40||morale>110||!base||base>1000000||equipment>1000000||equipment< -1000000)throw std::invalid_argument("morale capacity range");
    const auto result=static_cast<unsigned>(std::max(1,int(base*morale/100)+equipment));
    if(result>1000000)throw std::invalid_argument("morale capacity bound");
    return result; // Integral private policy: floor fractions.
}
class Catalog {
    std::map<inventory::Packet,MoralePlan> definitions_;
    static inventory::Packet signature(inventory::Item item){auto p=inventory::baseline();item.definition.id=1;item.definition.quantity=1;item.bag=1;item.slot=0;p.next_item=2;p.items.emplace(1,std::move(item));return inventory::encode(p);}
public:
    std::size_t size()const{return definitions_.size();}
    void load(const std::filesystem::path& path){
        constexpr std::uintmax_t limit=16*1024*1024;
        if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>limit)throw std::invalid_argument("consumable catalog file bound");
        std::ifstream file(path);if(!file)throw std::invalid_argument("consumable catalog unavailable");
        std::map<inventory::Packet,MoralePlan> next;std::string line;std::uintmax_t total=0;
        const auto hex=[](std::string_view text){inventory::Packet out;if(text.size()%2)throw std::invalid_argument("consumable catalog hex length");for(std::size_t i=0;i<text.size();i+=2){unsigned v=0;for(unsigned j=0;j<2;++j){auto c=text[i+j];auto n=c>='0'&&c<='9'?unsigned(c-'0'):c>='a'&&c<='f'?unsigned(c-'a'+10):99u;if(n>15)throw std::invalid_argument("consumable catalog hex character");v=v*16+n;}out.push_back(v);}return out;};
        while(std::getline(file,line)){total+=line.size()+1;if(total>limit||line.size()>65536)throw std::invalid_argument("consumable catalog read bound");if(line.empty()||line[0]=='#')continue;
            if(next.size()>=4096||line.find('\t')!=64)throw std::invalid_argument("consumable catalog recipe/provenance bound");(void)hex(std::string_view(line).substr(0,64));
            const auto profile=inventory::decode(hex(std::string_view(line).substr(65)));
            if(profile.items.size()!=1||!profile.items.contains(1)||!morale_plan(profile.items.at(1).definition.model)||profile.items.at(1).definition.quantity!=1)throw std::invalid_argument("reviewed morale consumable definition required");
            const auto& item=profile.items.at(1);
            if(!next.emplace(signature(item),*morale_plan(item.definition.model)).second)throw std::invalid_argument("duplicate consumable definition");
        }
        if(file.bad())throw std::invalid_argument("consumable catalog read failed");definitions_=std::move(next);
    }
    std::optional<MoralePlan> plan(const inventory::Item& item)const{
        if(definitions_.empty())return {};
        const auto found=definitions_.find(signature(item));return found==definitions_.end()?std::nullopt:std::optional(found->second);
    }
    bool honeycomb(const inventory::Item& item)const{return honeycomb_model(item.definition.model)&&bool(plan(item));}
};
struct Proposal {bool accepted=false;inventory::Profile profile;inventory::Batch packets;};
inline Proposal debit(const inventory::Profile& current,unsigned id,const Catalog& catalog,bool allowed){
    inventory::validate_character(current);Proposal out{false,current,{}};
    auto found=current.items.find(id);if(!allowed||found==current.items.end()||!catalog.plan(found->second))return out;
    auto bag=current.bags.find(found->second.bag);if(bag==current.bags.end()||bag->second.type!=1||!found->second.definition.quantity)return out;
    auto& quantity=out.profile.items.at(id).definition.quantity;
    if(--quantity)out.packets.push_back(items::quantity_2022(id,quantity));
    else{out.profile.items.erase(id);out.packets.push_back(items::detach_2022(current.inventory,id));out.packets.push_back(items::delete_2022(id));}
    inventory::validate_character(out.profile);out.accepted=true;return out;
}
}
