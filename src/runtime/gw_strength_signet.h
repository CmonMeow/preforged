#pragma once
#include "gw_heal_effects.h"
#include "gw_attack_outcomes.h"
#include <set>
namespace fw::gw::skills::effects {
struct StrengthSignetPlan {unsigned rank=0,attacks=1,bonus=5;Time activation=1000,aftercast=750,recharge=45000;auto operator<=>(const StrengthSignetPlan&)const=default;};
inline std::optional<StrengthSignetPlan> strength_signet(unsigned rank){
 const auto* r=find_skill(944);
 if(rank>20||!r||r->type!=7||r->profession!=1||r->attribute!=17||r->special||r->energy_cost()||r->health_sacrifice||r->adrenaline||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=45||r->scale0!=1||r->scale15!=16||r->bonus0!=5||r->bonus15!=5||r->caster_body_resource!=1028||r->target_body_resource!=2077)return {};
 return StrengthSignetPlan{rank,1+rank};
}
// Detached landed-hit subset only. Accuracy-failure/cancel/multi-hit policies
// remain intentionally unimplemented;normal944 must stay unbound until these
// consumers and damage ordering are reviewed. Feed server QualifiedHit events
// before damage,not client outcomes or post-damage replays. World works on a
// copy of both ledger and Registry and commits them with actual Health damage.
class StrengthSignetLedger {
 Scope owner_;unsigned effect_=0,last_effect_=0,rank_=0,remaining_=0;Time applied_=0,clock_=0;bool retired_=false;
 std::set<std::uint64_t> seen_;
 bool owns(Scope s)const{return s==owner_&&s.instance&&s.actor&&s.actor<=65535&&s.generation;}
public:
 explicit StrengthSignetLedger(Scope owner):owner_(owner){}
 unsigned remaining()const{return remaining_;}
 bool attach(Scope owner,const Registry& registry,unsigned effect,Time now){
  if(retired_||!owns(owner)||!registry.matches(owner)||now<clock_||effect<=last_effect_)return false;
  const auto e=registry.lookup(owner,effect);
  if(!e||e->skill!=944||e->cause!=owner.actor||e->kind!=Category::other||e->expires||e->applied!=now)return false;
  const auto p=strength_signet(e->rank);if(!p)return false;
  effect_=effect;last_effect_=effect;rank_=e->rank;remaining_=p->attacks;applied_=now;clock_=now;return true;
 }
 std::optional<unsigned> consume_landed_hit(Scope owner,Registry& registry,const combat::weapon::QualifiedHit& hit){
  if(retired_||!owns(owner)||!registry.matches(owner)||!effect_||!remaining_||hit.attacker!=owner||hit.target.instance!=owner.instance||!hit.target.actor||hit.target.actor>65535||!hit.target.generation||hit.target.actor==owner.actor||!hit.event||seen_.contains(hit.event)||seen_.size()>=4096||hit.at<clock_||hit.released<applied_||hit.at<hit.released||hit.delivery==combat::weapon::AttackDelivery::unknown||!combat::weapon::valid_attack_delivery(hit.delivery))return {};
  const auto e=registry.lookup(owner,effect_);const auto p=strength_signet(rank_);
  if(!p||!e||e->skill!=944||e->rank!=rank_||e->cause!=owner.actor||e->kind!=Category::other||e->expires||e->applied!=applied_)return {};
  // Allocate replay receipt before mutating authoritative state. Caller still
  // owns the copy/commit boundary with actual damage and effect-end packets.
  seen_.insert(hit.event);
  if(remaining_==1)registry.remove(owner,effect_);
  --remaining_;clock_=hit.at;return p->bonus;
 }
 bool detach(Scope owner,Time now){if(retired_||!owns(owner)||now<clock_)return false;effect_=0;remaining_=0;clock_=now;return true;}
 void retire(){retired_=true;effect_=0;remaining_=0;seen_.clear();}
};
}
