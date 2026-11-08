#pragma once
#include "gw_blessed_signet.h"
#include "gw_recall_disables.h"
namespace fw::gw::skills::effects {
struct RecallApplication {Registry registry;MaintainedEnchantments ownership;RecallAnchor anchor;Entry entry;};
// Copy-only proposal. World must settle Energy and Health through now, reserve
// the effect identity, then commit registry, ownership and anchor together.
inline std::optional<RecallApplication> prepare_recall_application(const RecallShadowPlan& plan,
 Scope caster,Scope target,unsigned effect,Time now,bool caster_alive,bool target_alive,bool ally,double distance,
 const Registry& registry,const MaintainedEnchantments& ownership){
 const auto anchor=prepare_recall_anchor(plan,caster,target,effect,now,caster_alive,target_alive,ally,distance);
 if(!anchor||!registry.matches(caster)||registry.newest_skill(caster,925,now))return {};
 RecallApplication result{registry,ownership,*anchor,{effect,caster.actor,925,0,Category::enchantment,now,{}}};
 const auto applied=result.registry.apply(caster,result.entry,Replace::same_skill);
 if(!applied.accepted||!applied.removed.empty()||!result.ownership.add_recall(*anchor,result.registry,now))return {};
 return result;
}
struct RecallRetirement {Registry registry;MaintainedEnchantments ownership;RecallAnchor anchor;Entry ended;RecallEndAction action;};
// The ally reference is not the registry owner. Never retarget a recycled actor
// ID. A dead/missing ally ends maintenance without step or skill disabling.
inline std::optional<RecallRetirement> prepare_recall_retirement(const RecallShadowPlan& plan,
 const RecallAnchor& anchor,Scope caster,Scope target,bool caster_alive,bool target_alive,Time now,
 const Registry& registry,const MaintainedEnchantments& ownership){
 if(!registry.matches(anchor.caster)||caster!=anchor.caster||
    ownership.maintainer(anchor.caster,anchor.effect,now)!=caster)return {};
 const auto entry=registry.lookup(caster,anchor.effect);
 if(!entry||entry->skill!=925||entry->cause!=caster.actor||entry->rank||entry->expires||
    entry->kind!=Category::enchantment||entry->applied!=anchor.applied)return {};
 const auto action=recall_end_action(plan,anchor,caster,target,caster_alive,target_alive,now);if(!action)return {};
 RecallRetirement result{registry,ownership,anchor,*entry,*action};
 if(!result.registry.remove(caster,anchor.effect)||!result.ownership.ended(caster,anchor.effect,now))return {};
 return result;
}
struct RecallShadowEndProposal {RecallRetirement retirement;RecallDisables disables;};
// Both components are copied before either can be committed. World must also
// prepare a valid navigation receipt and its ordered publication batch.
inline std::optional<RecallShadowEndProposal> prepare_recall_end_transaction(const RecallShadowPlan& plan,
 const RecallAnchor& anchor,Scope caster,Scope target,bool caster_alive,bool target_alive,Time now,
 const Registry& registry,const MaintainedEnchantments& ownership,const RecallDisables& disables,
 const CooldownBarSnapshot& bar,std::uint64_t event){
 if(!disables.disabled_until(caster,{925,0},now))return {};
 auto retirement=prepare_recall_retirement(plan,anchor,caster,target,caster_alive,target_alive,now,registry,ownership);
 if(!retirement)return {};
 if(!retirement->action.step)return RecallShadowEndProposal{std::move(*retirement),disables};
 const auto proposal=disables.propose(caster,bar,event,now);if(!proposal)return {};
 return RecallShadowEndProposal{std::move(*retirement),*proposal};
}
}

