#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
struct TaintedFleshPlan {
 unsigned rank=0;std::uint64_t duration=0,disease_duration=0;float range=1248;
 auto operator<=>(const TaintedFleshPlan&)const=default;
};
inline std::optional<TaintedFleshPlan> tainted_flesh(unsigned rank){
 const auto* row=find_skill(113);
 if(rank>20||!row||row->type!=6||row->profession!=4||row->attribute!=5||row->special!=4||
    row->energy_cost()!=5||row->health_sacrifice||row->activation!=1.f||row->aftercast!=.75f||row->recharge||
    row->duration0!=20||row->duration15!=44||row->bonus0!=3||row->bonus15!=15||
    row->caster_body_resource!=2077||row->target_body_resource!=214)return {};
 return TaintedFleshPlan{rank,std::uint64_t(interpolate(20,44,rank))*1000,std::uint64_t(interpolate(3,15,rank))*1000};
}
enum class TaintedContact {invalid,no_effect,unknown,infected};
struct TaintedReaction {
 TaintedContact contact=TaintedContact::invalid;combat::conditions::State state;
};
// The caller must supply one resolved melee hit against the enchanted ally.
// Initial Disease does not require matching creature types;that restriction
// belongs to later contagious spread. Zero damage can still be a landed hit.
// World owns event deduplication,range/LOS,live scopes and original-caster credit.
inline TaintedReaction tainted_flesh_hit(const TaintedFleshPlan& plan,
 const combat::conditions::State& before,unsigned caster,std::uint64_t now,
 bool melee,bool landed,std::optional<bool> fleshy,std::optional<bool> immune){
 TaintedReaction result{TaintedContact::invalid,before};const auto expected=tainted_flesh(plan.rank);
 if(!expected||*expected!=plan||!caster||caster>65535)return result;
 if(!melee||!landed||(fleshy&&!*fleshy)||(immune&&*immune)){
  result.contact=TaintedContact::no_effect;return result;
 }
 if(!fleshy||!immune){result.contact=TaintedContact::unknown;return result;}
 if(!result.state.apply(combat::conditions::Kind::disease,caster,now,plan.disease_duration))return result;
 result.contact=TaintedContact::infected;return result;
}
}
