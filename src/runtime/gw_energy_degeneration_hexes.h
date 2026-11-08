#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
struct EnergyDegenerationHexPlan {
 unsigned skill=0,rank=0,target_health_degen=0,caster_health_degen=0,energy_degen=1,zero_damage=0;
 std::uint64_t duration=0;float range=1248;
 auto operator<=>(const EnergyDegenerationHexPlan&)const=default;
};
inline std::optional<EnergyDegenerationHexPlan> energy_degeneration_hex(unsigned skill,unsigned rank){
 if(rank>20||(skill!=125&&skill!=140))return {};
 const auto* row=find_skill(skill);const bool wither=skill==125;
 if(!row||row->type!=4||row->profession!=4||row->attribute!=7||row->special!=(wither?516u:0u)||
    row->health_sacrifice||row->energy_cost()!=(wither?10u:5u)||row->activation!=2.f||row->aftercast!=.75f||
    row->recharge!=(wither?10u:2u)||row->duration0!=5||row->duration15!=35||
    row->scale0!=(wither?2u:1u)||row->scale15!=(wither?4u:1u)||
    row->bonus0!=(wither?15u:5u)||row->bonus15!=(wither?75u:50u)||
    row->caster_body_resource!=2077||row->target_body_resource!=(wither?233u:252u))return {};
 return EnergyDegenerationHexPlan{skill,rank,wither?interpolate(2,4,rank):0u,wither?0u:1u,1,
     interpolate(wither?15:5,wither?75:50,rank),std::uint64_t(interpolate(5,35,rank))*1000,1248};
}
inline bool valid_energy_degeneration_hex(const EnergyDegenerationHexPlan& plan){
 const auto canonical=energy_degeneration_hex(plan.skill,plan.rank);return canonical&&*canonical==plan;
}
}
