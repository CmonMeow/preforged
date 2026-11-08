#pragma once
#include "gw_inventory.h"
#include <tuple>

namespace fw::gw::collectors {
// Recipes are trusted server data. Client messages select a menu entry; they
// never supply a cost, a reward definition, or an inventory to debit.
struct Offer {
    unsigned key=0,map=0,profession=0;
    std::string collector,name,trophy_name;
    items::Definition trophy;
    unsigned cost=0;
    inventory::Item reward;
    bool customize=false;
};
inline bool trophy_matches(const items::Definition& a,const items::Definition& b){
    return a.model==b.model&&a.file==b.file&&a.type==b.type&&a.encoded_name==b.encoded_name&&
        a.modifiers==b.modifiers&&a.dye==b.dye&&a.unknown1==b.unknown1&&
        a.materials==b.materials&&a.unknown2==b.unknown2&&a.interaction==b.interaction&&a.price==b.price;
}
inline unsigned available(const inventory::Profile& p,const Offer& offer){
    unsigned count=0;
    for(const auto& [id,item]:p.items){const auto bag=p.bags.find(item.bag);
        if(bag!=p.bags.end()&&bag->second.type==1&&trophy_matches(item.definition,offer.trophy))count+=item.definition.quantity;}
    return count;
}
struct Proposal {inventory::Profile profile;inventory::Batch packets;unsigned reward=0;};
// Shared copy-first reward issuance after an authoritative trophy debit.
// Failure discards the local proposal, including all debit packets.
inline std::optional<Proposal> issue_reward(const inventory::Profile& current,const Offer& offer,
    std::u16string_view owner,Proposal out){
    auto reward=offer.reward;
    if(offer.customize)reward.customized=std::u16string(owner);
    else if(reward.profession&&*reward.profession==0&&!reward.customized&&
        (reward.definition.type==2||reward.definition.type==5||reward.definition.type==12||
         reward.definition.type==15||reward.definition.type==22||reward.definition.type==24||
         reward.definition.type==26||reward.definition.type==27||reward.definition.type==32||
         reward.definition.type==35||reward.definition.type==36))
        reward.customized=std::u16string{}; // Trusted catalog explicitly declares unrestricted issuance.
    // Captured Brownlow Devourer Eggs carry the native stackable flag.
    // Merge an exact reward into a carried stack before requiring a free slot,
    // following the same identity/metadata rules as ground-item pickup.
    if(offer.reward.definition.interaction&0x80000){
        inventory::Item* target=nullptr;
        for(auto& [id,item]:out.profile.items){const auto bag=out.profile.bags.find(item.bag);
            if(bag==out.profile.bags.end()||bag->second.type!=1||!inventory::accepts_item(bag->second,reward,owner)||item.definition.quantity>=250||
               item.profession!=reward.profession||item.customized!=reward.customized||
               item.reviewed_armor!=reward.reviewed_armor||!inventory::compatible(item.definition,reward.definition))continue;
            if(!target||std::tie(bag->second.index,item.slot,id)<
                std::tie(out.profile.bags.at(target->bag).index,target->slot,target->definition.id))target=&item;
        }
        if(target){
            ++target->definition.quantity;out.reward=target->definition.id;
            out.packets.push_back(items::quantity_2022(out.reward,target->definition.quantity));
            inventory::validate_character(out.profile);return out;
        }
    }
    if(out.profile.items.size()>=inventory::maximum_items||current.next_item>=65535)return {};
    for(const auto& [id,bag]:out.profile.bags){if(bag.type!=1||!inventory::accepts_item(bag,reward,owner))continue;
        for(unsigned slot=0;slot<bag.capacity;++slot){
            if(std::any_of(out.profile.items.begin(),out.profile.items.end(),[&](const auto& entry){return entry.second.bag==id&&entry.second.slot==slot;}))continue;
            auto item=reward;item.definition.id=out.profile.next_item++;item.bag=id;item.slot=static_cast<std::uint8_t>(slot);
            out.reward=item.definition.id;auto definitions=inventory::item_packets(item);
            out.packets.insert(out.packets.end(),definitions.begin(),definitions.end());
            out.packets.push_back(items::location_2022(current.inventory,out.reward,id,item.slot));
            out.profile.items.emplace(out.reward,std::move(item));inventory::validate_character(out.profile);return out;
        }
    }
    return {};
}
// Copy-first proposal: caller persists profile using its inventory revision/CAS
// and publishes packets only after that succeeds. A consumed stack can release
// the reward slot, but a partially consumed stack cannot manufacture capacity.
inline std::optional<Proposal> exchange(const inventory::Profile& current,const Offer& offer,
                                        unsigned map,unsigned profession,std::u16string_view owner){
    inventory::validate_character(current);
    if(!offer.key||!offer.map||map!=offer.map||!offer.cost||offer.cost>250||
       offer.trophy.type!=30||!offer.trophy.file||offer.trophy.encoded_name.empty()||
       (offer.profession&&offer.profession!=profession)||offer.reward.definition.quantity!=1||
       available(current,offer)<offer.cost)return {};
    if(offer.customize&&(owner.empty()||owner.size()>19||owner.find(u'\0')!=owner.npos))return {};
    Proposal out{current,{},{}};unsigned remaining=offer.cost;
    for(auto it=out.profile.items.begin();it!=out.profile.items.end()&&remaining;){
        const auto bag=out.profile.bags.find(it->second.bag);
        if(bag==out.profile.bags.end()||bag->second.type!=1||!trophy_matches(it->second.definition,offer.trophy)){++it;continue;}
        const auto count=std::min(remaining,it->second.definition.quantity);remaining-=count;
        auto& quantity=it->second.definition.quantity;quantity-=count;
        if(quantity){out.packets.push_back(items::quantity_2022(it->first,quantity));++it;}
        else{out.packets.push_back(items::detach_2022(current.inventory,it->first));out.packets.push_back(items::delete_2022(it->first));it=out.profile.items.erase(it);}
    }
    if(remaining)return {};
    return issue_reward(current,offer,owner,std::move(out));
}
// Runtime binds this to an actually visible, nearby NPC and invalidates on
// movement out of range, transfer, inventory mutation, or another interaction.
// Names matter: several distinct pre-Searing collectors share model1471.
struct Menu {
    unsigned actor=0,map=0;std::uint64_t revision=0,expires=0;std::string collector;
    std::vector<unsigned> offers;
    bool accepts(unsigned selected,unsigned npc,unsigned zone,std::uint64_t version,std::uint64_t now)const{
        return actor&&npc==actor&&map==zone&&version==revision&&now<expires&&
            std::find(offers.begin(),offers.end(),selected)!=offers.end();
    }
    void clear(){*this={};}
};
}
