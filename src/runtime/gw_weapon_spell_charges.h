#pragma once
// Finite-charge transaction authority. World supplies reviewed792/2868 contact
// policy;795 metadata remains unbound while its damage-reduction phase is open.
#include "gw_weapon_spells.h"
#include "gw_attack_outcome_delivery.h"
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>
namespace fw::gw::skills::effects {
inline bool charged_scope_valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
inline bool charged_entry_equal(const Entry&a,const Entry&b){return a.instance==b.instance&&a.cause==b.cause&&a.skill==b.skill&&a.rank==b.rank&&a.kind==b.kind&&a.applied==b.applied&&a.expires==b.expires;}
struct OwnedChargedWeaponSpell {
 Scope source,recipient;Entry entry;ChargedWeaponSpellPlan plan;
 bool active(const Registry&registry,std::uint64_t at)const{
  if(!charged_scope_valid(source)||!charged_scope_valid(recipient)||source.instance!=recipient.instance||!registry.matches(recipient)||
     !valid_charged_weapon_spell(plan)||!entry.instance||entry.cause!=source.actor||entry.skill!=plan.skill||entry.rank!=plan.rank||
     entry.kind!=Category::weapon_spell||!entry.expires||entry.applied>at||at>=*entry.expires||*entry.expires-entry.applied!=plan.duration)return false;
  const auto actual=registry.lookup(recipient,entry.instance);
  return actual&&charged_entry_equal(*actual,entry)&&registry.count(recipient,Category::weapon_spell,at)==1;
 }
};
// Caller decides from reviewed mechanics. In particular, ignoring a miss for795
// is not evidence that792 should ignore that outcome. No default policy exists.
// This choice concerns the charge only; caller-authored Health proposals remain
// independent, so the foundation does not infer payload/consumption coupling.
enum class ChargeConsumption {ignore,consume};
struct ChargedHealthFact {
 double current=0,maximum=0;std::uint64_t at=0,revision=0;
 bool valid()const{return std::isfinite(current)&&std::isfinite(maximum)&&maximum>0&&current>=0&&current<=maximum&&revision;}
 bool operator==(const ChargedHealthFact&)const=default;
};
struct ChargedHealthMutation {Scope recipient;ChargedHealthFact before,after;};
class ChargeProposal;
class WeaponSpellChargeLedger {
 Scope recipient_;std::optional<OwnedChargedWeaponSpell>owned_;unsigned remaining_=0;
 std::uint64_t revision_=0,clock_=0,sealed_through_=0;unsigned last_effect_=0;bool retired_=false;std::map<std::uint64_t,combat::weapon::ResolvedAttack>seen_;
 friend class ChargeProposal;
public:
 explicit WeaponSpellChargeLedger(Scope recipient):recipient_(recipient){}
 void swap(WeaponSpellChargeLedger&other)noexcept{
  static_assert(noexcept(owned_.swap(other.owned_))&&noexcept(seen_.swap(other.seen_)));
  using std::swap;swap(recipient_,other.recipient_);owned_.swap(other.owned_);swap(remaining_,other.remaining_);
  swap(revision_,other.revision_);swap(clock_,other.clock_);swap(sealed_through_,other.sealed_through_);swap(last_effect_,other.last_effect_);swap(retired_,other.retired_);seen_.swap(other.seen_);
 }
 friend void swap(WeaponSpellChargeLedger&a,WeaponSpellChargeLedger&b)noexcept{a.swap(b);}
 bool attach(const OwnedChargedWeaponSpell&owned,const Registry&registry,std::uint64_t at){
  // World's player next_health_effect and NPC next_npc_hex allocate increasing
  // effect instances for this lifetime. Keep one high-water mark, not an
  // indefinitely growing set of every replaced effect.
  if(retired_||owned.recipient!=recipient_||at<clock_||revision_==UINT64_MAX||owned.entry.instance<=last_effect_||!owned.active(registry,at))return false;
  owned_=owned;remaining_=owned.plan.charges;last_effect_=owned.entry.instance;clock_=at;++revision_;return true;
 }
 // Replacing/ending the active spell preserves receipt and effect high-water
 // barriers. This is not actor retirement and cannot resurrect an older effect.
 bool detach(Scope recipient,std::uint64_t at){
  if(retired_||recipient!=recipient_||at<clock_||revision_==UINT64_MAX)return false;
  owned_.reset();remaining_=0;clock_=at;++revision_;return true;
 }
 // Terminal actor-generation retirement, not a temporary death/resurrection.
 bool retire(Scope recipient){if(retired_||recipient!=recipient_||revision_==UINT64_MAX)return false;retired_=true;owned_.reset();remaining_=0;seen_.clear();++revision_;return true;}
 unsigned remaining()const{return remaining_;}
 std::uint64_t revision()const{return revision_;}
 std::size_t receipts()const{return seen_.size();}
 std::uint64_t sealed_through()const{return sealed_through_;}
 bool retire_receipts(const combat::weapon::AttackConsumerFrontier&frontier){
  if(retired_||frontier.owner()!=recipient_||frontier.through()<sealed_through_)return false;
  if(frontier.through()==sealed_through_)return true;
  if(revision_==UINT64_MAX)return false;
  auto seen=seen_;std::erase_if(seen,[&](const auto&e){return e.first<=frontier.through();});
  seen_.swap(seen);sealed_through_=frontier.through();++revision_;return true;
 }
 const std::optional<OwnedChargedWeaponSpell>&owned()const{return owned_;}
};
// Transaction-local adapter snapshot, NOT a second World Health authority.
// A production adapter must take and revalidate these facts under the same lock
// as the actual Registry, charge ledger and all affected actor Health stores.
struct ChargedSpellSnapshot {
 Registry registry;WeaponSpellChargeLedger ledger;std::map<Scope,ChargedHealthFact>health;
 explicit ChargedSpellSnapshot(Scope recipient):registry(recipient),ledger(recipient){}
};
inline bool replace_charged_weapon_spell(ChargedSpellSnapshot&s,const OwnedChargedWeaponSpell&owned,std::uint64_t at){
 const auto health=s.health.find(owned.recipient);
 if(health==s.health.end()||!health->second.valid()||health->second.current<=0||health->second.at>at)return false;
 auto registry=s.registry;auto ledger=s.ledger;
 if(!registry.apply(owned.recipient,owned.entry,Replace::category).accepted||!ledger.attach(owned,registry,at))return false;
 static_assert(std::is_nothrow_swappable_v<Registry> && std::is_nothrow_swappable_v<WeaponSpellChargeLedger>);
 using std::swap;swap(s.registry,registry);swap(s.ledger,ledger);return true;
}
class ChargeProposal {
 WeaponSpellChargeLedger next_;OwnedChargedWeaponSpell owned_;std::uint64_t before_revision_=0,at_=0;
 std::map<Scope,ChargedHealthFact>before_health_,after_health_;bool removes_=false;
 ChargeProposal(const ChargedSpellSnapshot&s):next_(s.ledger),owned_(*s.ledger.owned_),before_revision_(s.ledger.revision_),before_health_(s.health),after_health_(s.health){}
public:
 // Health deltas are fully resolved, caller-authored proposals. This helper
 // supplies no AoE selection, damage, mitigation, life steal or reaction order.
 // The caller retains this exact expected identity with its event anchor. A
 // failed proposal must not be retried by reselecting a replacement effect.
 static std::optional<ChargeProposal> prepare(const ChargedSpellSnapshot&s,const OwnedChargedWeaponSpell&expected,const combat::weapon::ResolvedAttack&event,
     ChargeConsumption decision,std::span<const ChargedHealthMutation>changes){
  const auto&l=s.ledger;
  if(l.retired_||!l.owned_||!l.remaining_||l.revision_==UINT64_MAX||l.seen_.size()>=4096||!event.event||event.event<=l.sealed_through_||l.seen_.contains(event.event)||
     !combat::weapon::valid_attack_outcome(event.outcome)||!combat::weapon::valid_attack_delivery(event.delivery)||
     event.delivery==combat::weapon::AttackDelivery::unknown||(event.before_damage&&(*event.before_damage<event.released||*event.before_damage>event.at))||
     (event.outcome==combat::weapon::AttackOutcome::hit&&!event.before_damage)||
     event.attacker!=l.recipient_||!charged_scope_valid(event.target)||event.target.instance!=event.attacker.instance||event.target.actor==event.attacker.actor||
     event.released>event.at||event.at<l.clock_||!l.owned_->active(s.registry,event.at)||expected.source!=l.owned_->source||expected.recipient!=l.owned_->recipient||
     !charged_entry_equal(expected.entry,l.owned_->entry)||expected.plan!=l.owned_->plan)return {};
  const auto attacker=s.health.find(event.attacker);
  if(attacker==s.health.end()||!attacker->second.valid()||attacker->second.current<=0||attacker->second.at>event.at)return {};
  const auto target=s.health.find(event.target);
  if(target==s.health.end()||!target->second.valid()||target->second.at>event.at)return {};
  if(decision!=ChargeConsumption::ignore&&decision!=ChargeConsumption::consume)return {};
  ChargeProposal p(s);p.at_=event.at;std::set<Scope>changed;
  for(const auto&m:changes){
   const auto current=s.health.find(m.recipient);
   if(!charged_scope_valid(m.recipient)||m.recipient.instance!=event.attacker.instance||!changed.insert(m.recipient).second||current==s.health.end()||
      current->second!=m.before||!m.before.valid()||m.before.current<=0||m.before.at>event.at||m.before.revision==UINT64_MAX||!m.after.valid()||
      m.after.maximum!=m.before.maximum||m.after.at!=event.at||m.after.revision!=m.before.revision+1)return {};
   p.after_health_.at(m.recipient)=m.after;
  }
  if(decision==ChargeConsumption::consume){--p.next_.remaining_;p.removes_=!p.next_.remaining_;if(p.removes_)p.next_.owned_.reset();}
  p.next_.seen_.emplace(event.event,event);p.next_.clock_=event.at;++p.next_.revision_;
  // Validate final-charge Registry removal on a copy before offering a proposal.
  if(p.removes_){auto registry=s.registry;if(!registry.remove(l.recipient_,p.owned_.entry.instance))return {};}
  return p;
 }
 bool commit(ChargedSpellSnapshot&s,std::uint64_t at){
  if(at!=at_||s.ledger.retired_||s.ledger.revision_!=before_revision_||s.ledger.recipient_!=owned_.recipient||
     !s.ledger.owned_||!charged_entry_equal(s.ledger.owned_->entry,owned_.entry)||s.ledger.owned_->source!=owned_.source||
     s.ledger.owned_->plan!=owned_.plan||!owned_.active(s.registry,at)||s.health!=before_health_)return false;
  // Preserve unrelated Registry changes, e.g. a concurrent hex application.
  auto registry=s.registry;if(removes_&&!registry.remove(owned_.recipient,owned_.entry.instance))return false;
  auto ledger=next_;auto health=after_health_; // All potentially allocating work is before mutation.
  static_assert(std::is_nothrow_swappable_v<Registry> && std::is_nothrow_swappable_v<WeaponSpellChargeLedger>);
  static_assert(std::is_nothrow_swappable_v<decltype(health)>);
  using std::swap;swap(s.registry,registry);swap(s.ledger,ledger);swap(s.health,health);return true;
 }
};
}
