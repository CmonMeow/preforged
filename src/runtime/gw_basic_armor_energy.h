#pragma once
#include "gw_inventory.h"
#include "gw_starter_shield_armor.h"
#include "gw_arthur_focus_armor.h"
#include "gw_item_base_armor.h"
#include "gw_energy_regeneration.h"
#include "gw_character_profile.h"
#include "gw_armor_attribute_runes.h"
#include <array>
namespace fw::gw::energy {
// Armor_rating.html: chest3/8, legs2/8, head/hands/feet1/8 each.
// Equipment slots2..6: chest, legs, head, feet, hands. An empty slot is
// unarmored; an equipped but unreviewed definition remains unknown.
enum class ArmorAspect {physical,elemental,neutral};
inline std::optional<std::array<int,5>> armor_locations_aspect(const inventory::Profile& profile,unsigned primary,ArmorAspect aspect){
    valid_primary(primary);std::array<int,5> result{};std::array<bool,5> seen{};
    constexpr std::array<unsigned,7> types{0,0,7,19,16,4,13};
    for(const auto&[id,item]:profile.items){
        const auto bag=profile.bags.find(item.bag);
        if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot<2||item.slot>6)continue;
        const auto index=item.slot-2;
        if(seen[index]||!item.reviewed_armor||item.profession!=primary||item.definition.quantity!=1||item.definition.type!=types[item.slot])return {};
        seen[index]=true;
        const auto base=items::base_armor::decode(inventory::presented_definition(item).modifiers);if(!base)return {};
        const auto insignia=aspect==ArmorAspect::neutral?
            equipment::attribute_runes::neutral_insignia_armor(item.definition.modifiers,primary):
            equipment::attribute_runes::insignia_armor(item.definition.modifiers,primary,aspect==ArmorAspect::physical);
        if(!insignia)return {};
        result[index]=*base+(aspect==ArmorAspect::physical?item.reviewed_armor->physical:
            aspect==ArmorAspect::elemental?item.reviewed_armor->elemental:0)+*insignia;
    }
    const auto shield=items::shield_armor::starter_bonus(profile)+items::arthur_focus::armor_bonus(profile);
    for(auto& location:result)location+=static_cast<int>(shield);
    return result;
}
inline std::optional<std::array<int,5>> armor_locations(const inventory::Profile& profile,unsigned primary,bool physical){
    return armor_locations_aspect(profile,primary,physical?ArmorAspect::physical:ArmorAspect::elemental);
}
inline std::optional<std::array<int,5>> physical_armor_locations(const inventory::Profile& profile,unsigned primary){return armor_locations(profile,primary,true);}
inline std::optional<std::array<int,5>> elemental_armor_locations(const inventory::Profile& profile,unsigned primary){return armor_locations(profile,primary,false);}
inline std::optional<std::array<int,5>> neutral_armor_locations(const inventory::Profile& profile,unsigned primary){return armor_locations_aspect(profile,primary,ArmorAspect::neutral);}
inline std::optional<int> armor_hit(const std::array<int,5>& locations,unsigned bucket){
    if(bucket>=8)return {};
    constexpr std::array<unsigned,8> slots{0,0,0,1,1,2,3,4};
    return locations[slots[bucket]];
}
inline std::optional<int> physical_armor_hit(const std::array<int,5>& locations,unsigned bucket){return armor_hit(locations,bucket);}
inline inventory::ReviewedArmor private_maximum_armor(unsigned primary){
    valid_primary(primary);return {static_cast<std::uint16_t>(primary==1||primary==9?80:primary==2||primary==7||primary==10?70:60),
        static_cast<std::uint16_t>(primary==1?20:0),static_cast<std::uint16_t>(primary==2?30:0)};
}
inline std::optional<int> fully_modeled_armor_aspect(const inventory::Profile& profile,unsigned primary,ArmorAspect aspect){
    valid_primary(primary);
    constexpr std::array<unsigned,7> types{0,0,7,19,16,4,13};
    std::array<std::optional<int>,5> pieces{};bool any=false;
    for(const auto&[id,item]:profile.items){
        const auto bag=profile.bags.find(item.bag);
        if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot<2||item.slot>6)continue;
        any=true;if(!item.reviewed_armor||!item.profession||*item.profession!=primary||
            item.definition.quantity!=1||item.definition.type!=types[item.slot]||pieces[item.slot-2])return {};
        // Use exactly the base value published to the native formatter. The
        // reviewed metadata supplies only independently modeled bonuses.
        const auto base=items::base_armor::decode(inventory::presented_definition(item).modifiers);
        if(!base)return {};
        const auto insignia=aspect==ArmorAspect::neutral?
            equipment::attribute_runes::neutral_insignia_armor(item.definition.modifiers,primary):
            equipment::attribute_runes::insignia_armor(item.definition.modifiers,primary,aspect==ArmorAspect::physical);
        if(!insignia)return {};
        pieces[item.slot-2]=*base+(aspect==ArmorAspect::physical?item.reviewed_armor->physical:
            aspect==ArmorAspect::elemental?item.reviewed_armor->elemental:0)+*insignia;
    }
    const auto shield=static_cast<int>(items::shield_armor::starter_bonus(profile)+items::arthur_focus::armor_bonus(profile));
    if(!any)return shield;
    if(!pieces[0])return {};
    for(const auto& value:pieces)if(!value||*value!=*pieces[0])return {};
    return *pieces[0]+shield;
}
inline std::optional<int> fully_modeled_armor(const inventory::Profile& profile,unsigned primary,bool physical){return fully_modeled_armor_aspect(profile,primary,physical?ArmorAspect::physical:ArmorAspect::elemental);}
inline std::optional<int> fully_modeled_neutral_armor(const inventory::Profile& profile,unsigned primary){return fully_modeled_armor_aspect(profile,primary,ArmorAspect::neutral);}
inline std::optional<int> fully_modeled_elemental_armor(const inventory::Profile& profile,unsigned primary){return fully_modeled_armor(profile,primary,false);}
// Physical totals are independent of elemental armor; only explicit reviewed pieces qualify.
inline std::optional<int> fully_modeled_physical_armor(const inventory::Profile& profile,unsigned primary){return fully_modeled_armor(profile,primary,true);}
inline std::optional<int> fully_modeled_earth_armor(const inventory::Profile& profile,unsigned primary){return fully_modeled_elemental_armor(profile,primary);}
inline ArmorBonus owned_basic_armor(const inventory::Profile& owned,unsigned primary){
    valid_primary(primary);ArmorBonus result;
    constexpr std::array<unsigned,7> types{0,0,7,19,16,4,13};
    for(const auto&[id,item]:owned.items){
        const auto bag=owned.bags.find(item.bag);
        if(bag==owned.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot<2||item.slot>6)continue;
        if(item.definition.quantity!=1||item.definition.type!=types[item.slot]||!item.profession||*item.profession!=primary)continue;
        const auto definition=inventory::presented_definition(item);
        // Native energy properties stand independently of the armor-rating
        // property. No extra metadata fallback after projecting the baseline.
        const auto explicit_bonus=items::base_armor::energy(definition.modifiers);
        if(explicit_bonus){result.capacity+=explicit_bonus->capacity;result.pips+=explicit_bonus->pips;}
    }return result;
}
// Basic_armor.html: Dervish chest grants +25 health. Only explicit private
// reviewed armor qualifies; raw captured modifier arrays remain authoritative
// and are not assigned a bonus solely from profession or visual composition.
inline unsigned owned_basic_health(const inventory::Profile& owned,unsigned primary){
    valid_primary(primary);if(primary!=10)return 0;
    for(const auto&[id,item]:owned.items){
        const auto bag=owned.bags.find(item.bag);
        if(bag==owned.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot!=2)continue;
        if(item.definition.type==7&&item.definition.quantity==1&&item.profession==10&&item.reviewed_armor)
            return 25;
    }
    return 0;
}
// Newly created private-server gear. Capture supplies only visual composition;
// identities are allocated afresh. These are never official captured items.
inline inventory::Profile private_starter_armor(unsigned primary,std::span<const CharacterProfile::EquipmentAppearance> visual,std::u16string owner){
    valid_primary(primary);auto profile=inventory::baseline();
    if(visual.size()!=4&&visual.size()!=5)return profile;
    constexpr std::array<unsigned,5> slots{4,2,6,3,5};
    constexpr std::array<unsigned,7> types{0,0,7,19,16,4,13};
    const auto start=visual.size()==4?1u:0u;
    for(unsigned n=0;n<visual.size();++n){
        const auto slot=slots[start+n];const auto& appearance=visual[n];
        if(!appearance.composite||appearance.composite>=0xedb)continue;
        inventory::Item item;auto& definition=item.definition;
        definition.id=profile.next_item++;definition.file=appearance.composite;
        definition.type=static_cast<std::uint8_t>(types[slot]);definition.unknown1=appearance.parameter;
        definition.dye=appearance.color;definition.interaction=0x20000006;
        definition.encoded_name=u"\x108\x107Private Maximum Basic Armor\x1";
        item.bag=2;item.slot=static_cast<std::uint8_t>(slot);item.profession=static_cast<std::uint8_t>(primary);item.customized=owner;
        item.reviewed_armor=private_maximum_armor(primary);
        profile.items.emplace(definition.id,std::move(item));
    }inventory::validate_character(profile);return profile;
}
// Wiki Basic_armor.html (oldid 2577636): minimum basic ratings and
// campaign-specific warrior starter physical bonus. Visuals remain separate
// from reviewed gameplay stats; this does not fabricate captured item models.
inline inventory::ReviewedArmor creation_basic_armor(unsigned primary,unsigned campaign){
    valid_primary(primary);
    if(campaign<1||campaign>3)throw std::invalid_argument("starter armor campaign");
    constexpr std::array<std::uint16_t,11> minimum{0,25,10,5,5,5,5,10,5,20,10};
    return {minimum[primary],static_cast<std::uint16_t>(primary==1?(campaign==1?20:10):0),
        static_cast<std::uint16_t>(primary==2?30:0)};
}
inline inventory::Profile creation_starter_armor(unsigned primary,unsigned campaign,
    std::span<const CharacterProfile::EquipmentAppearance> visual,std::u16string owner){
    const auto stats=creation_basic_armor(primary,campaign);
    auto profile=private_starter_armor(primary,visual,std::move(owner));
    for(auto&[id,item]:profile.items){
        item.reviewed_armor=stats;
        item.definition.encoded_name=u"\x108\x107Starter Basic Armor\x1";
    }
    return profile;
}
}
