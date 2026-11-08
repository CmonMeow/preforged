#pragma once
#include "gw_inventory.h"
#include <array>
namespace fw::gw::inventory::dye {
// Native7c3840 ->7c6d40(property24e): first matching raw property's low18bits.
// Vial color does NOT come from the vial's own base DyeInfo appearance.
inline std::optional<std::uint32_t> colors(const items::Definition& item){
    if(item.type!=10)return {};
    std::optional<std::uint32_t> result;
    for(auto word:item.modifiers)if((word&0x3ff00000u)==0x24e00000u){
        if(result)return {};result=word&0x3ffffu;
    }
    if(!result||!*result||*result>0xffff)return {};
    for(auto packed=*result;packed;packed>>=4)if((packed&15)<2||(packed&15)>13)return {};
    return result;
}
inline std::array<unsigned,16> multiset(std::uint32_t packed){
    std::array<unsigned,16> result{};for(;packed;packed>>=4)++result[packed&15];return result;
}
// Original4f22b0: target forbids interaction bit200 and type10, must belong
// to the local inventory owner. Authoritative owned placement supplies owner.
inline bool eligible(const Profile& profile,const Item& target){
    const auto bag=profile.bags.find(target.bag);
    // Original352->318 establishes the installed Item+8 owning-bag link.
    // Canonical durable bag0 is not an unowned item; resolve its container link.
    const bool installed_container=target.definition.type==3&&installed(target)&&target.slot==0&&
        std::count_if(profile.bags.begin(),profile.bags.end(),[&](const auto& entry){
            return entry.second.type==1&&entry.second.container==target.definition.id;})==1;
    return target.definition.type!=10&&!(target.definition.interaction&0x200)&&target.definition.quantity==1&&
           (installed_container||(bag!=profile.bags.end()&&(bag->second.type==1||bag->second.type==2)));
}
struct Proposal {Profile profile;std::uint32_t target=0;std::uint16_t color=0;std::map<std::uint32_t,unsigned> consumed;};
inline std::optional<Proposal> apply(const Profile& profile,std::uint32_t target,std::span<const std::uint32_t> vials){
    if(vials.empty()||vials.size()>4)return {};
    const auto ti=profile.items.find(target);if(ti==profile.items.end()||!eligible(profile,ti->second))return {};
    Proposal plan{profile,target};std::uint32_t mixed=0;unsigned count=0;bool old=false,modern=false;
    for(auto id:vials){
        const auto vi=profile.items.find(id);if(id==target||vi==profile.items.end())return {};
        const auto bag=profile.bags.find(vi->second.bag);if(bag==profile.bags.end()||bag->second.type!=1)return {};
        auto packed=colors(vi->second.definition);if(!packed)return {};
        old|=*packed>15;modern|=*packed<=15;
        if(old&&modern)return {}; // Original4f35e0 explicitly rejects this mix.
        auto& amount=plan.consumed[id];if(++amount>vi->second.definition.quantity)return {};
        // Original4f3740 appends each vial's low nibbles in this exact order.
        for(auto value=*packed;value;value>>=4){if(++count>4)return {};mixed=(mixed<<4)|(value&15);}
    }
    // Bag.html: inventory containers accept single colors, not mixtures.
    // Count expanded legacy-vial nibbles too, not merely selected vial IDs.
    if(ti->second.definition.type==3&&count!=1)return {};
    const auto frequencies=multiset(mixed);
    if(count>1&&std::count_if(frequencies.begin(),frequencies.end(),[](auto n){return n!=0;})==1)return {};
    if((ti->second.definition.interaction&0x20000000u)&&multiset(ti->second.definition.dye)==frequencies)return {};
    plan.color=static_cast<std::uint16_t>(mixed);plan.profile.items.at(target).definition.dye=plan.color;
    for(const auto& [id,amount]:plan.consumed){auto& quantity=plan.profile.items.at(id).definition.quantity;quantity-=amount;if(!quantity)plan.profile.items.erase(id);}
    validate_character(plan.profile);return plan;
}
}
