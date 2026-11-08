#pragma once
#include "gw_conditions.h"
#include <span>
namespace fw::gw::items::modifiers {
struct ConditionBonuses {
 std::array<bool,10> extension{},inscription{},rune{};
};
// Original2022 formatter879a10:246/247 use an18-bit skill identifier;
//277 uses low8 and bits8..17 condition indexes;285 uses bits8..17.
//588870 resolves skill.name;58aa90 indexes the same478..486 name table.
// This decodes condition properties only, not arbitrary item stats/ownership.
inline std::optional<ConditionBonuses> condition_bonuses(std::span<const std::uint32_t> words){
 ConditionBonuses out;
 for(auto raw:words){
  const auto property=(raw>>20)&1023u;
  if(property!=0x246&&property!=0x247&&property!=0x277&&property!=0x285)continue;
  if((raw&0xc0000u)!=0x80000u)return {};
  if(property==0x246||property==0x247){
   const auto skill=raw&0x3ffffu;
   const auto found=std::find(combat::conditions::skills.begin(),combat::conditions::skills.end(),skill);
   if(found==combat::conditions::skills.end())return {};
   const auto index=std::size_t(found-combat::conditions::skills.begin());
   auto& flags=property==0x246?out.extension:out.inscription;
   if(flags[index])return {};flags[index]=true;
  }else{
   const auto first=(raw>>8)&1023u,second=raw&255u;
   if(first>=9||(property==0x277&&second>=9)||(property==0x285&&second))return {};
   out.rune[first]=true;if(property==0x277)out.rune[second]=true;
  }
 }
 return out;
}
}
