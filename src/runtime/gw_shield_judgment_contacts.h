#pragma once
#include "gw_shield_judgment.h"
#include <map>
namespace fw::gw::skills::effects {
// Instance-owned contact journal. Observe server release before qualification;
// snapshot an active status at impact,then retire the contact BEFORE callbacks.
// Release IDs are monotonic per attacker,but projectiles may IMPACT out of order.
class ShieldJudgmentContacts {
 struct Pending {Time released;combat::weapon::AttackDelivery delivery;std::optional<ShieldJudgmentReaction> reaction;};
 using Key=std::pair<Scope,std::uint64_t>;
 Scope owner_;bool retired_=false;
 std::map<Key,Pending> pending_;
 std::map<Scope,std::uint64_t> last_release_;
 static bool valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
public:
 explicit ShieldJudgmentContacts(Scope owner):owner_(owner){}
 bool release(Scope owner,Scope attacker,std::uint64_t event,Time at,combat::weapon::AttackDelivery delivery){
  if(retired_||owner!=owner_||!valid(owner_)||!valid(attacker)||owner.instance!=attacker.instance||owner.actor==attacker.actor||
     !event||!combat::weapon::valid_attack_delivery(delivery)||pending_.size()>=4096)return false;
  const auto previous=last_release_.find(attacker);if(previous!=last_release_.end()&&event<=previous->second)return false;
  if(previous==last_release_.end()&&last_release_.size()>=4096)return false;
  if(!pending_.emplace(Key{attacker,event},Pending{at,delivery,{}}).second)return false;
  last_release_[attacker]=event;return true;
 }
 bool prepare(Scope owner,Scope caster,const Entry& enchantment,
        const combat::weapon::QualifiedHit& hit,std::optional<bool> hostile,const HolyTraits& traits,unsigned level){
  if(retired_||owner!=owner_||hit.target!=owner_)return false;
  const auto found=pending_.find({hit.attacker,hit.event});
  if(found==pending_.end()||found->second.reaction||found->second.released!=hit.released||found->second.delivery!=hit.delivery)return false;
  const combat::weapon::ResolvedAttack terminal{hit.attacker,hit.target,hit.event,hit.released,hit.at,
        combat::weapon::AttackOutcome::hit,hit.at,hit.delivery};
  const auto reaction=shield_judgment_reaction(caster,owner_,enchantment,terminal,hostile,traits,level);if(!reaction)return false;
  found->second.reaction=*reaction;return true;
 }
 std::optional<ShieldJudgmentReaction> consume(Scope owner,const combat::weapon::ResolvedAttack& terminal){
  if(retired_||owner!=owner_||terminal.target!=owner_||!combat::weapon::valid_attack_outcome(terminal.outcome))return {};
  const auto found=pending_.find({terminal.attacker,terminal.event});if(found==pending_.end())return {};
  const auto contact=found->second;pending_.erase(found);
  // Current World paths qualify and finish one impact at the same timestamp.
  // Do not apply an old proposal at a newer Health/control clock silently.
  if(!contact.reaction||terminal.outcome!=combat::weapon::AttackOutcome::hit||terminal.released!=contact.released||
     terminal.before_damage!=contact.reaction->at||terminal.at!=contact.reaction->at||terminal.delivery!=contact.delivery)return {};
  return contact.reaction;
 }
 // World may forget a source only after retiring its exact actor lifetime.
 // Ownership validation stays external;never call this for an active source.
 bool retire_attacker(Scope owner,Scope attacker){
  if(retired_||owner!=owner_)return false;
  std::erase_if(pending_,[&](const auto& item){return item.first.first==attacker;});last_release_.erase(attacker);return true;
 }
 bool retire(Scope owner){if(owner!=owner_||retired_)return false;retired_=true;pending_.clear();last_release_.clear();return true;}
 bool references_caster(Scope caster)const{for(const auto& [key,contact]:pending_)if(contact.reaction&&contact.reaction->caster==caster)return true;return false;}
 std::size_t size()const{return pending_.size();}
};
}
