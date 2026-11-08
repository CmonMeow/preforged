#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
namespace fw::gw::skills::effects {
struct MindWrackPlan {
 unsigned skill=49,rank=0,damage_per_energy=5,zero_damage=15;
 Time duration=5000;float range=1248;
 auto operator<=>(const MindWrackPlan&)const=default;
};
inline std::optional<MindWrackPlan> mind_wrack(unsigned rank,bool pvp=false){
 const auto skill=pvp?2734u:49u;const auto* row=find_skill(skill);
 if(rank>20||!row||row->type!=4||row->profession!=5||row->attribute!=2||row->duration0!=5||row->duration15!=40||
    row->scale0!=15||row->scale15!=100||row->bonus0!=5||row->bonus15!=25||row->pvp_skill!=(pvp?49u:2734u))return {};
 return MindWrackPlan{skill,rank,pvp?0u:interpolate(5,25,rank),interpolate(15,100,rank),Time(interpolate(5,40,rank))*1000};
}
struct MindWrackOwnership {Scope target,caster;unsigned effect=0;auto operator<=>(const MindWrackOwnership&)const=default;};
inline bool valid_mind_wrack_owner(const MindWrackOwnership& owner){
 return owner.effect&&owner.target.instance&&owner.target.instance==owner.caster.instance&&owner.target.actor&&owner.caster.actor&&
  owner.target.actor<=65535&&owner.caster.actor<=65535&&owner.target.actor!=owner.caster.actor&&owner.target.generation&&owner.caster.generation;
}
inline std::optional<Entry> owned_mind_wrack(const Registry& registry,const MindWrackOwnership& owner,Time now){
 if(!valid_mind_wrack_owner(owner))return {};
 const auto entry=registry.lookup(owner.target,owner.effect);
 if(!entry||(entry->skill!=49&&entry->skill!=2734)||entry->cause!=owner.caster.actor||entry->kind!=Category::hex||
    entry->applied>now||!entry->expires||now>=*entry->expires||!mind_wrack(entry->rank,entry->skill==2734))return {};
 return entry;
}
struct MindWrackApplication {Registry registry;MindWrackOwnership owner;Entry entry;std::vector<Entry> removed;};
inline std::optional<MindWrackApplication> prepare_mind_wrack(const Registry& original,Scope caster,Scope target,
 unsigned effect,const MindWrackPlan& plan,Time now){
 const MindWrackOwnership owner{target,caster,effect};const auto canonical=mind_wrack(plan.rank,plan.skill==2734);
 if(!valid_mind_wrack_owner(owner)||!canonical||*canonical!=plan||!original.matches(target)||original.lookup(target,effect)||now>UINT64_MAX-plan.duration)return {};
 auto registry=original;std::vector<Entry> removed;
 // PvE/PvP aliases are one hex family, not independently stacking hexes.
 for(const auto& entry:registry.active(target,now))if(entry.skill==49||entry.skill==2734){registry.remove(target,entry.instance);removed.push_back(entry);}
 const Entry entry{effect,caster.actor,plan.skill,plan.rank,Category::hex,now,now+plan.duration};
 auto changed=registry.apply(target,entry,Replace::same_skill);if(!changed.accepted)return {};
 removed.insert(removed.end(),changed.removed.begin(),changed.removed.end());
 return MindWrackApplication{std::move(registry),owner,entry,std::move(removed)};
}
inline bool current_mind_wrack(const MindWrackApplication& application,const Registry& current,Scope caster,Scope target,Time now){
 if(application.owner.caster!=caster||application.owner.target!=target)return false;
 const auto entry=owned_mind_wrack(current,application.owner,now);
 return entry&&entry->skill==application.entry.skill&&entry->rank==application.entry.rank&&entry->applied==application.entry.applied&&entry->expires==application.entry.expires;
}
struct MindWrackEnd {Registry registry;Entry hex;};
// Expiry, removal and caster death/departure do not manufacture a zero-Energy
// transition. They remove this exact owned hex without explosion damage.
inline std::optional<MindWrackEnd> end_mind_wrack(const Registry& original,const MindWrackOwnership& owner,Time now){
 if(!valid_mind_wrack_owner(owner))return {};
 const auto entry=original.lookup(owner.target,owner.effect);
 if(!entry||(entry->skill!=49&&entry->skill!=2734)||entry->cause!=owner.caster.actor||entry->kind!=Category::hex||entry->applied>now)return {};
 auto registry=original;if(!registry.remove(owner.target,owner.effect))return {};
 return MindWrackEnd{std::move(registry),*entry};
}
struct MindWrackEnergy {
 unsigned current=0,maximum=0,debt=0,fractional_units=0; // 300000 units per Energy.
 auto operator<=>(const MindWrackEnergy&)const=default;
};
inline bool valid_mind_wrack_energy(const MindWrackEnergy& e){return e.maximum&&e.maximum<=1000000&&e.current<=e.maximum&&e.debt<=1000000&&!(e.current&&e.debt)&&e.fractional_units<300000&&!(e.current==e.maximum&&e.fractional_units);}
struct MindWrackTransition {Scope target;std::uint64_t event=0;Time at=0;MindWrackEnergy before,after;};
inline bool valid_mind_wrack_transition(const MindWrackTransition& e){
 return e.target.instance&&e.target.actor&&e.target.actor<=65535&&e.target.generation&&e.event&&valid_mind_wrack_energy(e.before)&&valid_mind_wrack_energy(e.after);
}
inline bool mind_wrack_zero_transition(const MindWrackTransition& e){
 // A UI zero caused by equipment Energy debt is not real zero. An actor
 // already at zero has not dropped to zero, and gains cannot trigger this.
 const auto before=(std::int64_t(e.before.current)-e.before.debt)*300000+e.before.fractional_units;
 const auto after=(std::int64_t(e.after.current)-e.after.debt)*300000+e.after.fractional_units;
 return valid_mind_wrack_transition(e)&&before>0&&after==0;
}
struct MindWrackUse {Scope caster,target;unsigned skill=0;std::uint64_t event=0;Time at=0;bool confirmed_target=false;};
inline bool mind_wrack_qualifying_target(const MindWrackOwnership& owner,const MindWrackUse& use){
 const auto* row=find_skill(use.skill);
 return valid_mind_wrack_owner(owner)&&use.event&&use.confirmed_target&&use.caster==owner.caster&&use.target==owner.target&&
  use.skill&&row&&row->profession==5&&row->type&&row->type!=4&&row->type!=8;
}
struct MindWrackReaction {Registry registry;Entry hex;unsigned per_loss_damage=0,zero_damage=0;bool ends=false;};
// Consume a real Energy transition from any authoritative cause (cost, drain,
// effect or equipment). Caller validates the original caster's CURRENT scope
// and alive state and stages damage before committing this registry proposal.
inline std::optional<MindWrackReaction> mind_wrack_energy_reaction(const Registry& original,const MindWrackOwnership& owner,const MindWrackTransition& transition){
 if(!valid_mind_wrack_transition(transition)||transition.target!=owner.target)return {};
 const auto entry=owned_mind_wrack(original,owner,transition.at);if(!entry)return {};
 const auto plan=*mind_wrack(entry->rank,entry->skill==2734);auto registry=original;
 const bool ends=mind_wrack_zero_transition(transition);
 if(ends&&!registry.remove(owner.target,owner.effect))return {};
 return MindWrackReaction{std::move(registry),*entry,0,ends?plan.zero_damage:0,ends};
}
// World supplies the actual one-Energy loss rather than assuming it succeeded.
// This deliberately leaves targeting/activation/effect ordering to integration;
// a client-requested target is not a confirmed authoritative target event.
inline std::optional<MindWrackReaction> mind_wrack_target_reaction(const Registry& original,const MindWrackOwnership& owner,
 const MindWrackUse& use,const MindWrackTransition& loss){
 if(!mind_wrack_qualifying_target(owner,use)||use.event!=loss.event||use.at!=loss.at||loss.before.maximum!=loss.after.maximum||
    loss.before.debt!=loss.after.debt||loss.before.fractional_units!=loss.after.fractional_units||loss.after.current>loss.before.current||loss.before.current-loss.after.current>1)return {};
 auto result=mind_wrack_energy_reaction(original,owner,loss);if(!result)return {};
 const auto plan=*mind_wrack(result->hex.rank,result->hex.skill==2734);
 result->per_loss_damage=(loss.before.current-loss.after.current)*plan.damage_per_energy;return result;
}
// Copy this ledger with the registry/damage proposal and commit together.
// Keep it across same-lifetime renewals so an old event cannot hit a new hex.
class MindWrackEvents {
 Scope owner_;std::uint64_t last_=0;Time clock_=0;
public:
 explicit MindWrackEvents(Scope owner):owner_(owner){}
 bool accept(const MindWrackTransition& e){if(!valid_mind_wrack_transition(e)||e.target!=owner_||e.event<=last_||e.at<clock_)return false;last_=e.event;clock_=e.at;return true;}
};
}
