#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include <compare>

namespace fw::gw::skills::effects {
struct VirulencePlan {
 unsigned rank=0;std::uint64_t duration=0;float range=1248;
 auto operator<=>(const VirulencePlan&)const=default;
};
inline std::optional<VirulencePlan> virulence(unsigned rank){
 const auto* row=find_skill(107);
 if(rank>20||!row||row->type!=5||row->profession!=4||row->attribute!=5||row->energy_cost()!=5||
    row->activation!=1.f||row->aftercast!=.75f||row->recharge!=15||row->bonus0!=3||row->bonus15!=15||
    row->caster_body_resource!=2077||row->target_body_resource!=206)return {};
 return VirulencePlan{rank,std::uint64_t(interpolate(3,15,rank))*1000};
}
struct VirulenceStep {combat::conditions::Kind kind;combat::conditions::State state;};
struct VirulenceProposal {bool triggered=false;std::vector<VirulenceStep> steps;};
// A proposal is not a batch commit. World must run reactions after each step,
// stop on death, and validate the same target lifetime before applying another.
// Wiki behavior notes: Poison covers Weakness, which covers Disease.
inline std::optional<VirulenceProposal> virulence_conditions(const VirulencePlan& plan,
 const combat::conditions::State& before,unsigned caster,std::uint64_t now){
 const auto expected=virulence(plan.rank);
 if(!expected||*expected!=plan||!caster||caster>65535)return {};
 VirulenceProposal result;if(!before.count(now))return result;
 auto proposed=before;result.triggered=true;result.steps.reserve(3);
 for(const auto kind:{combat::conditions::Kind::disease,combat::conditions::Kind::weakness,combat::conditions::Kind::poison}){
  if(!proposed.apply(kind,caster,now,plan.duration))return {};
  result.steps.push_back({kind,proposed});
 }
 return result;
}
}
