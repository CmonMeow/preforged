#pragma once
#include "gw_inventory.h"
#include <array>
#include <tuple>
namespace fw::gw::inventory::presearing {
// Equipped starter records from archived profession playthroughs. These are
// captured Hammer, Bow, Holy Rod, Truncheon, Cane and Elemental Rod
// definitions, not the later Grawl quest's Starter Sword.
inline std::optional<Profile> creation_weapon(const Profile& current,unsigned primary,std::u16string_view owner){
    validate_character(current);
    if(primary<1||primary>6||owner.empty()||owner.size()>19||owner.find(u'\0')!=owner.npos||
       current.next_item>=65535||current.items.size()>=maximum_items||!current.bags.contains(2))return {};
    if(std::any_of(current.items.begin(),current.items.end(),[](const auto& p){return p.second.bag==2&&p.second.slot==0;}))return {};
    auto proposed=current;Item item;auto& d=item.definition;d.id=proposed.next_item++;d.quantity=1;
    item.bag=2;item.slot=0;item.profession=static_cast<std::uint8_t>(primary);item.customized=std::u16string(owner);
    if(primary==1){
        d.model=1699;d.file=39776;d.type=15;d.interaction=572526592;d.encoded_name={9301,47099,51777,17807};
        const std::array<std::uint32_t,4> words{616038400,2760377603,2761455616,3221225472};d.modifiers=items::modifiers::capture_to_wire(words);
    }else if(primary==2){
        // Ranger equipment capture line25; raw source/hash in starter-bow-provenance.json.
        d.model=477;d.file=9380;d.type=5;d.interaction=572526592;d.encoded_name={8767,34752,49756,14222};
        const std::array<std::uint32_t,5> words{639107840,616038656,2760377603,2761455616,3221225472};d.modifiers=items::modifiers::capture_to_wire(words);
    }else if(primary>=3&&primary<=5){
        // Exact equipped definitions, source rows and hash preserved in
        // data/gw/quests/starter-caster-provenance.json. Opaque words stay opaque.
        d.type=22;d.interaction=570429440;
        std::array<std::uint32_t,5> words{};
        if(primary==3){d.model=2787;d.file=94904;d.encoded_name={9762,60084,41282,6389};words={616039680,647495684,2760377603,2761455616,3221225472};}
        if(primary==4){d.model=2694;d.file=94936;d.encoded_name={9687,39411,42256,4016};words={616040192,647495682,2760377603,2761455616,3221225472};}
        if(primary==5){d.model=2652;d.file=93541;d.encoded_name={9651,48088,37012,21300};words={616039936,647495680,2760377603,2761455616,3221225472};}
        d.modifiers=items::modifiers::capture_to_wire(words);
    }else{
        d.model=2742;d.file=94937;d.type=22;d.interaction=570429440;d.encoded_name={9725,51251,46412,18024};
        const std::array<std::uint32_t,5> words{616039680,647495684,2760377603,2761455616,3221225472};d.modifiers=items::modifiers::capture_to_wire(words);
    }
    proposed.weapons.sets[0].weapon=d.id;proposed.weapons.active=0;
    proposed.items.emplace(d.id,std::move(item));validate_character(proposed);return proposed;
}
// Archived E/W ground-item records: model2566/file23704/type21, decoded
// Shimmering Scale, resource units9558/52505/35705/32767. Numeric item IDs
// are session allocations and must not identify the objective item.
inline bool shimmering_scale(const Item& item){
    const auto& d=item.definition;
    return d.model==2566&&d.file==23704&&d.type==21&&d.quantity>0&&
        d.encoded_name==std::u16string({9558,52505,35705,32767});
}
// Immutable captured metadata; IDs are allocated by the world, not copied from
// the observed item489. Unknown profession cache byte11 is not a restriction.
inline items::Definition shimmering_scale_definition(){
    items::Definition d;d.file=23704;d.model=2566;d.type=21;d.quantity=1;
    d.interaction=537399809;d.encoded_name={9558,52505,35705,32767};
    const std::array<std::uint32_t,1> modifiers{3221225472u};
    d.modifiers=items::modifiers::capture_to_wire(modifiers);return d;
}
inline std::optional<std::uint32_t> carried_shimmering_scale(const Profile& current){
    validate_character(current);
    // Turn-in is planned against both the session's wire profile and the
    // durable account profile. Item IDs can reorder after ownership transfer;
    // bag index and slot survive projection and select the same physical item.
    const Item* selected=nullptr;
    for(const auto& [id,item]:current.items){
        const auto bag=current.bags.find(item.bag);
        if(bag==current.bags.end()||bag->second.type!=1||!shimmering_scale(item))continue;
        if(!selected||std::tie(bag->second.index,item.slot)<
            std::tie(current.bags.at(selected->bag).index,selected->slot))selected=&item;
    }
    return selected?std::optional<std::uint32_t>{selected->definition.id}:std::nullopt;
}
inline std::optional<Profile> consume_shimmering_scale(const Profile& current){
    if(const auto id=carried_shimmering_scale(current)){
        auto proposed=current;auto& scale=proposed.items.at(*id);
        if(scale.definition.quantity>1)--scale.definition.quantity;
        else proposed.items.erase(*id);
        validate_character(proposed);return proposed;
    }
    return {}; // Never complete/consume a lookalike trophy or a bank item.
}
// W/E capture line3197, immediately after quest75 acceptance at3195.
// Source SHA2563388d5edbe0ca0488f91d13244d6ec89670d14bed93b8ef0790b3fdaf7f283ff.
// IDs and placement are owned-server allocations, never copied from item749.
inline std::optional<Profile> grant_starter_sword(const Profile& current,std::u16string_view character){
    validate_character(current);
    if(character.empty()||character.size()>19||character.find(u'\0')!=character.npos)return {};
    if(current.items.size()>=maximum_items||current.next_item>=65535)return {};
    auto proposed=current;
    for(const auto& [bag_id,bag]:current.bags){
        if(bag.type!=1)continue;
        for(unsigned slot=0;slot<bag.capacity;++slot){
            if(std::any_of(current.items.begin(),current.items.end(),[&](const auto& pair){return pair.second.bag==bag_id&&pair.second.slot==slot;}))continue;
            Item item;auto& d=item.definition;d.id=proposed.next_item++;d.file=9454;d.type=27;
            d.interaction=570429440;d.model=2982;d.quantity=1;
            d.encoded_name={10001,48536,57935,32686};
            const std::array<std::uint32_t,3> captured{616038912,2760377090,3221225472u};
            d.modifiers=items::modifiers::capture_to_wire(captured);
            // Captured byte11 is the client's unknown cache sentinel, not a
            // restriction. This server-owned quest sword has explicit generic
            // weapon permission: Grawl Invasion gives it to secondary Warriors
            // too (local Grawl_Invasion/Starter_Sword wiki evidence). Keep the
            // raw observation in provenance; do not relax unknown armor rules.
            item.bag=bag_id;item.slot=static_cast<std::uint8_t>(slot);item.profession=0;item.customized=std::u16string(character);
            proposed.items.emplace(d.id,std::move(item));validate_character(proposed);return proposed;
        }
    }
    return {}; // A full inventory must not consume the quest acceptance.
}
// Quest42 wiki specifies Starter Bow. Reuse its captured creation definition;
// the capture does not prove Ivor's item instance/customization. Carried slot,
// fresh ID and customization below are explicit private ownership policy.
inline std::optional<Profile> grant_starter_bow(const Profile& current,std::u16string_view character){
    validate_character(current);
    if(current.items.size()>=maximum_items||current.next_item>=65535)return {};
    const auto reference=creation_weapon(baseline(),2,character);if(!reference)return {};
    for(const auto& [bag_id,bag]:current.bags){
        if(bag.type!=1)continue;
        for(unsigned slot=0;slot<bag.capacity;++slot){
            if(std::any_of(current.items.begin(),current.items.end(),[&](const auto& pair){return pair.second.bag==bag_id&&pair.second.slot==slot;}))continue;
            auto proposed=current;auto item=reference->items.begin()->second;
            item.definition.id=proposed.next_item++;item.bag=bag_id;item.slot=static_cast<std::uint8_t>(slot);
            // Bows have no profession restriction; secondary training may use
            // the same quest bow without becoming a primary Ranger.
            item.profession=0;
            proposed.items.emplace(item.definition.id,std::move(item));validate_character(proposed);return proposed;
        }
    }
    return {};
}
}
