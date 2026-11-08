#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct ConditionSpellPlan{unsigned skill=0,rank=0;float range=1248,radius=0;std::uint64_t duration=0;bool spread=false;combat::conditions::Kind kind=combat::conditions::Kind::weakness;bool requires_bleeding=false;auto operator<=>(const ConditionSpellPlan&)const=default;};
// Ebon Vanguard rank is a title progression value, never a profession
// attribute. Keep this plan separate until Runtime has the character's
// authoritative title profile at cast admission and completion.
struct SignetInfectionPlan {unsigned skill=2229,ebon_rank=0;std::uint64_t duration=13000;auto operator<=>(const SignetInfectionPlan&)const=default;};
inline std::optional<SignetInfectionPlan> signet_infection(unsigned ebon_rank){
 const auto* r=find_skill(2229);
 if(ebon_rank>10||!r||r->profession||r->attribute!=51||r->type!=7||r->special!=524288||
    r->energy_cost()||r->adrenaline||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=10||
    r->scale0!=13||r->scale15!=20||r->caster_body_resource!=1945||r->target_body_resource!=2077)return {};
 return SignetInfectionPlan{2229,ebon_rank,1000ull*interpolate(13,20,std::min(ebon_rank,5u)*3u)};
}
inline bool signet_infection_bleeding_target(const combat::conditions::State& target,std::uint64_t now){
 return target.has(combat::conditions::Kind::bleeding,now);
}
inline std::optional<ConditionSpellPlan> signet_infection_condition(unsigned ebon_rank){
 const auto plan=signet_infection(ebon_rank);if(!plan)return {};
 return ConditionSpellPlan{2229,ebon_rank,1248,0,plan->duration,false,combat::conditions::Kind::disease,true};
}
inline std::optional<ConditionSpellPlan> condition_spell(unsigned skill,unsigned rank=0){
 if(rank>20)return {};if(skill==117||skill==2859)return ConditionSpellPlan{skill,rank,1248,0,std::uint64_t(interpolate(10,30,rank))*1000,false};
 if(skill==118||skill==2885)return ConditionSpellPlan{skill,rank,1248,252,std::uint64_t(interpolate(skill==118?5:1,skill==118?20:16,rank))*1000,false};
 if(skill==131)return ConditionSpellPlan{skill,rank,1248,166,std::uint64_t(interpolate(3,15,rank))*1000,false,combat::conditions::Kind::bleeding};
 if(skill==159)return ConditionSpellPlan{skill,rank,1248,166,std::uint64_t(interpolate(5,20,rank))*1000,false,combat::conditions::Kind::cracked_armor};
 if(skill==78)return ConditionSpellPlan{78,rank,1248,166,0,true};return {};
}
inline bool valid_condition_spell(const ConditionSpellPlan& plan){const auto expected=plan.skill==2229?signet_infection_condition(plan.rank):condition_spell(plan.skill,plan.rank);return expected&&*expected==plan;}
struct SpreadConditions{combat::conditions::State state;std::vector<combat::conditions::Kind> kinds;};
inline std::optional<SpreadConditions> spread_conditions(const combat::conditions::State& source,const combat::conditions::State& recipient,std::uint32_t caster,std::uint64_t now){
 if(!caster)return {};SpreadConditions result{recipient,{}};
 for(unsigned i=0;i<10;++i)if(source.has(static_cast<combat::conditions::Kind>(i),now))result.kinds.push_back(static_cast<combat::conditions::Kind>(i));
 std::sort(result.kinds.begin(),result.kinds.end(),[&](auto a,auto b){return source.lookup(a)->order<source.lookup(b)->order;});
 for(const auto kind:result.kinds)if(!result.state.apply(kind,caster,now,source.lookup(kind)->expires-now))return {};
 return result;
}
}
