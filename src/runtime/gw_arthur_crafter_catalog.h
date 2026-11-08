#pragma once
#include "gw_arthur_focus_catalog.h"
namespace fw::gw::crafter {
// PRIVATE_ARTHUR_WIKI_CATALOG_V1. Four exact same-window weapons and
// nine exact foci. Wiki complete13row membership is independent evidence;
// chosen list order is private. Missing captured132 is NOT reconstructed.
inline std::vector<Recipe> arthur_wiki_catalog(){std::vector<Recipe> out;
{Recipe r;auto& d=r.reward.definition;
// Source353 line460881; preserve exact encoded name.
d.id=347u;
d.file=2147493136u;
d.type=2u;
d.unknown1=6u;
d.dye=0u;
d.materials=489u;
d.unknown2=1u;
d.interaction=571473921u;
d.price=17u;
d.model=12u;
d.quantity=1u;
d.encoded_name={8594u,58318u,39591u,29238u};
d.modifiers={616038912u,604504281u,623903155u,591922432u,2760378885u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=488;r.gold=100;r.maximum_quantity=10;
r.materials={{1,4},{6,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Source353 line460882; preserve exact encoded name.
d.id=348u;
d.file=2147493155u;
d.type=5u;
d.unknown1=4u;
d.dye=0u;
d.materials=497u;
d.unknown2=1u;
d.interaction=572522497u;
d.price=20u;
d.model=461u;
d.quantity=1u;
d.encoded_name={33025u,5483u,59127u,45657u,11859u};
d.modifiers={639107328u,616038656u,2760378887u,2155872512u,2720530688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=496;r.gold=100;r.maximum_quantity=10;
r.materials={{6,5}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Source353 line460892; preserve exact encoded name.
d.id=333u;
d.file=2147523413u;
d.type=15u;
d.unknown1=6u;
d.dye=0u;
d.materials=508u;
d.unknown2=1u;
d.interaction=572522497u;
d.price=14u;
d.model=1685u;
d.quantity=1u;
d.encoded_name={9284u,55952u,59798u,1498u};
d.modifiers={664277763u,616038400u,2812810253u,2155872512u,2720530688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=507;r.gold=100;r.maximum_quantity=10;
r.materials={{1,3},{6,2}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Source353 line460893; preserve exact encoded name.
d.id=354u;
d.file=2147493102u;
d.type=27u;
d.unknown1=3u;
d.dye=0u;
d.materials=596u;
d.unknown2=1u;
d.interaction=570425345u;
d.price=25u;
d.model=2969u;
d.quantity=1u;
d.encoded_name={9985u,61327u,58018u,11208u};
d.modifiers={664278019u,616038912u,2812808714u,2155872512u,2700608768u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=595;r.gold=100;r.maximum_quantity=10;
r.materials={{1,5}};out.push_back(std::move(r));}
for(const auto& row:arthur_focus_candidates())out.push_back(row);return out;}
}
