#pragma once
#include "gw_quest_restore.h"
#include "gw_pet_profile.h"
#include "gw_inventory.h"
#include "gw_effect_registry.h"
#include <array>
namespace fw::gw::quests::ranger_companion {
inline constexpr unsigned id=76,experience=250;
// Complete secondary-trial capture58991..58996;446 was already learned in
// primary Ranger acceptance14859..14862 and was therefore not taught twice.
inline constexpr std::array<std::uint32_t,3> grants{436,446,411};
struct Eligibility {bool prophecies=false,presearing=false;unsigned primary=0,secondary=0;};
struct Interaction {skills::effects::Scope selected,current;std::string_view name;unsigned map=0;bool reachable=false;};
inline bool eligible(const Profile& quests,const Eligibility& e){
    constexpr std::array<unsigned,7> tests{0,85,86,84,82,81,83};
    if(!e.prophecies||!e.presearing||e.primary<1||e.primary>6||e.secondary>6||e.secondary==e.primary||
       (e.primary!=2&&e.secondary!=2&&e.secondary!=0))return false;
    const auto found=quests.records.find(tests[e.primary]);
    return found!=quests.records.end()&&found->second.stage==Stage::rewarded;
}
inline bool matches(const Interaction& npc){return npc.selected.instance&&npc.selected.actor&&npc.selected.generation&&
    npc.selected==npc.current&&npc.name=="Master Ranger Nente"&&npc.map==162&&npc.reachable;}
inline bool valid_progress(const Progress& p){return p.cycle&&!p.timed&&p.objective_credits.empty();}
inline std::optional<Profile> accept(const Profile& current,unsigned cycle,const Eligibility& e,const pets::Profile& pet,const Interaction& npc){
    validate(current);pets::validate(pet);if(!eligible(current,e)||pet.pet||!matches(npc))return {};
    auto next=quests::accept(current,id);if(!next||next->records.at(id).cycle!=cycle)return {};return next;
}
inline bool objective_ready(const Profile& current,const Eligibility& e,const pets::Profile& pet){
    pets::validate(pet);return eligible(current,e)&&pet.pet&&pet.pet->species==pets::Species::melandrus_stalker;
}
inline std::optional<Profile> complete(const Profile& current,unsigned cycle,const Eligibility& e,const pets::Profile& pet){
    validate(current);const auto found=current.records.find(id);
    if(found==current.records.end()||!valid_progress(found->second)||!objective_ready(current,e,pet))return {};
    return advance(current,id,cycle,Stage::active,Stage::completed);
}
inline std::optional<Profile> reward(const Profile& current,unsigned cycle,const Eligibility& e,const pets::Profile& pet,const Interaction& npc){
    validate(current);const auto found=current.records.find(id);
    if(found==current.records.end()||!valid_progress(found->second)||!objective_ready(current,e,pet)||!matches(npc))return {};
    return advance(current,id,cycle,Stage::completed,Stage::rewarded);
}
inline bool valid_transition(const Profile& previous,const Profile& next,const Eligibility& e,const pets::Profile& pet){
    validate(previous);validate(next);const auto before=previous.records.find(id),after=next.records.find(id);
    if(before==previous.records.end()&&after==next.records.end())return true;
    if(before!=previous.records.end()&&after!=next.records.end()&&before->second==after->second)return true;
    if(after==next.records.end()||!valid_progress(after->second))return false;
    std::optional<Profile> proposed;
    if(after->second.stage==Stage::active&&eligible(previous,e)&&!pet.pet)proposed=quests::accept(previous,id);
    if(after->second.stage==Stage::completed)proposed=complete(previous,after->second.cycle,e,pet);
    if(before!=previous.records.end()&&after->second.stage==Stage::abandoned)proposed=quests::abandon(previous,id,before->second.cycle);
    return proposed&&*proposed==next;
}
inline items::Definition starter_bow(unsigned identity){
    items::Definition d;d.id=identity;d.file=2147493028u;d.type=5;d.interaction=572526592u;d.model=477;d.quantity=1;
    d.encoded_name={8767,34752,49756,14222};d.modifiers={639107840,616038656,2760377603};return d;
}
// PRIVATE duplicate policy, consistent with first Ranger(no bow packet) versus
// second off-profession(with bow) acceptance. The archive does not distinguish
// class-based from inventory-based retail issuance. Do not fabricate ownership
// or unrestricted profession metadata: captured byte2, customization UNKNOWN.
inline bool has_starter_bow(const inventory::Profile& current){
    const auto expected=starter_bow(1);
    for(const auto& [id,item]:current.items){const auto bag=current.bags.find(item.bag);const auto& d=item.definition;
        if(bag==current.bags.end()||(bag->second.type!=1&&bag->second.type!=2))continue;
        if(d.type==5&&d.model==477&&(d.file&0x7fffffffu)==9380&&d.encoded_name==expected.encoded_name&&
           d.modifiers.size()>=expected.modifiers.size()&&std::equal(expected.modifiers.begin(),expected.modifiers.end(),d.modifiers.begin()))return true;
    }
    return false;
}
struct BowGrant {inventory::Profile profile;inventory::Batch packets;std::optional<unsigned> item;};
inline std::optional<BowGrant> grant_bow(const inventory::Profile& current){
    inventory::validate_character(current);if(has_starter_bow(current))return BowGrant{current,{},{}};
    if(current.next_item>=65535||current.items.size()>=inventory::maximum_items)return {};
    inventory::Item item;item.definition=starter_bow(current.next_item);item.profession=2;
    for(const auto& [id,bag]:current.bags){
        if(bag.type!=1||!inventory::accepts_item(bag,item,{}))continue;
        for(unsigned slot=0;slot<bag.capacity;++slot){
            if(std::any_of(current.items.begin(),current.items.end(),[&](const auto& row){return row.second.bag==id&&row.second.slot==slot;}))continue;
            BowGrant out{current,{},item.definition.id};item.bag=id;item.slot=static_cast<std::uint8_t>(slot);
            out.packets=inventory::item_packets(item);out.packets.push_back(items::location_2022(current.inventory,item.definition.id,id,item.slot));
            ++out.profile.next_item;out.profile.items.emplace(item.definition.id,item);inventory::validate_character(out.profile);return out;
        }
    }
    return {};
}
inline Catalog bind_markers(const Catalog& source,unsigned map){
    auto result=source;const auto found=result.find(id);if(found==result.end())return result;
    if(map!=162){found->second.entry.x=found->second.entry.y=std::bit_cast<float>(0x7f800000u);}
    if(found->second.completed_variant){found->second.completed_variant=std::make_shared<Content>(*found->second.completed_variant);
        if(map!=162)found->second.completed_variant->entry.x=found->second.completed_variant->entry.y=std::bit_cast<float>(0x7f800000u);}
    return result;
}
}
