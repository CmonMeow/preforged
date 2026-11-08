#pragma once
#include "gw_panic.h"
namespace fw::gw::skills::effects {
struct PanicApplication {
 Registry registry;Scope caster,target;Entry entry;Applied change;
};
inline bool valid_panic_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
inline std::optional<PanicApplication> prepare_panic_application(const Registry& original,Scope caster,Scope target,
 unsigned effect,const PanicPlan& plan,Time now){
 const auto canonical=panic(plan.rank);
 if(!canonical||plan.skill!=canonical->skill||plan.duration!=canonical->duration||plan.range!=canonical->range||
    plan.apply_radius!=canonical->apply_radius||plan.reaction_radius!=canonical->reaction_radius||
    !valid_panic_scope(caster)||!valid_panic_scope(target)||caster.instance!=target.instance||caster.actor==target.actor||
    !original.matches(target)||!effect||now>UINT64_MAX-plan.duration)return {};
 const Entry entry{effect,caster.actor,52,plan.rank,Category::hex,now,now+plan.duration};
 auto registry=original;const auto change=registry.apply(target,entry,Replace::same_skill);
 if(!change.accepted)return {};
 return PanicApplication{std::move(registry),caster,target,entry,change};
}
// A World-owned credit must match both actor generations and this exact hex
// instance before deriving allegiance or accepting a skill-use reaction.
inline bool current_panic_application(const PanicApplication& application,const Registry& registry,Scope caster,Scope target,Time now){
 if(caster!=application.caster||target!=application.target||!registry.matches(target))return false;
 const auto current=registry.lookup(target,application.entry.instance);const auto& expected=application.entry;
 return current&&current->skill==52&&current->cause==caster.actor&&current->rank==expected.rank&&current->kind==Category::hex&&
     current->applied==expected.applied&&current->expires==expected.expires&&now>=current->applied&&current->expires&&now<*current->expires;
}
}
