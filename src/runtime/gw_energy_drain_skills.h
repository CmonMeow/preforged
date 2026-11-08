#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_vitals.h"
#include <compare>
namespace fw::gw::skills::effects {
struct EnergyDrainPlan{unsigned skill=0,rank=0,amount=0,gain_multiplier=0,damage_per_energy=0;float range=1248,radius=0;bool drain_each=false;std::uint64_t weakness_ms=0;unsigned heal_per_energy=0;auto operator<=>(const EnergyDrainPlan&)const=default;};
inline std::optional<EnergyDrainPlan> energy_drain(unsigned skill,unsigned rank=0){
 if(rank>20)return {};EnergyDrainPlan p;p.skill=skill;p.rank=rank;
 switch(skill){
 case 40:p.amount=3;p.heal_per_energy=interpolate(20,65,rank);break;
 case 39:p.amount=interpolate(1,10,rank);p.damage_per_energy=9;p.radius=252;break;
 case 42:p.amount=interpolate(1,10,rank);p.damage_per_energy=9;break;
 case 59:p.amount=interpolate(3,8,rank);p.radius=252;p.drain_each=true;p.weakness_ms=std::uint64_t(interpolate(1,12,rank))*1000;break;
 case 79:p.amount=interpolate(2,9,rank);p.gain_multiplier=3;break;
 case 80:p.amount=interpolate(4,7,rank);p.gain_multiplier=2;break;
 default:return {};
 }return p;
}
inline bool valid_energy_drain(const EnergyDrainPlan& p){const auto expected=energy_drain(p.skill,p.rank);return expected&&*expected==p;}
struct EnergyDrainResult{unsigned lost=0,gained=0,damage=0,target_energy=0,healed=0;};
inline std::optional<EnergyDrainResult> drain_energy(const EnergyDrainPlan& plan,const vitals::Resources& target){
 if(!valid_energy_drain(plan)||!target.max_energy||target.energy>target.max_energy)return {};
 const auto lost=std::min(plan.amount,target.energy);return EnergyDrainResult{lost,lost*plan.gain_multiplier,lost*plan.damage_per_energy,target.energy-lost,lost*plan.heal_per_energy};
}
}
