#pragma once
#include "gw_item_modifiers.h"
namespace fw::gw::items::stats {
struct WeaponDamage {unsigned minimum,maximum;std::optional<unsigned> raw_damage_type;};
// Source: GWToolboxpp ItemDrops.cpp at ef3d3670d0d6343471341373680aafd9de911447,
// PendingDrop constructor. This reports explicit captured raw base damage;
// applying requirement/customization/armor/critical math is a separate step.
inline std::optional<WeaponDamage> reviewed_weapon_damage(std::span<const modifiers::Record> records){
 std::optional<modifiers::Record> damage,type;
 // The pre-Searing Starter Sword has captured A4880302 instead of A7A8.
 // Its local wiki independently specifies 2..3. Support this reviewed value
 // only; the captured Starter Hammer adds A4880503, independently reviewed
 // against Starter_Hammer.html's 3..5 range. Other A488 payloads stay unknown.
 for(auto mod:records){if(mod.identifier()==0xa7a8||mod.raw==0xa4880302||mod.raw==0xa4880503){if(damage)return {};damage=mod;}if(mod.identifier()==0x24b8){if(type)return {};type=mod;}}
 if(!damage||!damage->arg2()||damage->arg2()>damage->arg1())return {};
 return WeaponDamage{damage->arg2(),damage->arg1(),type?std::optional<unsigned>(type->arg1()):std::nullopt};
}
// Native7c6530/879a10 independently resolve2798 attribute ten-bit field
// and low-byte requirement (see gw_armor_attribute_runes.h). The exact base
// armor/energy identities and origin are not inferred from raw item names.
inline bool unresolved_stat_identifier(unsigned identifier){switch(identifier){
 case 0x27e9:case 0x27ea:return true;default:return false;
}}
}
