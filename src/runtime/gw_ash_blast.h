#pragma once
#include "gw_heal_effects.h"
#include "gw_effect_registry.h"
#include "gw_conditions.h"
#include "gw_combat.h"
#include "gw_attack_miss.h"
namespace fw::gw::skills::effects {
struct AshBlastPlan {
 unsigned rank=0,damage=35,miss_percent=20,energy=5;Time duration=5000,activation=1000,aftercast=750,recharge=15000;
 float range=1248.f,radius=166.f;auto operator<=>(const AshBlastPlan&)const=default;
};
inline std::optional<AshBlastPlan> ash_blast(unsigned rank){
 const auto* r=find_skill(1085);
 if(rank>20||!r||r->type!=4||r->profession!=6||r->attribute!=9||r->special||r->energy_cost()!=5||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=15||r->scale0!=35||r->scale15!=65||r->bonus0!=20||r->bonus15!=75||r->duration0!=5||r->duration15!=5||r->caster_body_resource!=1185||r->target_body_resource!=1186)return {};
 return AshBlastPlan{rank,heal_scale(35,65,rank),heal_scale(20,75,rank)};
}
inline bool ash_scope(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
struct AshBlastHex {Registry registry;Entry hex;std::vector<Entry> removed;};
// Damage owns survival and Burning is sampled at impact. This followup edits
// only the current registry; it must never reapply damage or restore Health.
inline std::optional<AshBlastHex> prepare_ash_blast_hex(const AshBlastPlan& p,
 Scope caster,Scope target,unsigned effect,Time now,const Registry& registry,
 bool alive,bool burning_at_impact){
 const auto canonical=ash_blast(p.rank);
 if(!canonical||*canonical!=p||!ash_scope(caster)||!ash_scope(target)||
    caster.instance!=target.instance||caster.actor==target.actor||!registry.matches(target)||
    !effect||!alive||!burning_at_impact||now>UINT64_MAX-p.duration||registry.lookup(target,effect))return {};
 const Entry hex{effect,caster.actor,1085,p.rank,Category::hex,now,now+p.duration};
 AshBlastHex result{registry,hex,{}};
 const auto applied=result.registry.apply(target,hex,Replace::same_skill);
 if(!applied.accepted)return {};
 result.removed=applied.removed;return result;
}
struct AshBlastTarget {combat::Actor actor;combat::Hit hit;Registry registry;std::optional<Entry> hex;std::vector<Entry> removed;};
// One copied target proposal,not an AoE World transaction. World must select
// authoritative foes around the original target,settle clocks and preflight
// EVERY recipient before committing any proposal. Burning is sampled at impact.
inline std::optional<AshBlastTarget> prepare_ash_blast_target(const AshBlastPlan& p,Scope caster,Scope target,unsigned effect,Time now,
 const Registry& registry,const combat::conditions::State& conditions,combat::Actor actor,unsigned caster_level,bool hostile,float distance_from_center){
 const auto canonical=ash_blast(p.rank);
 if(!canonical||*canonical!=p||!ash_scope(caster)||!ash_scope(target)||caster.instance!=target.instance||caster.actor==target.actor||!registry.matches(target)||actor.id!=target.actor||actor.generation!=target.generation||!effect||!std::isfinite(distance_from_center)||distance_from_center<0||distance_from_center>p.radius)return {};
 AshBlastTarget result{actor,{},registry,{}, {}};
 result.hit=combat::apply(result.actor,{combat::Kind::earth,double(p.damage),caster_level,1085},hostile);
 if(result.hit.reason!=combat::Reason::accepted)return {};
 if(!result.hit.killed&&conditions.has(combat::conditions::Kind::burning,now)){
  const auto followup=prepare_ash_blast_hex(p,caster,target,effect,now,registry,true,true);if(!followup)return {};
  result.registry=followup->registry;result.hex=followup->hex;result.removed=followup->removed;
 }
 return result;
}
// Burning is required to apply the hex,not to retain it. Cleansing Burning
// later does not remove an already-applied five-second accuracy penalty.
inline std::optional<double> ash_blast_miss_chance(const Registry& registry,Scope owner,Time now){
 if(!ash_scope(owner)||!registry.matches(owner))return {};
 const auto e=registry.newest_skill(owner,1085,now);if(!e)return 0.;
 const auto p=ash_blast(e->rank);
 if(!p||e->kind!=Category::hex||!e->instance||!e->cause||e->cause==owner.actor||!e->expires||*e->expires-e->applied!=p->duration)return {};
 return p->miss_percent*.01;
}
}
