#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <set>
namespace fw::gw::skills::effects {
struct KeystonePlan {
 unsigned skill=63,rank=0,charges=0,damage=15;Time duration=20000;float radius=166;
 auto operator<=>(const KeystonePlan&)const=default;
};
inline std::optional<KeystonePlan> keystone_signet(unsigned rank){
 const auto* row=find_skill(63);
 if(rank>20||!row||row->type!=7||row->attribute!=0||!row->elite()||row->energy_cost()!=0||row->activation!=1.f||row->recharge!=15||row->duration0!=20||row->duration15!=20||row->bonus0!=0||row->bonus15!=6||row->scale0!=15||row->scale15!=60)return {};
 return KeystonePlan{63,rank,interpolate(0,6,rank),interpolate(15,60,rank)};
}
inline bool valid_keystone_scope(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
struct KeystoneUse {
 Scope caster,target;unsigned skill=0;std::uint64_t event=0;Time at=0;bool success=false,foe=false;
 auto operator<=>(const KeystoneUse&)const=default;
};
inline bool valid_keystone_use(const KeystoneUse& use){
 const auto* row=find_skill(use.skill);
 return valid_keystone_scope(use.caster)&&valid_keystone_scope(use.target)&&use.caster.instance==use.target.instance&&
  (!use.foe||use.caster.actor!=use.target.actor)&&use.event&&row&&row->id&&row->type;
}
struct KeystoneEffect {
 Scope owner;unsigned effect=0,rank=0,remaining=0;std::uint64_t application=0;Time applied=0,expires=0;
 auto operator<=>(const KeystoneEffect&)const=default;
};
struct PreparedKeystoneUse {
 KeystoneUse use;std::optional<KeystoneEffect> before;
 bool triggers=false;unsigned remaining=0,damage=0;float radius=166;
 auto operator<=>(const PreparedKeystoneUse&)const=default;
};
// Authoritative charge ledger only. World/Runtime must atomically reset the
// other signet recharges and publish the initial body before installing this
// ledger. A zero-charge timer here makes no claim about retail icon lifetime.
class KeystoneCharges {
 Scope owner_;std::optional<KeystoneEffect> effect_;
 std::uint64_t application_=0,event_=0;Time clock_=0;
public:
 explicit KeystoneCharges(Scope owner):owner_(owner){}
 bool apply(Scope owner,const KeystonePlan& plan,unsigned effect,std::uint64_t application,Time now){
  const auto canonical=keystone_signet(plan.rank);
  if(owner!=owner_||!valid_keystone_scope(owner)||!canonical||*canonical!=plan||!effect||!application||application<=application_||now<clock_||now>UINT64_MAX-plan.duration)return false;
  effect_=KeystoneEffect{owner,effect,plan.rank,plan.charges,application,now,now+plan.duration};application_=application;clock_=now;return true;
 }
 std::optional<KeystoneEffect> snapshot()const{return effect_;}
 std::optional<PreparedKeystoneUse> prepare(const KeystoneUse& use)const{
  if(!valid_keystone_use(use)||use.caster!=owner_||use.event<=event_||use.at<clock_)return {};
  PreparedKeystoneUse result{use,effect_};const auto* row=find_skill(use.skill);
  if(!use.success||!use.foe||row->type!=7||use.skill==63||!effect_||!effect_->remaining||use.at<effect_->applied||use.at>=effect_->expires)return result;
  const auto plan=keystone_signet(effect_->rank);if(!plan)return {};
  result.triggers=true;result.remaining=effect_->remaining-1;result.damage=plan->damage;return result;
 }
 // World supplies current full lifetimes after the primary signet effect.
 // Reconstruct the proposal so forged damage/charges and renewals cannot pass.
 bool commit(const PreparedKeystoneUse& proposed,Scope caster,Scope target){
  if(caster!=owner_||target!=proposed.use.target)return false;
  const auto current=prepare(proposed.use);if(!current||*current!=proposed)return false;
  event_=proposed.use.event;clock_=proposed.use.at;
  if(proposed.triggers){effect_->remaining=proposed.remaining;if(!effect_->remaining)effect_.reset();}
  return true;
 }
};
struct KeystoneContact {Scope owner;float distance=0;bool alive=true,foe=true,acting=false;};
struct KeystoneRecipient {Scope owner;unsigned damage=0;bool interrupt_attempt=false;};
// Foe is relative to the Keystone owner. Primary target is excluded even if
// killed by the triggering signet. Living idle neighbors still take damage.
inline std::optional<std::vector<KeystoneRecipient>> keystone_recipients(const PreparedKeystoneUse& p,const std::vector<KeystoneContact>& contacts){
 if(!valid_keystone_use(p.use)||!p.triggers||!p.before||!p.use.success||!p.use.foe||find_skill(p.use.skill)->type!=7||p.use.skill==63)return {};
 const auto& e=*p.before;const auto plan=keystone_signet(e.rank);
 if(!plan||e.owner!=p.use.caster||!e.effect||!e.application||!e.remaining||e.remaining>plan->charges||e.applied>p.use.at||e.applied>UINT64_MAX-plan->duration||e.expires!=e.applied+plan->duration||p.use.at>=e.expires||p.remaining!=e.remaining-1||p.damage!=plan->damage||p.radius!=plan->radius)return {};
 std::vector<KeystoneRecipient> result;std::set<unsigned> seen;
 for(const auto& c:contacts){
  if(!valid_keystone_scope(c.owner)||c.owner.instance!=p.use.caster.instance||!seen.insert(c.owner.actor).second||!std::isfinite(c.distance)||c.distance<0)return {};
  if(c.owner.actor==p.use.target.actor){if(c.owner!=p.use.target)return {};continue;}
  if(c.owner.actor==p.use.caster.actor){if(c.owner!=p.use.caster)return {};continue;}
  if(c.alive&&c.foe&&c.distance<=p.radius)result.push_back({c.owner,p.damage,c.acting});
 }
 return result;
}
}
