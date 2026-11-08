#pragma once
#include "gw_inspired_hex.h"
#include "gw_echo.h"
#include "gw_arcane_mimicry.h"
#include "gw_skillbar.h"
#include <map>
namespace fw::gw::skills::effects {
// Original2022 pending references add copy to (skill<<16). Keep copies in
// their own16-bit lane and never reuse one during this actor lifetime.
// This is our server allocation policy, not a claim about ArenaNet allocation.
class TemporarySkillIds {
 Scope owner_;
 std::map<unsigned,unsigned> next_;
 std::optional<skills::Slot> allocate_known(Scope owner,unsigned skill,const skills::Slots& bar){
  if(owner!=owner_||!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||
     !skill||!find_skill(skill)||!find_skill(skill)->type)return {};
  unsigned candidate=1;
  if(const auto it=next_.find(skill);it!=next_.end())candidate=it->second;
  for(const auto& slot:bar)if(slot.skill==skill){
   if(slot.copy>=65535)return {};
   candidate=std::max(candidate,slot.copy+1);
  }
  if(candidate>65535)return {};
  next_[skill]=candidate+1;
  return skills::Slot{skill,candidate};
 }
public:
 explicit TemporarySkillIds(Scope owner):owner_(owner){}
 std::optional<skills::Slot> allocate_mimicry(Scope owner,const MimicryDonorLease& lease,const skills::Slots& bar){
  if(lease.owner()!=owner||lease.owner_bar().bar!=bar||lease.donor().status!=MimicrySelection::copy||!lease.donor().slot)return {};
  return allocate_known(owner,lease.donor().slot->skill,bar);
 }
 std::optional<skills::Slot> allocate(Scope owner,unsigned skill,const skills::Slots& bar,Category kind=Category::hex){
  if(classify_effect_copy(skill,kind).kind!=HexCopyKind::player)return {};
  return allocate_known(owner,skill,bar);
 }
 std::optional<skills::Slot> allocate_echo(Scope owner,const EchoUseProposal& proposal,const skills::Slots& bar){
  const auto plan=echo(proposal.source.skill);
  if(!plan||!proposal.registry.matches(owner)||!proposal.copied_skill||*proposal.copied_skill!=proposal.used.skill||
     proposal.consumed.skill!=proposal.source.skill||proposal.consumed.cause!=owner.actor||
     proposal.consumed.kind!=Category::enchantment||proposal.used.skill==proposal.source.skill||
     (plan->skill==74&&proposal.used.skill==3)||proposal.copy_duration!=plan->copy_duration||
     proposal.restore_cooldown!=plan->restore_cooldown||proposal.restoration!=plan->restoration)return {};
  const auto* used=find_skill(proposal.used.skill);
  if(!used||(plan->spells_only&&!is_spell(*used)))return {};
  return allocate_known(owner,proposal.used.skill,bar);
 }
};
}
