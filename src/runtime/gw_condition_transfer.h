#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct ConditionTransferPlan{unsigned skill=0,rank=0,count=0;float range=144;unsigned duration_percent=100;auto operator<=>(const ConditionTransferPlan&)const=default;};
inline std::optional<ConditionTransferPlan> condition_transfer(unsigned skill,unsigned rank=0){
    if(rank>20)return {};
    if(skill==154)return ConditionTransferPlan{154,rank,interpolate(1,3,rank),144};
    if(skill==132)return ConditionTransferPlan{132,rank,10,1248,interpolate(100,200,rank)};return {};
}
inline bool valid_condition_transfer(const ConditionTransferPlan& p){auto expected=condition_transfer(p.skill,p.rank);return expected&&*expected==p;}
inline bool transfer_roll_required(const ConditionTransferPlan& p){return p.skill==132&&p.rank<=4;}
inline std::optional<bool> condition_transfer_failed(const ConditionTransferPlan& p,double roll){
    if(!valid_condition_transfer(p)||!std::isfinite(roll)||roll<0||roll>=1)return {};
    return transfer_roll_required(p)&&roll<.5;
}
struct TransferredConditions{combat::conditions::State source,target;std::vector<combat::conditions::Kind> kinds;};
// Select covered conditions newest first; scale remaining time by the reviewed
// signet percentage (Touch preserves100%). Round only at the millisecond boundary.
// A longer pre-existing recipient timer remains longer under State::apply.
inline std::optional<TransferredConditions> transfer_conditions(const ConditionTransferPlan& plan,const combat::conditions::State& source,const combat::conditions::State& target,std::uint32_t caster,std::uint64_t now){
    if(!valid_condition_transfer(plan)||!caster)return {};
    TransferredConditions result{source,target,{}};
    for(unsigned i=0;i<plan.count;++i){const auto kind=result.source.newest(now);if(!kind)break;
        const auto entry=result.source.lookup(*kind);if(!entry)return {};
        const auto remaining=entry->expires-now;
        if(remaining>(UINT64_MAX-50)/plan.duration_percent)return {};
        const auto duration=(remaining*plan.duration_percent+50)/100;
        if(!duration||!result.target.apply(*kind,caster,now,duration))return {};
        result.source.remove(*kind);result.kinds.push_back(*kind);
    }
    return result;
}
}
