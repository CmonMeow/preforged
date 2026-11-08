#pragma once
#include "gw_attributes.h"
#include <array>
#include <optional>
#include <algorithm>
namespace fw::gw::equipment::rules {
enum class Slot:unsigned {head,chest,hands,legs,feet};
enum class RuneKind {none,attribute,vigor,vitae,attunement,absorption};
enum class Insignia {none,survivor,radiant,tormentor,dreadnought,stalwart};
struct Rune {RuneKind kind=RuneKind::none;unsigned attribute=0,tier=0;};
struct ArmorPiece {Slot slot;unsigned profession;Rune rune;Insignia insignia=Insignia::none;};
struct Bonuses {std::array<unsigned,51> attributes{};int health=0,energy=0;unsigned physical_reduction=0;};
// Semantic inputs require proven modifier decoding. This does not turn an
// arbitrary model ID/name into an upgrade or supply missing captured items.
inline std::optional<Bonuses> armor_bonuses(unsigned primary,const std::array<std::optional<ArmorPiece>,5>& pieces){
 if(primary<1||primary>10)return {};Bonuses out;unsigned vigor=0;
 for(unsigned slot=0;slot<5;++slot){if(!pieces[slot])continue;const auto& piece=*pieces[slot];
  if(static_cast<unsigned>(piece.slot)!=slot||piece.profession!=primary)return {};
  const auto& rune=piece.rune;
  switch(rune.kind){
  case RuneKind::none:if(rune.attribute||rune.tier)return {};break;
  case RuneKind::attribute:
   if(rune.attribute>=51||attributes::catalog[rune.attribute].profession!=primary||rune.tier<1||rune.tier>3)return {};
   out.attributes[rune.attribute]=std::max(out.attributes[rune.attribute],rune.tier);
   out.health-=rune.tier==2?35:rune.tier==3?75:0;break;
  case RuneKind::vigor:
   if(rune.attribute||rune.tier<1||rune.tier>3)return {};
   vigor=std::max(vigor,rune.tier==1?30u:rune.tier==2?41u:50u);break;
  case RuneKind::absorption:if(primary!=1||rune.attribute||rune.tier<1||rune.tier>3)return {};out.physical_reduction=std::max(out.physical_reduction,rune.tier);break;
  case RuneKind::vitae:if(rune.attribute||rune.tier)return {};out.health+=10;break;
  case RuneKind::attunement:if(rune.attribute||rune.tier)return {};out.energy+=2;break;
  default:return {};
  }
  switch(piece.insignia){
  case Insignia::none:break;
  case Insignia::survivor:out.health+=piece.slot==Slot::chest?15:piece.slot==Slot::legs?10:5;break;
  case Insignia::radiant:out.energy+=piece.slot==Slot::chest?3:piece.slot==Slot::legs?2:1;break;
  // Armor and holy vulnerability depend on the worn location, and are
  // resolved by the per-piece armor / holy-equipment paths, not Health/Energy.
  case Insignia::tormentor:if(primary!=4)return {};break;
  case Insignia::dreadnought:if(primary!=1)return {};break;
  case Insignia::stalwart:break;
  default:return {};
  }
 }
 out.health+=vigor;return out;
}
enum class Container {bag,belt_pouch,equipment_pack,backpack,storage};
enum class HoldingRune {normal,superior,belt};
inline std::optional<unsigned> upgraded_capacity(Container kind,unsigned current,HoldingRune rune){
 if(kind==Container::bag&&rune==HoldingRune::normal&&current==5)return 10;
 if(kind==Container::bag&&rune==HoldingRune::superior&&(current==5||current==10))return 15;
 if(kind==Container::belt_pouch&&rune==HoldingRune::belt&&current==5)return 10;
 return {};
}
inline bool can_uninstall_container(unsigned contained_items){return contained_items==0;}
}
