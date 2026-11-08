#pragma once
#include "gw_creation_equipment.h"
#include "gw_character_profile.h"
namespace fw::gw::creation {
// Retail 7b1c10 scans slots from 9d609c and extracts the appearance only.
// The caller still owns item persistence and gameplay statistics.
inline std::vector<CharacterProfile::EquipmentAppearance> summary_equipment_2022(
    std::span<const StarterItem> catalog){
    std::array<const items::Definition*,9> slots{};
    for(const auto& entry:catalog){
        if(entry.slot>=slots.size()||slots[entry.slot])
            throw std::invalid_argument("duplicate or invalid summary equipment slot");
        slots[entry.slot]=&entry.definition;
    }
    std::vector<CharacterProfile::EquipmentAppearance> out;
    for(auto slot:{4,2,6,3,5}){
        const auto* item=slots[slot];
        if(!item||!item->file||!(item->interaction&4))continue;
        const auto tint=(item->interaction&0x20000000)?(item->unknown1?item->unknown1:43):0;
        out.push_back({static_cast<std::uint16_t>(item->file),item->dye,static_cast<std::uint8_t>(tint)});
    }
    return out;
}
// Appearance-only preview catalog. No model IDs, modifiers or item prices are
// inferred. Do not persist these definitions as finished gameplay inventory.
// File IDs/tints: GWToolboxpp ArmoryWindow_Constants.h, revision
// ef3d3670d0d6343471341373680aafd9de911447 (MIT, notice in third_party).
// Ascalon starter appearance: https://wiki.guildwars.com/wiki/Ascalon_armor
inline std::vector<StarterItem> prophecies_armor_preview(unsigned profession,
                                                       std::span<const std::uint8_t,9> colors){
    if(profession<1||profession>6)throw std::invalid_argument("unsupported preview profession");
    constexpr std::array<std::array<std::uint32_t,4>,6> files{{
        {0x05b,0x05c,0x05e,0x05a},{0x095,0x096,0x097,0x094},
        {0x0e7,0x0e8,0x0e9,0x0e6},{0x140,0x141,0x142,0x13f},
        {0x19b,0x19c,0x19d,0x19a},{0x1f1,0x1f2,0x1f3,0x1f0}
    }};
    constexpr std::array<std::uint8_t,6> tints{19,7,1,20,33,8};
    constexpr std::array<std::uint8_t,4> slots{2,6,3,5},types{7,13,19,4};
    std::vector<StarterItem> out;
    for(unsigned i=0;i<4;++i){
        if(colors[slots[i]]>13)throw std::invalid_argument("invalid preview dye");
        items::Definition item;item.file=files[profession-1][i];item.type=types[i];
        item.unknown1=tints[profession-1];item.dye=colors[slots[i]];
        item.interaction=0x20000006;
        out.push_back({slots[i],std::move(item)});
    }
    return out;
}
// Appearance-only Shing Jea/ Istani starter art; exact file IDs and tints
// from the same pinned GWToolbox Armory constants as Prophecies above.
// https://wiki.guildwars.com/wiki/Shing_Jea_armor
// https://wiki.guildwars.com/wiki/Istani_armor
inline std::vector<StarterItem> campaign_armor_preview(unsigned campaign,unsigned profession,
                                                      std::span<const std::uint8_t,9> colors){
    if(campaign==1)return prophecies_armor_preview(profession,colors);
    struct Art {unsigned campaign,profession;std::uint8_t slot,type;std::uint32_t file;std::uint8_t tint;};
    constexpr Art art[]{
        {2,1,2,7,0x0B2,3},
        {2,1,6,13,0x0B3,3},
        {2,1,3,19,0x0B5,3},
        {2,1,5,4,0x0B1,3},
        {3,1,2,7,0x541,19},
        {3,1,6,13,0x542,19},
        {3,1,3,19,0x543,19},
        {3,1,5,4,0x540,19},
        {2,2,2,7,0x170,22},
        {2,2,6,13,0x1B6,22},
        {2,2,3,19,0x1B7,22},
        {2,2,5,4,0x16F,22},
        {3,2,2,7,0x572,23},
        {3,2,6,13,0x573,23},
        {3,2,3,19,0x574,23},
        {3,2,5,4,0x571,23},
        {2,3,2,7,0x30E,16},
        {2,3,6,13,0x30F,16},
        {2,3,3,19,0x310,16},
        {2,3,5,4,0x30D,16},
        {3,3,2,7,0x5A0,5},
        {3,3,6,13,0x5A1,5},
        {3,3,3,19,0x5A2,5},
        {3,3,5,4,0x59F,5},
        {2,4,2,7,0x331,20},
        {2,4,6,13,0x332,20},
        {2,4,3,19,0x333,20},
        {2,4,5,4,0x330,20},
        {3,4,2,7,0x5CF,6},
        {3,4,6,13,0x5D0,6},
        {3,4,3,19,0x5D1,6},
        {3,4,5,4,0x5CE,6},
        {2,5,2,7,0x355,32},
        {2,5,6,13,0x356,32},
        {2,5,3,19,0x357,32},
        {2,5,5,4,0x354,32},
        {3,5,2,7,0x6CB,9},
        {3,5,6,13,0x6CC,9},
        {3,5,3,19,0x6CD,9},
        {3,5,5,4,0x6CA,9},
        {2,6,2,7,0x378,29},
        {2,6,6,13,0x379,29},
        {2,6,3,19,0x37A,29},
        {2,6,5,4,0x377,29},
        {3,6,2,7,0x611,8},
        {3,6,6,13,0x612,8},
        {3,6,3,19,0x613,8},
        {3,6,5,4,0x610,8},
        {2,7,2,7,0x3B2,11},
        {2,7,6,13,0x3B3,11},
        {2,7,3,19,0x3B4,11},
        {2,7,5,4,0x3B1,11},
        {2,8,2,7,0x405,12},
        {2,8,6,13,0x406,12},
        {2,8,3,19,0x407,12},
        {2,8,5,4,0x404,12},
        {3,9,2,7,0x640,13},
        {3,9,6,13,0x641,13},
        {3,9,3,19,0x642,13},
        {3,9,5,4,0x63F,13},
        {3,10,2,7,0x675,14},
        {3,10,6,13,0x676,14},
        {3,10,3,19,0x677,14},
        {3,10,5,4,0x674,14},
    };
    std::vector<StarterItem> out;
    for(const auto& row:art){
        if(row.campaign!=campaign||row.profession!=profession)continue;
        if(colors[row.slot]>13)throw std::invalid_argument("invalid preview dye");
        items::Definition item;item.file=row.file;item.type=row.type;
        item.unknown1=row.tint;item.dye=colors[row.slot];item.interaction=0x20000006;
        out.push_back({row.slot,std::move(item)});
    }
    if(out.size()!=4)throw std::invalid_argument("unsupported campaign preview profession");
    return out;
}
}
