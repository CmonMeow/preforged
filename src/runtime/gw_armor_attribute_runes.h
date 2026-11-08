#pragma once
#include "gw_inventory.h"
#include "gw_equipment_rules.h"
namespace fw::gw::equipment::attribute_runes {
struct Requirement {unsigned attribute,rank;};
// Native7c6530 and879a10, independently exercised by item_properties_2022.py.
// Preserve the full ten-bit field; valid player attributes are checked only
// when interpreting the rune as worn player equipment.
inline std::optional<Requirement> requirement(std::span<const std::uint32_t> raw){
 std::optional<Requirement> result;
 for(auto word:raw)if((word&0x3ff00000u)==0x27900000u){
  if(result)return {};result=Requirement{(word>>8)&0x3ffu,word&255u};
 }
 return result;
}
struct Decoded {bool valid=true;std::optional<rules::Rune> rune;rules::Insignia insignia=rules::Insignia::none;};
inline Decoded decode(std::span<const std::uint32_t> raw,unsigned primary){
 Decoded result;
 for(auto word:raw){
  std::optional<rules::Rune> passive;
  switch(word){
  case 0x27e802ea:case 0x27e902ea:case 0x27ea02ea:
    if(primary!=1)return {false,{}};
    passive=rules::Rune{rules::RuneKind::absorption,0,((word>>16)&3u)+1};break;  case 0x27e802c2:passive=rules::Rune{rules::RuneKind::vigor,0,1};break;
  case 0x27e902c2:passive=rules::Rune{rules::RuneKind::vigor,0,2};break;
  case 0x27ea02c2:passive=rules::Rune{rules::RuneKind::vigor,0,3};break;
  case 0x25300423:passive=rules::Rune{rules::RuneKind::attunement,0,0};break;
  case 0x25300425:passive=rules::Rune{rules::RuneKind::vitae,0,0};break;
  case 0x253003ca:if(result.insignia!=rules::Insignia::none)return {false,{}};result.insignia=rules::Insignia::radiant;break;
  case 0x253003cc:if(result.insignia!=rules::Insignia::none)return {false,{}};result.insignia=rules::Insignia::survivor;break;
  case 0x253003d8:if(primary!=4||result.insignia!=rules::Insignia::none)return {false,{}};result.insignia=rules::Insignia::tormentor;break;
  case 0xa53003f4:if(primary!=1||result.insignia!=rules::Insignia::none)return {false,{}};result.insignia=rules::Insignia::dreadnought;break;
  case 0xa53003ce:if(primary<1||primary>10||result.insignia!=rules::Insignia::none)return {false,{}};result.insignia=rules::Insignia::stalwart;break;
  }
  if(passive){if(result.rune)return {false,{}};result.rune=passive;}
  if((word&0x3ff00000u)==0x21e00000u){
  const auto attribute=(word>>8)&0x3ffu,tier=word&255u;
  if(result.rune||primary<1||primary>10||attribute>=attributes::catalog.size()
     ||attributes::catalog[attribute].profession!=primary||tier<1||tier>3)return {false,{}};
  result.rune=rules::Rune{rules::RuneKind::attribute,attribute,tier};
 }}
 return result;
}
inline std::optional<int> insignia_armor(std::span<const std::uint32_t> raw,unsigned primary,bool physical){
 const auto decoded=decode(raw,primary);if(!decoded.valid)return {};
 // Dreadnought_Insignia.html: Warrior-only +10 elemental armor on this piece.
 return decoded.insignia==rules::Insignia::tormentor||(!physical&&decoded.insignia==rules::Insignia::dreadnought)||
     (physical&&decoded.insignia==rules::Insignia::stalwart)?10:0;
}
inline std::optional<int> neutral_insignia_armor(std::span<const std::uint32_t> raw,unsigned primary){
 const auto decoded=decode(raw,primary);if(!decoded.valid)return {};
 // Tormentor grants armor against all damage. Stalwart and Dreadnought
 // are physical-only and elemental-only respectively.
 return decoded.insignia==rules::Insignia::tormentor?10:0;
}
// Exact passive identities additionally match pinned Toolbox PriceCheckerModule.
// Unrecognized words remain opaque; base armor benefits are not inferred.
// Equipment placement/profession is authoritative; raw arrays remain intact.
inline std::optional<rules::Bonuses> worn(const inventory::Profile& profile,unsigned primary){
 std::array<std::optional<rules::ArmorPiece>,5> pieces{};
 constexpr std::array<unsigned,7> type{0,0,7,19,16,4,13};
 constexpr std::array<unsigned,7> semantic_slot{0,0,1,3,0,4,2};
 for(const auto&[id,item]:profile.items){
  const auto bag=profile.bags.find(item.bag);
  if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot<2||item.slot>6)continue;
  if(item.definition.type!=type[item.slot]||item.definition.quantity!=1
     ||!item.profession||*item.profession!=primary)return {};
  const auto decoded=decode(item.definition.modifiers,primary);if(!decoded.valid)return {};
  const auto slot=semantic_slot[item.slot];
  pieces[slot]=rules::ArmorPiece{static_cast<rules::Slot>(slot),primary,decoded.rune.value_or(rules::Rune{}),decoded.insignia};
 }
 return rules::armor_bonuses(primary,pieces);
}
}
