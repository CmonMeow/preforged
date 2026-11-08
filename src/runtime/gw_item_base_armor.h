#pragma once
#include "gw_inventory.h"
#include <array>
namespace fw::gw::items::base_armor {
// Exact captured base armor marker, original879a10 emits token944+10bit amount.
// Conditional armor modifiers are not folded into this base value.
inline std::optional<unsigned> decode(std::span<const std::uint32_t> words){
 std::optional<unsigned> value;
 for(auto word:words)if((word&0xfffc00ffu)==0xa3c80000u){
  if(value)return {};const auto amount=(word>>8)&0x3ffu;
  if(!amount||amount>160)return {};value=amount;
 }return value;
}
struct Energy {unsigned capacity=0,pips=0;};
inline std::optional<Energy> energy(std::span<const std::uint32_t> words){
 Energy result;bool capacity=false,pips=false;
 for(auto word:words){
  if((word&0xfffc00ffu)==0x62c80000u){if(capacity)return {};capacity=true;result.capacity=(word>>8)&0x3ffu;}
  if((word&0xfffc00ffu)==0x62e80000u){if(pips)return {};pips=true;result.pips=(word>>8)&0x3ffu;}
 }
 if(result.capacity>50||result.pips>4)return {};return result;
}
using Locations=std::array<std::optional<unsigned>,5>;
inline Locations owned(const inventory::Profile& profile,unsigned primary){
 Locations result{};constexpr std::array<unsigned,7> type{0,0,7,19,16,4,13};
 for(const auto&[id,item]:profile.items){
  const auto bag=profile.bags.find(item.bag);
  if(bag==profile.bags.end()||bag->second.type!=2||bag->second.index!=21||item.slot<2||item.slot>6)continue;
  if(item.definition.type!=type[item.slot]||item.definition.quantity!=1||!item.profession||*item.profession!=primary)continue;
  result[item.slot-2]=decode(item.definition.modifiers);
 }return result;
}
}
