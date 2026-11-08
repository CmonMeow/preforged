#pragma once
#include "gw_equipment_wire.h"
#include "gw_inventory.h"
#include "gw_character_profile.h"
#include <array>

namespace fw::gw::equipment {
using Assignments=std::array<std::uint32_t,9>;
// Public armor order is head/chest/hands/legs/feet, regardless of how many
// pieces a summary originally contained. Empty slots stay explicitly empty.
inline std::vector<CharacterProfile::EquipmentAppearance> armor_visuals(const inventory::Profile& profile){
    constexpr std::array<unsigned,5> physical{4,2,6,3,5};
    constexpr std::array<unsigned,5> types{16,7,13,19,4};
    std::vector<CharacterProfile::EquipmentAppearance> result(physical.size());
    for(const auto& [id,item]:profile.items){
        const auto bag=profile.bags.find(item.bag);
        if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21)continue;
        const auto slot=std::find(physical.begin(),physical.end(),item.slot);
        if(slot==physical.end())continue;
        const auto index=static_cast<unsigned>(slot-physical.begin());const auto& definition=item.definition;
        if(definition.type!=types[index]||!definition.file||definition.file>=0xedb)continue;
        result[index]={static_cast<std::uint16_t>(definition.file),definition.dye,definition.unknown1};
    }
    return result;
}
inline Assignments assignments(const inventory::Profile& profile){
    Assignments result{};
    for(const auto& [id,item]:profile.items){
        const auto bag=profile.bags.find(item.bag);
        if(bag!=profile.bags.end()&&bag->second.type==2&&bag->second.index==21&&item.slot<result.size())
            result[item.slot]=id;
    }
    return result;
}
// Only call with the committed wire projection. Durable IDs and proposed
// transactions must never become client-visible item associations.
inline inventory::Batch assignment_changes(std::uint32_t agent,const Assignments& before,
                                          const Assignments& after){
    inventory::Batch packets;
    for(unsigned slot=0;slot<after.size();++slot)
        if(before[slot]!=after[slot])packets.push_back(assignment_2022(agent,slot,after[slot]));
    return packets;
}
inline inventory::Batch restore_assignments(std::uint32_t agent,const Assignments& current){
    inventory::Batch packets;
    for(unsigned slot=0;slot<current.size();++slot)
        if(current[slot])packets.push_back(assignment_2022(agent,slot,current[slot]));
    return packets;
}
}
