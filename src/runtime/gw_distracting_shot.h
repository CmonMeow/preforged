#pragma once
#include "gw_hit_interrupts.h"
namespace fw::gw::combat::weapon {
struct DistractingShotPlan {
 unsigned rank=0,damage=1;std::uint64_t animation=500,release=250,aftercast=750,extra_disable=20000;
 bool operator==(const DistractingShotPlan&)const=default;
};
inline std::optional<DistractingShotPlan> distracting_shot(unsigned rank){
 const auto* r=skills::find_skill(399);
 if(rank>20||!r||r->type!=14||r->attribute!=23||r->energy_cost()!=5||r->recharge!=10||r->activation!=.5f||r->aftercast!=1.5f||
    r->scale0!=1||r->scale15!=16||r->bonus0!=20||r->bonus15!=20||r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 // Historical wiki:arrow release is halfway through animation;aftercast.75s.
 // Native body duration remains.5s. Raw row aftercast is not this server phase.
 return DistractingShotPlan{rank,1+rank};
}
inline std::optional<bool> distracting_disable_eligible(const conditions::State& conditions,const AttackActionFacts& action,std::uint64_t now){
 const auto classified=classify_interrupt_action(action);const auto earlier=reviewed_hit_interrupt(conditions,action,AttackOutcome::hit,now);
 if(!classified||!earlier)return {};
 return classified->active&&action.skill.has_value()&&!*earlier;
}
}
