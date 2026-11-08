#pragma once
#include "gw_effective_weapon_equipment.h"
namespace fw::gw::items::shield_armor {
// Exact no-requirement Starter Shield, captured353 stock row10557 and
// independently repeated complete52044 merchant lists. Wiki Starter_Shield
// supplies requirement None/AR3. No unknown or requirement-bearing shield is
// inferred from its model, display marker or an arbitrary positive armor word.
inline unsigned starter_bonus(const inventory::Profile& profile){
 const auto active=inventory::effective_weapon_equipment(profile);
 if(!active||!active->offhand)return 0;
 const auto& item=profile.items.at(active->offhand);
 if(item.profession!=0||!item.customized||!item.customized->empty()||item.reviewed_armor)return 0;
 Definition expected;expected.id=item.definition.id;expected.file=15449;expected.type=24;
 expected.unknown1=3;expected.interaction=536875009;expected.price=0;expected.model=2822;expected.quantity=1;
 expected.encoded_name={9814,38279,61274,607};expected.modifiers={0xa3c80300};
 return definition_2022(item.definition)==definition_2022(expected)?3u:0u;
}
}
