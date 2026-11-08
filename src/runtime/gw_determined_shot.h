#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_skillbar.h"
#include "gw_attack_outcomes.h"
#include "gw_cooldown_ordering.h"
#include <set>
namespace fw::gw::combat::weapon {
struct DeterminedShotPlan {unsigned rank=0,bonus=5;auto operator<=>(const DeterminedShotPlan&)const=default;};
inline std::optional<DeterminedShotPlan> determined_shot(unsigned rank){
 const auto* row=skills::find_skill(402);
 if(rank>20||!row||row->type!=14||row->attribute!=25||row->energy_cost()!=5||row->activation!=0.f||row->aftercast!=0.f||row->recharge!=10||row->scale0!=5||row->scale15!=20)return {};
 return DeterminedShotPlan{rank,skills::effects::interpolate(5,20,rank)};
}
struct DeterminedOutcome {AttackScope owner;std::uint64_t event=0,at=0;bool recharge=false;bool operator==(const DeterminedOutcome&)const=default;};
inline bool valid_recharge_scope(AttackScope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
// Receives an already-authorized server ledger result, never a client report.
inline std::optional<DeterminedOutcome> determined_outcome(const DeterminedShotPlan& plan,AttackScope owner,const ResolvedAttack& outcome){
 const auto expected=determined_shot(plan.rank);
 if(!expected||*expected!=plan||!valid_recharge_scope(owner)||outcome.attacker!=owner||!valid_recharge_scope(outcome.target)||
    outcome.target.instance!=owner.instance||outcome.target.actor==owner.actor||!outcome.event||outcome.at<outcome.released||
    !valid_attack_outcome(outcome.outcome)||
    (outcome.failed_hit()&&outcome.before_damage))return {};
 return DeterminedOutcome{owner,outcome.event,outcome.at,outcome.failed_hit()};
}
struct AttackCooldown {skills::Slot slot;std::uint64_t recharge_until=0,disabled_until=0;bool operator==(const AttackCooldown&)const=default;};
struct AttackCooldowns {
 AttackScope owner;std::uint64_t revision=1,last_event=0,at=0;std::vector<AttackCooldown> slots;
 // Release IDs are not impact order: two arrows may resolve in reverse.
 // Retain all reset IDs at the latest impact time, then discard that bounded
 // set when time advances. Earlier immutable outcomes cannot be replayed.
 std::uint64_t last_reset_at=0;std::set<std::uint64_t> reset_events;
 // World knows its disables and bar at impact; local base timers remain
 // session-owned. Unknown local timers must never be reported as observed zero.
 bool local_recharge_known=true;
 bool operator==(const AttackCooldowns&)const=default;
};
struct DeterminedRecharge {AttackCooldowns before,after;DeterminedOutcome trigger;std::vector<skills::Slot> cleared;std::vector<skills::Slot> barriers;bool operator==(const DeterminedRecharge&)const=default;};
// Snapshot must be authoritative at impact, including the current eligible bar
// and World disable timers. Delayed delivery cannot substitute a newer bar.
inline std::optional<DeterminedRecharge> prepare_determined_recharge(const AttackCooldowns& current,const DeterminedOutcome& event){
 if(!valid_recharge_scope(current.owner)||current.owner!=event.owner||!event.recharge||!event.event||
    !current.revision||current.revision==UINT64_MAX||current.at!=event.at||event.at<current.last_reset_at||
    current.slots.size()>8||current.reset_events.size()>4096||
    (event.at==current.last_reset_at&&(current.reset_events.contains(event.event)||current.reset_events.size()==4096)))return {};
 DeterminedRecharge result{current,current,event,{},{}};std::set<std::pair<unsigned,unsigned>> seen;
 for(auto& slot:result.after.slots){const auto* row=skills::find_skill(slot.slot.skill);
  if((!current.local_recharge_known&&slot.recharge_until)||!row||!row->id||!row->type||slot.slot.copy>65535||!seen.emplace(slot.slot.skill,slot.slot.copy).second)return {};
  if(row->type==14)result.barriers.push_back(slot.slot);
  if(row->type==14&&(slot.recharge_until>event.at||slot.disabled_until>event.at)){
   slot.recharge_until=slot.disabled_until=0;result.cleared.push_back(slot.slot);
  }
 }
 ++result.after.revision;result.after.last_event=event.event;
 if(event.at>current.last_reset_at)result.after.reset_events.clear();
 result.after.last_reset_at=event.at;result.after.reset_events.insert(event.event);return result;
}
inline bool commit_determined_recharge(AttackCooldowns& current,const DeterminedRecharge& proposal){
 const auto expected=prepare_determined_recharge(current,proposal.trigger);
 if(!expected||*expected!=proposal)return false;current=proposal.after;return true;
}
// World must mint the stamp for this exact impact transaction. Revalidate the
// complete proposal; the session timeline then preserves any newer base timer.
inline skills::effects::OrderedAttackCooldowns::Result deliver_determined_recharge(
 skills::effects::OrderedAttackCooldowns& target,const DeterminedRecharge& proposal,skills::effects::CooldownStamp stamp){
 using Result=skills::effects::OrderedAttackCooldowns::Result;
 const auto expected=prepare_determined_recharge(proposal.before,proposal.trigger);
 if(!expected||*expected!=proposal||stamp.owner!=proposal.trigger.owner||stamp.at!=proposal.trigger.at)return Result::invalid;
 return target.reset_attacks(stamp,proposal.barriers);
}

}
