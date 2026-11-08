#include "../src/runtime/gw_armor_attribute_runes.h"
#include "../src/runtime/gw_attribute_condition_bonuses.h"
#include "../src/runtime/gw_basic_armor_energy.h"
#include "../src/runtime/gw_player_holy_equipment.h"
#include <iostream>
using namespace fw::gw;
void check(bool b){if(!b)throw std::runtime_error("armor attribute rune regression");}
int main()try{
 using namespace equipment::attribute_runes;
 {
  const std::array<std::uint32_t,1> dread{0xa53003f4};
  check(decode(dread,1).valid&&decode(dread,1).insignia==equipment::rules::Insignia::dreadnought);
  for(unsigned primary=2;primary<=10;++primary)check(!decode(dread,primary).valid);
  for(auto second:{0xa53003f4u,0x253003cau,0x253003ccu}){
   const std::array<std::uint32_t,2> conflict{0xa53003f4,second};check(!decode(conflict,1).valid);
  }
  std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
  auto suit=energy::private_starter_armor(1,visual,u"Warrior");
  for(auto&[id,item]:suit.items)if(item.slot==2)item.definition.modifiers.push_back(0xa53003f4);
  const auto physical=energy::physical_armor_locations(suit,1),elemental=energy::elemental_armor_locations(suit,1),neutral=energy::neutral_armor_locations(suit,1);check(physical&&elemental&&neutral);
  for(unsigned slot=0;slot<5;++slot)check((*physical)[slot]==100&&(*elemental)[slot]==(slot==0?90:80));
  for(unsigned slot=0;slot<5;++slot)check((*neutral)[slot]==80);
  check(energy::fully_modeled_neutral_armor(suit,1)==80);
  check(!energy::fully_modeled_elemental_armor(suit,1)&&energy::fully_modeled_physical_armor(suit,1)==100);
  const auto bonuses=worn(suit,1);check(bonuses&&bonuses->health==0&&bonuses->energy==0&&bonuses->physical_reduction==0);
  check(equipment::reviewed_player_holy_bonus(suit,1)==0);
  for(auto&[id,item]:suit.items)if(item.slot!=2)item.definition.modifiers.push_back(0xa53003f4);
  check(energy::fully_modeled_elemental_armor(suit,1)==90&&energy::fully_modeled_physical_armor(suit,1)==100);
  const auto restored=inventory::decode(inventory::encode(suit));check(energy::elemental_armor_locations(restored,1)==energy::elemental_armor_locations(suit,1));
  check(energy::neutral_armor_locations(restored,1)==energy::neutral_armor_locations(suit,1));
  auto& carried=suit.items.begin()->second;carried.bag=1;carried.slot=0;
  check((*energy::elemental_armor_locations(suit,1))[2]==0); // Head now empty, not a global bonus.
 }
 {
  const std::array<std::uint32_t,1> stalwart{0xa53003ce};
  check(!decode(stalwart,0).valid&&!decode(stalwart,11).valid);
  std::array<CharacterProfile::EquipmentAppearance,5> visual{};for(auto& piece:visual)piece.composite=1;
  for(unsigned primary=1;primary<=10;++primary){
   check(decode(stalwart,primary).valid&&decode(stalwart,primary).insignia==equipment::rules::Insignia::stalwart);
   auto suit=energy::private_starter_armor(primary,visual,u"Stalwart Owner");
   const auto original_physical=*energy::physical_armor_locations(suit,primary),original_elemental=*energy::elemental_armor_locations(suit,primary),original_neutral=*energy::neutral_armor_locations(suit,primary);
   for(auto&[id,item]:suit.items)if(item.slot==2)item.definition.modifiers.push_back(0xa53003ce);
   const auto physical=*energy::physical_armor_locations(suit,primary);check(energy::elemental_armor_locations(suit,primary)==original_elemental);
   check(energy::neutral_armor_locations(suit,primary)==original_neutral);
   for(unsigned slot=0;slot<5;++slot)check(physical[slot]==original_physical[slot]+(slot==0?10:0));
   check(!energy::fully_modeled_physical_armor(suit,primary));
   const auto bonus=worn(suit,primary);check(bonus&&bonus->health==0&&bonus->energy==0&&bonus->physical_reduction==0&&equipment::reviewed_player_holy_bonus(suit,primary)==0);
   for(auto&[id,item]:suit.items)if(item.slot!=2)item.definition.modifiers.push_back(0xa53003ce);
   check(energy::fully_modeled_physical_armor(suit,primary)==original_physical[0]+10&&energy::elemental_armor_locations(suit,primary)==original_elemental);
   check(energy::physical_armor_locations(inventory::decode(inventory::encode(suit)),primary)==energy::physical_armor_locations(suit,primary));
  }
  for(auto second:{0xa53003ceu,0xa53003f4u,0x253003ccu}){const std::array<std::uint32_t,2> conflict{0xa53003ce,second};check(!decode(conflict,1).valid);}
 }
 for(unsigned tier=1;tier<=3;++tier){const std::array<std::uint32_t,1> raw{0x21e80000u|(9u<<8)|tier};const auto rune=decode(raw,6);check(rune.valid&&rune.rune&&rune.rune->attribute==9&&rune.rune->tier==tier);}
 const std::array<std::uint32_t,1> req{0x27980000u|(500u<<8)|13u};check(requirement(req)->attribute==500&&requirement(req)->rank==13);
 const std::array<std::uint32_t,2> duplicate{0x21e80903,0x21e80901};check(!decode(duplicate,6).valid);
 const std::array<std::uint32_t,1> wrong{0x21e80903};check(!decode(wrong,3).valid);
 const std::array<std::uint32_t,1> invalid{0x21e80904};check(!decode(invalid,6).valid);
 auto profile=inventory::baseline();
 for(unsigned slot:{2u,3u}){inventory::Item item;item.definition.id=profile.next_item++;item.definition.file=1;item.definition.type=slot==2?7:19;item.definition.modifiers={0x21e80903,0x123400aa};item.bag=2;item.slot=slot;item.profession=6;item.customized=u"";profile.items.emplace(item.definition.id,item);}
 inventory::validate(profile);const auto saved=inventory::encode(profile);const auto bonus=worn(profile,6);check(bonus&&bonus->attributes[9]==3&&bonus->health==-150&&bonus->energy==0);check(inventory::encode(profile)==saved);
 profile.items.begin()->second.definition.modifiers={0x21e80901};check(worn(profile,6)->health==-75&&worn(profile,6)->attributes[9]==3);
 profile.items.begin()->second.bag=1;profile.items.begin()->second.slot=0;check(worn(profile,6)->health==-75);
 profile.items.rbegin()->second.definition.modifiers.clear();check(worn(profile,6)->health==0&&worn(profile,6)->attributes[9]==0);
 profile.items.begin()->second.bag=2;profile.items.begin()->second.slot=2;
 profile.items.begin()->second.definition.modifiers={0x27e902c2,0x253003cc};
 profile.items.rbegin()->second.definition.modifiers={0x27ea02c2,0x253003ca};
 check(worn(profile,6)->health==65&&worn(profile,6)->energy==2);
 profile.items.begin()->second.definition.modifiers={0x25300425,0x253003cc};
 profile.items.rbegin()->second.definition.modifiers={0x25300423,0x253003ca};
 check(worn(profile,6)->health==25&&worn(profile,6)->energy==4);
 const std::array<std::uint32_t,2> conflict{0x25300425,0x27ea02c2};check(!decode(conflict,6).valid);
 const std::array<std::uint32_t,2> insignia_conflict{0x253003cc,0x253003ca};check(!decode(insignia_conflict,6).valid);
 const std::array<std::uint32_t,1> unknownpassive{0x27e802ea};check(!decode(unknownpassive,6).valid);
 const std::array<std::uint32_t,1> superiorabsorption{0x27ea02ea};check(decode(superiorabsorption,1).valid&&decode(superiorabsorption,1).rune->tier==3);
 for(unsigned tier=1;tier<=3;++tier){const std::array<std::uint32_t,1> raw{0x27e702eau+(tier<<16)};const auto decoded=decode(raw,1);check(decoded.valid&&decoded.rune&&decoded.rune->kind==fw::gw::equipment::rules::RuneKind::absorption&&decoded.rune->tier==tier);}
 const std::array<std::uint32_t,2> absorptionconflict{0x27e802ea,0x27ea02ea};check(!decode(absorptionconflict,1).valid);
 auto warrior=profile;for(auto&[id,item]:warrior.items){item.profession=1;item.definition.modifiers={0x27e802ea};}warrior.items.rbegin()->second.definition.modifiers={0x27ea02ea};
 const auto absorbed=worn(warrior,1);check(absorbed&&absorbed->physical_reduction==3&&absorbed->health==0);
 warrior.items.rbegin()->second.bag=1;warrior.items.rbegin()->second.slot=1;check(worn(warrior,1)->physical_reduction==1); attributes::Ranks base{};base[9]=12;base[12]=1;base[26]=1;attributes::Bonuses gear{};gear[9]=3;gear[8]=3;
 const auto weak=attributes::condition_bonuses(base,gear,true);check(weak[9]==2&&weak[12]==-1&&weak[8]==3&&weak[26]==0);
 check(attributes::condition_bonuses(base,gear,false)==gear&&base[9]==12);
 base[26]=0;const attributes::Policy policy{6,0,200,true,true};
 const auto native=attributes::snapshot_packet_2022(1,base,policy,weak);
 const auto read=[&](unsigned at){return unsigned(native[at])|(unsigned(native[at+1])<<8)|(unsigned(native[at+2])<<16)|(unsigned(native[at+3])<<24);};
 const unsigned n=(native[6]|(native[7]<<8))/3;bool seen9=false;
 for(unsigned i=0;i<n;++i)if(read(8+4*i)==9){check(read(8+4*n+4*i)==12&&read(8+8*n+4*i)==14);seen9=true;}
 check(seen9);
 std::cout<<"PASS native field widths, class/grade bounds, opaque preservation and worn rune stacking\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
