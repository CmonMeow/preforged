#pragma once
#include "gw_skill_catalog_generated.h"
#include <optional>
#include <span>
#include <vector>
namespace fw::gw::items::effect_references {
struct Reference {std::uint32_t raw;unsigned skill,grade,rank;};
struct Result {bool valid=true;std::vector<Reference> references;};
// Original879a10 property27E→878cf0. These are internal skill references,
// not numeric health bonuses. Type/name/effect mechanics require a separate
// reviewed catalog; preserving the reference does not implement its effect.
inline Result decode(std::span<const std::uint32_t> modifiers){
 Result result;if(modifiers.size()>64)return {false,{}};
 for(auto raw:modifiers){
  if((raw&0x3ff00000u)!=0x27e00000u)continue;
  const auto skill=raw&0xffffu,grade=((raw&0x3ffffu)>>16)+1;
  if(!skill||skill>=skills::skill_catalog_2022.size()||grade>3)return {false,{}};
  const auto& record=skills::skill_catalog_2022[skill];
  // Original lookup accepts internal skills; ordinary learned eligibility
  // must never be used as the equipment-effect execution whitelist.
  if(record.id!=skill)return {false,{}};
  result.references.push_back({raw,skill,grade,((grade*15-15)+1)>>1});
 }
 return result;
}
}
