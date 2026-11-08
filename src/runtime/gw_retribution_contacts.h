#pragma once
#include "gw_retribution.h"
#include <map>
namespace fw::gw::skills::effects {
// Snapshot the maintained lease at qualification, then resolve only the
// matching terminal attack's settled damage. No callback can replay a contact.
class RetributionContacts {
 struct Pending {Time released;combat::weapon::AttackDelivery delivery;std::optional<RetributionLease> lease;Time qualified=0;};
 Scope owner_;bool retired_=false;
 std::map<std::pair<Scope,std::uint64_t>,Pending> pending_;
 std::map<Scope,std::uint64_t> last_release_;
 static bool valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
public:
 explicit RetributionContacts(Scope owner):owner_(owner){}
 bool release(Scope owner,Scope attacker,std::uint64_t event,Time at,combat::weapon::AttackDelivery delivery){
  if(retired_||owner!=owner_||!valid(owner)||!valid(attacker)||owner.instance!=attacker.instance||owner.actor==attacker.actor||
    !event||!combat::weapon::valid_attack_delivery(delivery)||pending_.size()>=4096)return false;
  const auto previous=last_release_.find(attacker);if(previous!=last_release_.end()&&event<=previous->second)return false;
  if(previous==last_release_.end()&&last_release_.size()>=4096)return false;
  if(!pending_.emplace(std::pair{attacker,event},Pending{at,delivery,{}}).second)return false;
  last_release_[attacker]=event;return true;
 }
 bool prepare(Scope owner,const RetributionLease& lease,const combat::weapon::QualifiedHit& hit,
        const Registry& registry,const MaintainedEnchantments& ownership){
  if(retired_||owner!=owner_||hit.target!=owner||lease.target!=owner||!retribution_lease_matches(lease,registry,ownership,hit.at))return false;
  const auto found=pending_.find({hit.attacker,hit.event});
  if(found==pending_.end()||found->second.lease||found->second.released!=hit.released||found->second.delivery!=hit.delivery||hit.at<hit.released)return false;
  found->second.lease=lease;found->second.qualified=hit.at;return true;
 }
 std::optional<RetributionReaction> consume(Scope owner,const combat::weapon::ResolvedAttack& terminal,
        const combat::Hit& settled,std::optional<bool> hostile,const HolyTraits& traits,unsigned level){
  if(retired_||owner!=owner_||terminal.target!=owner)return {};
  const auto found=pending_.find({terminal.attacker,terminal.event});if(found==pending_.end())return {};
  const auto pending=found->second;pending_.erase(found);
  if(!pending.lease||terminal.released!=pending.released||terminal.at!=pending.qualified||terminal.delivery!=pending.delivery)return {};
  return retribution_reaction(*pending.lease,terminal,settled,hostile,traits,level);
 }
 bool retire_attacker(Scope owner,Scope attacker){
  if(retired_||owner!=owner_)return false;
  std::erase_if(pending_,[&](const auto& item){return item.first.first==attacker;});last_release_.erase(attacker);return true;
 }
 std::optional<RetributionReaction> consume_damage(Scope owner,const combat::weapon::CommittedAttackDamage& damage,
        std::optional<bool> hostile,const HolyTraits& traits,unsigned level){
  return consume(owner,damage.attack(),damage.hit(),hostile,traits,level);
 }
 bool retire(Scope owner){if(retired_||owner!=owner_)return false;retired_=true;pending_.clear();last_release_.clear();return true;}
 std::size_t size()const{return pending_.size();}
};
}
