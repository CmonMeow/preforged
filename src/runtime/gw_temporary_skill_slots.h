#pragma once
#include "gw_inspired_hex.h"
#include "gw_echo.h"
#include "gw_arcane_mimicry.h"
#include "gw_skillbar.h"
namespace fw::gw::skills::effects {
struct TemporarySkillSlot {
 unsigned index=0;skills::Slot original,replacement;
 Time applied=0,expires=0;std::uint64_t generation=0;
 Time restore_cooldown=0;
 EchoRestorationKind restoration=EchoRestorationKind::observed_cooldown;
 bool require_campaign=false;
};
struct TemporarySkillCastGrant {
 Scope owner;unsigned index=0;skills::Slot slot;
 std::uint64_t generation=0;Time expires=0;
 bool require_campaign=false;
};
struct TemporarySkillRestoration {TemporarySkillSlot grant;bool restored=false;};
// Scope-owned temporary capability, never a persistent Learned/Policy update.
// The caller chooses the replacement copy index from verified protocol policy.
// PvP alias wire IDs remain unresolved and are deliberately not admitted here.
class TemporarySkillSlots {
 Scope owner_;Time clock_=0;std::uint64_t next_generation_=1;
 std::map<unsigned,TemporarySkillSlot> slots_;
 bool owns(Scope owner)const{return owner==owner_&&owner.instance&&owner.actor&&owner.actor<=65535&&owner.generation;}
 std::optional<TemporarySkillSlot> install_leaf(Scope owner,skills::Slots& bar,unsigned index,
    skills::Slot original,skills::Slot replacement,Time now,Time duration,Time restore_cooldown=0,
    EchoRestorationKind restoration=EchoRestorationKind::observed_cooldown,bool require_campaign=false){
  if(!owns(owner)||now<clock_||!duration||now>UINT64_MAX-duration||index>=bar.size()||
     next_generation_==UINT64_MAX||bar[index]!=original||replacement.copy>65535)return {};
  const auto prior=slots_.find(index);
  auto root=original;
  if(prior!=slots_.end()){
   if(prior->second.replacement!=original||now<prior->second.applied||now>=prior->second.expires)return {};
   root=prior->second.original;restore_cooldown=prior->second.restore_cooldown;restoration=prior->second.restoration;
   require_campaign=require_campaign||prior->second.require_campaign;
  }
  for(unsigned other=0;other<bar.size();++other)if(other!=index&&bar[other]==replacement)return {};
  const TemporarySkillSlot grant{index,root,replacement,now,now+duration,next_generation_,restore_cooldown,restoration,require_campaign};
  slots_.insert_or_assign(index,grant);bar[index]=replacement;++next_generation_;clock_=now;return grant;
 }
 std::optional<std::vector<TemporarySkillRestoration>> restore(Scope owner,skills::Slots& bar,Time now,bool all){
  if(!owns(owner)||now<clock_)return {};
  auto proposed=slots_;auto next_bar=bar;std::vector<TemporarySkillRestoration> changes;
  for(auto it=proposed.begin();it!=proposed.end();){
   if(!all&&now<it->second.expires){++it;continue;}
   const bool matches=next_bar[it->first]==it->second.replacement;
   changes.push_back({it->second,matches});if(matches)next_bar[it->first]=it->second.original;
   it=proposed.erase(it);
  }
  slots_=std::move(proposed);bar=next_bar;clock_=now;return changes;
 }
public:
 explicit TemporarySkillSlots(Scope owner):owner_(owner){}
 std::optional<TemporarySkillSlot> install_mimicry(Scope owner,skills::Slots& bar,unsigned index,
    skills::Slot original,skills::Slot replacement,Time now,const MimicryDonorLease& lease){
  const auto plan=arcane_mimicry();
  if(!plan||lease.owner()!=owner||lease.source()!=original||original.skill!=65||lease.at()!=now||
     lease.donor().status!=MimicrySelection::copy||!lease.donor().slot||replacement.skill!=lease.donor().slot->skill||
     !replacement.copy||lease.owner_bar().bar!=bar)return {};
  return install_leaf(owner,bar,index,original,replacement,now,Time(plan->copy_seconds)*1000,
      Time(plan->restoration_disable)*1000,EchoRestorationKind::independent_disable,true);
 }
 std::optional<TemporarySkillSlot> install(Scope owner,skills::Slots& bar,unsigned index,
    skills::Slot original,skills::Slot replacement,Time now){
  const auto plan=inspired_hex(original.skill,0);
  if(!plan||classify_effect_copy(replacement.skill,plan->removed_category).kind!=HexCopyKind::player)return {};
  return install_leaf(owner,bar,index,original,replacement,now,20000);
 }
 std::optional<TemporarySkillSlot> install_echo(Scope owner,skills::Slots& bar,unsigned index,
    skills::Slot original,skills::Slot replacement,Time now,const EchoUseProposal& proposal){
  const auto plan=echo(original.skill);
  if(!plan||proposal.at!=now||proposal.source!=original||!proposal.registry.matches(owner)||proposal.consumed.skill!=original.skill||
     proposal.consumed.cause!=owner.actor||proposal.consumed.kind!=Category::enchantment||
     !proposal.consumed.instance||proposal.registry.lookup(owner,proposal.consumed.instance)||
     proposal.consumed.applied>now||!proposal.consumed.expires||now>=*proposal.consumed.expires||
     proposal.copied_skill!=replacement.skill||proposal.copy_duration!=plan->copy_duration||
     proposal.restore_cooldown!=plan->restore_cooldown||proposal.restoration!=plan->restoration)return {};
  return install_leaf(owner,bar,index,original,replacement,now,plan->copy_duration,plan->restore_cooldown,plan->restoration);
 }
 std::optional<TemporarySkillCastGrant> authorize(Scope owner,const skills::Slots& bar,unsigned index,
    skills::Slot requested,Time now)const{
  if(!owns(owner)||now<clock_||index>=bar.size()||bar[index]!=requested)return {};
  const auto slot=slots_.find(index);
  if(slot==slots_.end()||slot->second.replacement!=requested||now<slot->second.applied||now>=slot->second.expires)return {};
  return TemporarySkillCastGrant{owner,index,requested,slot->second.generation,slot->second.expires,slot->second.require_campaign};
 }
 bool valid(const TemporarySkillCastGrant& grant,const skills::Slots& bar,Time now)const{
  const auto current=authorize(grant.owner,bar,grant.index,grant.slot,now);
  return current&&current->generation==grant.generation&&current->expires==grant.expires&&current->require_campaign==grant.require_campaign;
 }
 std::optional<std::vector<TemporarySkillRestoration>> expire(Scope owner,skills::Slots& bar,Time now){return restore(owner,bar,now,false);}
 std::optional<std::vector<TemporarySkillRestoration>> retire(Scope owner,skills::Slots& bar,Time now){
  const auto changes=restore(owner,bar,now,true);if(changes)owner_={};return changes;
 }
 // Standard eligibility is evaluated against the original learned abilities;
 // only exact, current, live copied slots get this substitution. Callers still
 // require a cast grant for each copied request, checked again on completion.
 std::optional<skills::Slots> eligibility_bar(Scope owner,const skills::Slots& current,Time now)const{
  if(!owns(owner)||now<clock_)return {};auto ordinary=current;
  for(const auto& [index,slot]:slots_)if(current[index]==slot.replacement){
   if(now<slot.applied||now>=slot.expires)return {};ordinary[index]=slot.original;
  }
  return ordinary;
 }
};
}
