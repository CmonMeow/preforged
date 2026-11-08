#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct EnergyEnchantmentPlan {
 unsigned skill=0,rank=0,pips=0;std::uint64_t duration=0;float range=0;
 auto operator<=>(const EnergyEnchantmentPlan&)const=default;
};
inline std::optional<EnergyEnchantmentPlan> energy_enchantment(unsigned skill,unsigned rank){
 const auto* row=find_skill(skill);
 if(rank>20||!row||row->type!=6||row->profession!=4||row->attribute!=4||row->aftercast!=.75f||(row->special&1))return {};
 if(skill==119){
  if(row->energy_cost()!=1||row->health_sacrifice!=33||row->activation!=.25f||row->recharge||
     row->duration0!=10||row->duration15!=10||row->bonus0!=3||row->bonus15!=6||
     row->caster_body_resource!=2077||row->target_body_resource!=224||(row->special&2))return {};
  return EnergyEnchantmentPlan{skill,rank,interpolate(3,6,rank),10000,1248};
 }
 if(skill==157){
  if(row->energy_cost()!=5||row->health_sacrifice!=17||row->activation!=2.f||row->recharge!=2||
     row->duration0!=8||row->duration15!=14||row->bonus0!=3||row->bonus15!=3||
     row->caster_body_resource!=281||row->target_body_resource!=2077||!(row->special&2))return {};
  return EnergyEnchantmentPlan{skill,rank,3,std::uint64_t(interpolate(8,14,rank))*1000,144};
 }
 return {};
}
}
