#pragma once
#include "gw_skill_effect_plan.h"
#include <compare>
namespace fw::gw::skills::effects {
enum class GainPredicate {foe_below_half_health,caster_energy_below,foe_conditions};
struct ResourceGainPlan {
    unsigned skill=0,rank=0,health=0,energy=0,energy_threshold=0;
    GainPredicate predicate=GainPredicate::foe_below_half_health;
    float range=1248;
    unsigned adrenaline=0;
    auto operator<=>(const ResourceGainPlan&)const=default;
};
// Net Health delta includes any pending lethal Deep Wound debt settled by
// the gain. It can be negative even though the requested action grants Health.
struct ResourceGainOutcome {bool condition_met=false;double health_delta=0;unsigned energy_gained=0;};
// Local primary wiki semantics plus independently pinned2022 numeric fields.
// These gains do not consume or inspect an enemy's Energy resource.
inline std::optional<ResourceGainPlan> resource_gain(unsigned skill,unsigned rank=0){
    if(rank>20)return {};
    if(skill==365){const auto* row=find_skill(skill);
        if(!row||row->type!=15||!row->elite()||row->attribute!=21||row->energy_cost()!=5||
           row->activation!=0||row->aftercast!=0||row->recharge!=15||row->scale0!=10||row->scale15!=68||
           row->bonus0!=3||row->bonus15!=7||row->caster_body_resource!=624||row->target_body_resource!=2077)return {};
        return ResourceGainPlan{365,rank,interpolate(10,68,rank),interpolate(3,7,rank),0,GainPredicate::foe_conditions};
    }
    if(skill==1141){const auto* row=find_skill(skill);
        if(!row||row->type!=15||row->profession!=1||row->attribute!=17||row->elite()||row->energy_cost()!=5||
           row->activation!=0||row->aftercast!=0||row->recharge!=15||row->scale0!=50||row->scale15!=50||
           row->bonus0!=1||row->bonus15!=3||row->caster_body_resource!=1033||row->target_body_resource!=2077)return {};
        return ResourceGainPlan{1141,rank,0,0,0,GainPredicate::foe_below_half_health,1248,interpolate(1,3,rank)};
    }
    if(skill==1365)return ResourceGainPlan{skill,rank,interpolate(10,100,rank),interpolate(1,10,rank),0,GainPredicate::foe_below_half_health};
    if(skill==881)return ResourceGainPlan{skill,rank,0,interpolate(10,20,rank),interpolate(5,10,rank),GainPredicate::caster_energy_below};
    return {};
}
inline bool gain_is_shout(const ResourceGainPlan& p){return p.skill==365||p.skill==1141;}
inline bool gain_targets_foe(const ResourceGainPlan& p){return p.predicate!=GainPredicate::caster_energy_below;}
inline bool valid_resource_gain(const ResourceGainPlan& p){const auto expected=resource_gain(p.skill,p.rank);return expected&&*expected==p;}
}
