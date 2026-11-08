#pragma once
#include "gw_effect_registry.h"
#include "gw_disable_reset.h"
#include "gw_skill_effect_plan.h"
#include "gw_skillbar.h"
#include "gw_cooldown_ordering.h"
#include "gw_cooldown_bar_authority.h"
namespace fw::gw::skills::effects {
struct FlourishPlan {unsigned rank=0,energy_per_recharge=2;Time activation=1000,aftercast=750,recharge=8000;bool operator==(const FlourishPlan&)const=default;};
inline std::optional<FlourishPlan> flourish(unsigned rank){
 const auto* r=find_skill(389);
 if(rank>20||!r||r->type!=10||!r->elite()||r->attribute!=17||r->energy_cost()!=5||r->adrenaline||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=8||r->scale0!=2||r->scale15!=7||r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 return FlourishPlan{rank,interpolate(2,7,rank)};
}
inline bool flourish_attack_skill(unsigned skill){const auto* r=find_skill(skill);return r&&(r->type==14||r->type==20);}
struct FlourishPermit {CooldownBarSnapshot bar;DisableRevisions disable_revision{};CooldownStamp stamp;bool operator==(const FlourishPermit&)const=default;};
struct FlourishSlot {Slot slot;Time recharge_until=0,disabled_until=0;bool operator==(const FlourishSlot&)const=default;};
struct FlourishCooldowns {Scope owner;std::uint64_t revision=1,last_event=0;Time at=0;bool timers_known=true;std::vector<FlourishSlot> slots;bool operator==(const FlourishCooldowns&)const=default;};
struct FlourishRecharge {FlourishCooldowns before,after;FlourishPlan plan;std::uint64_t event=0;std::vector<Slot> cleared,barriers;unsigned energy=0;bool operator==(const FlourishRecharge&)const=default;};
// Completion-time current-bar snapshot. No Energy or adrenaline is mutated here:
// World/session must commit timer/reset authority and capped Energy together.
inline std::optional<FlourishRecharge> prepare_flourish_recharge(const FlourishCooldowns& current,const FlourishPlan& plan,std::uint64_t event){
 const auto canonical=flourish(plan.rank);
 if(!canonical||*canonical!=plan||!current.owner.instance||!current.owner.actor||current.owner.actor>65535||!current.owner.generation||
    !current.revision||current.revision==UINT64_MAX||!current.timers_known||!event||event<=current.last_event||current.slots.size()>8)return {};
 FlourishRecharge p{current,current,plan,event};std::set<std::pair<unsigned,unsigned>> seen;
 for(auto& entry:p.after.slots){const auto* row=find_skill(entry.slot.skill);
  if(!row||!row->id||!row->type||entry.slot.copy>65535||!seen.emplace(entry.slot.skill,entry.slot.copy).second)return {};
  if(!flourish_attack_skill(entry.slot.skill))continue;
  p.barriers.push_back(entry.slot);
  if(entry.recharge_until<=current.at&&entry.disabled_until<=current.at)continue;
  entry.recharge_until=entry.disabled_until=0;p.cleared.push_back(entry.slot);p.energy+=plan.energy_per_recharge;
 }
 ++p.after.revision;p.after.last_event=event;return p;
}
inline bool commit_flourish_cooldowns(FlourishCooldowns& current,const FlourishRecharge& proposal){
 const auto expected=prepare_flourish_recharge(current,proposal.plan,proposal.event);
 if(!expected||*expected!=proposal)return false;current=proposal.after;return true;
}
// Deliver only a revalidated reset receipt,including ready-slot barriers.
// Energy remains owned by the eventual joint World/session commit.
inline OrderedAttackCooldowns::Result deliver_flourish_recharge(OrderedAttackCooldowns& target,const FlourishRecharge& proposal,CooldownStamp stamp){
 const auto expected=prepare_flourish_recharge(proposal.before,proposal.plan,proposal.event);
 if(!expected||*expected!=proposal||stamp.owner!=proposal.before.owner||stamp.at!=proposal.before.at)return OrderedAttackCooldowns::Result::invalid;
 return target.reset_attack_skills(stamp,proposal.barriers);
}

}
