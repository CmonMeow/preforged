#pragma once
#include "gw_crafter_transaction.h"
namespace fw::gw::crafter {
// PRIVATE_ARTHUR_WIKI_CATALOG_V1, staged only; no NPC/service activation.
// Exact9 same-owner353 candidate definitions; retail132 order/name join absent.
// Arthur wiki SHA1b4c6ec592883020e9cd0b4e70bfe49a41e5a222b3deaa6aa2ef301a0eaa11cf.
// Weaponsmith wiki SHA9d47dcaecc840017c486559ae21e196122f49ef4d215a51122c04c8429a18c68
// says crafted weapons are not automatically customized; profession0 is
// explicit private unrestricted issuance, not a decoded native owner field.
inline const std::vector<Recipe>& arthur_focus_candidates(){static const std::vector<Recipe> rows=[](){std::vector<Recipe> out;
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460883; item331.
d.id=331u;
d.file=2147573170u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=188u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=5u;
d.model=969u;
d.quantity=1u;
d.encoded_name={8954u,59837u,47861u,27886u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=187;r.gold=50;r.maximum_quantity=10;
r.materials={{1,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460884; item349.
d.id=349u;
d.file=2147573173u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=188u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=5u;
d.model=999u;
d.quantity=1u;
d.encoded_name={8975u,50637u,53941u,8386u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=187;r.gold=50;r.maximum_quantity=10;
r.materials={{1,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460885; item332.
d.id=332u;
d.file=2147570926u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=217u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=3u;
d.model=1040u;
d.quantity=1u;
d.encoded_name={8999u,37339u,53360u,11406u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=216;r.gold=50;r.maximum_quantity=10;
r.materials={{0,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460886; item328.
d.id=328u;
d.file=2147524679u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=203u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=3u;
d.model=1060u;
d.quantity=1u;
d.encoded_name={9015u,46849u,63835u,1762u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=202;r.gold=50;r.maximum_quantity=10;
r.materials={{8,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460887; item350.
d.id=350u;
d.file=2147571005u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=217u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=3u;
d.model=1080u;
d.quantity=1u;
d.encoded_name={9026u,38707u,59560u,6713u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=216;r.gold=50;r.maximum_quantity=10;
r.materials={{0,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460888; item351.
d.id=351u;
d.file=2147573419u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=181u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=3u;
d.model=1114u;
d.quantity=1u;
d.encoded_name={9051u,39151u,45345u,29910u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=180;r.gold=50;r.maximum_quantity=10;
r.materials={{9,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460889; item352.
d.id=352u;
d.file=2147573179u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=181u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=3u;
d.model=1163u;
d.quantity=1u;
d.encoded_name={9091u,48080u,45101u,21146u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=180;r.gold=50;r.maximum_quantity=10;
r.materials={{9,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460890; item353.
d.id=353u;
d.file=2147570929u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=203u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=3u;
d.model=1222u;
d.quantity=1u;
d.encoded_name={9135u,37057u,35237u,28388u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=202;r.gold=50;r.maximum_quantity=10;
r.materials={{8,1}};out.push_back(std::move(r));}
{Recipe r;auto& d=r.reward.definition;
// Captured353 source line460891; item329.
d.id=329u;
d.file=2147576788u;
d.type=12u;
d.unknown1=0u;
d.dye=0u;
d.materials=203u;
d.unknown2=1u;
d.interaction=536870913u;
d.price=3u;
d.model=1243u;
d.quantity=1u;
d.encoded_name={9148u,45067u,54321u,1123u};
d.modifiers={1657275136u,2747794688u};
r.reward.profession=0;r.reward.customized=std::u16string{};
r.native_index=202;r.gold=50;r.maximum_quantity=10;
r.materials={{8,1}};out.push_back(std::move(r));}
return out;}();return rows;}
}
