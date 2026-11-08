#pragma once
#include "gw_blessed_signet.h"
#include "gw_attack_outcomes.h"
#include "gw_holy_damage.h"
namespace fw::gw::skills::effects {
struct RetributionPlan {
 unsigned rank=0,percent=33,maximum=5,energy=10,upkeep=1;
 Time activation=2000,aftercast=750,recharge=0;
 auto operator<=>(const RetributionPlan&)const=default;
};
inline std::optional<RetributionPlan> retribution(unsigned rank){
 const auto* row=find_skill(248);
 if(rank>20||!row||row->campaign||row->profession!=3||row->attribute!=14||row->type!=6||row->special||
    row->energy_cost()!=10||row->health_sacrifice||row->adrenaline||row->activation!=2.f||row->aftercast!=.75f||row->recharge||
    row->target!=3||row->scale0!=33||row->scale15!=33||row->bonus0!=5||row->bonus15!=20||
    row->duration0!=131072||row->duration15!=131072||row->caster_body_resource!=2077||row->target_body_resource!=454)return {};
 return RetributionPlan{rank,33,5+rank};
}
struct RetributionLease {
 Scope caster,target;Entry entry;std::uint64_t application=0;
};
struct RetributionApplication {
 Registry registry;MaintainedEnchantments ownership;RetributionLease lease;std::vector<Entry> removed;
};
// Copy-only transaction. World must settle resources, validate allied actors
// and allocate a shared unique effect ID before committing both authorities.
inline std::optional<RetributionApplication> prepare_retribution(const RetributionPlan& plan,
        Scope caster,Scope target,unsigned effect,Time now,const Registry& registry,const MaintainedEnchantments& ownership){
 const auto canonical=retribution(plan.rank);if(!canonical||*canonical!=plan||!registry.matches(target))return {};
 // Local Enchantment_spell notes: a weaker recast retains the previous higher
 // values until the original expiry. Retribution is indefinite, so there is
 // no timed fallback to the weaker rank. Ownership still belongs to this cast.
 unsigned effective_rank=plan.rank;
 for(const auto& old:registry.active(target,now))if(old.skill==248){
  if(!retribution(old.rank))return {};effective_rank=std::max(effective_rank,old.rank);
 }
 Entry entry{effect,caster.actor,248,effective_rank,Category::enchantment,now,{}};
 RetributionApplication result{registry,ownership,{caster,target,entry},{}};
 const auto applied=result.registry.apply(target,entry,Replace::same_skill);if(!applied.accepted)return {};
 const auto sequence=result.registry.entry_sequence(target,effect);if(!sequence)return {};
 result.lease.application=*sequence;
 for(const auto& old:applied.removed)if(old.skill!=248||!result.ownership.ended(target,old.instance,now))return {};
 if(!result.ownership.add_retribution(caster,target,effect,result.registry,now))return {};
 result.removed=applied.removed;return result;
}
inline bool retribution_lease_matches(const RetributionLease& lease,const Registry& registry,const MaintainedEnchantments& ownership,Time now){
 if(!lease.application||lease.entry.skill!=248||lease.entry.kind!=Category::enchantment||lease.entry.cause!=lease.caster.actor||lease.entry.expires)return false;
 const auto e=registry.lookup(lease.target,lease.entry.instance);
 return registry.matches(lease.target)&&e&&e->instance==lease.entry.instance&&e->cause==lease.caster.actor&&
  e->skill==248&&e->rank==lease.entry.rank&&e->kind==Category::enchantment&&!e->expires&&
  e->applied==lease.entry.applied&&now>=e->applied&&retribution(e->rank)&&
  ownership.maintainer(lease.target,e->instance,now)==lease.caster&&registry.entry_sequence(lease.target,e->instance)==lease.application;
}
struct RetributionEnd {Registry registry;MaintainedEnchantments ownership;Entry entry;};
inline std::optional<RetributionEnd> prepare_retribution_end(const RetributionLease& lease,Time now,
        const Registry& registry,const MaintainedEnchantments& ownership){
 if(!retribution_lease_matches(lease,registry,ownership,now))return {};
 RetributionEnd result{registry,ownership,lease.entry};
 if(!result.registry.remove(lease.target,lease.entry.instance)||!result.ownership.ended(lease.target,lease.entry.instance,now))return {};
 return result;
}
struct RetributionReaction {
 Scope caster,protected_actor,attacker;unsigned effect=0;
 std::uint64_t attack_event=0;Time at=0;combat::Damage damage;
};
// Supply the ACTUAL settled attack component after damage protection and
// conversion, not its unreduced proposal, Health loss, or an indirect effect.
// Positive damage is required; unlike Shield of Judgment a zero-damage hit
// does not qualify. Overkill uses damage, not clipped Health loss. The latter
// ordering and receiver modifiers remain retail verification gaps. A bounded
// retail11attack/3returned receipt supports truncation above the minimum-one
// rule; it does not establish all modifier ordering or hidden receiver state.
inline std::optional<RetributionReaction> retribution_reaction(const RetributionLease& lease,
        const combat::weapon::ResolvedAttack& attack,const combat::Hit& settled,
        std::optional<bool> hostile_to_caster,const HolyTraits& traits,unsigned caster_level){
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 const auto& entry=lease.entry;const auto plan=retribution(entry.rank);
 if(!plan||!lease.application||!valid(lease.caster)||!valid(lease.target)||!valid(attack.attacker)||
    lease.caster.instance!=lease.target.instance||attack.attacker.instance!=lease.target.instance||
    attack.target!=lease.target||attack.attacker.actor==lease.target.actor||!attack.event||attack.at<attack.released||
    !attack.before_damage||*attack.before_damage!=attack.at||!combat::weapon::valid_attack_delivery(attack.delivery)||
    attack.outcome!=combat::weapon::AttackOutcome::hit||hostile_to_caster!=std::optional(true)||!traits.light_sensitive||
    entry.skill!=248||entry.kind!=Category::enchantment||!entry.instance||entry.cause!=lease.caster.actor||entry.expires||entry.applied>attack.at||
    settled.reason!=combat::Reason::accepted||settled.target!=lease.target.actor||settled.generation!=lease.target.generation||
    !std::isfinite(settled.damage)||settled.damage<=0||settled.damage>16000000||
    settled.applied!=std::min(settled.damage,settled.before.current)||!combat::valid_damage_health(settled))return {};
 const auto base=std::min(double(plan->maximum),std::max(1.,std::floor(settled.damage*plan->percent/100.)));
 combat::Damage damage{combat::Kind::armor_ignoring,base*(*traits.light_sensitive?2.:1.),caster_level,248};
 if(!combat::valid(damage))return {};
 return RetributionReaction{lease.caster,lease.target,attack.attacker,entry.instance,attack.event,attack.at,damage};
}
}
