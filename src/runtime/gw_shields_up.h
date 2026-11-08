#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"

namespace fw::gw::skills::effects {
struct ShieldsUpPlan {
 unsigned rank=0,energy=10,armor=60;Time duration=5000,recharge=30000;float range=1012;
 auto operator<=>(const ShieldsUpPlan&)const=default;
};
inline std::optional<ShieldsUpPlan> shields_up(unsigned rank){
 const auto* r=find_skill(367);
 if(rank>20||!r||r->profession!=1||r->attribute!=21||r->type!=15||r->energy_cost()!=10||r->adrenaline||r->health_sacrifice||
    r->activation!=0||r->aftercast!=0||r->recharge!=30||r->scale0!=60||r->scale15!=60||r->bonus0||r->bonus15||
    r->duration0!=5||r->duration15!=11||r->caster_body_resource!=626||r->target_body_resource!=2077)return {};
 return ShieldsUpPlan{rank,10,60,1000ull*interpolate(5,11,rank)};
}
// Membership, life state and distance come from the authoritative completion
// snapshot. Recipients keep the shout after leaving earshot or the party.
inline std::optional<Entry> prepare_shields_up(const ShieldsUpPlan& plan,Scope source,Scope target,unsigned effect,Time now,
 bool party_member,bool alive,float distance){
 const auto canonical=shields_up(plan.rank);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(source)||!valid(target)||source.instance!=target.instance||!effect||
    !alive||(!party_member&&source!=target)||!std::isfinite(distance)||distance<0||distance>plan.range||now>UINT64_MAX-plan.duration)return {};
 return Entry{effect,source.actor,367,plan.rank,Category::other,now,now+plan.duration};
}
enum class ShieldsUpDamage {other,melee_attack,projectile_attack,projectile_spell};
// Reviewed native weapon kinds: bow5, spear35, wand22 and staff26. A spell
// with a traveling visual is not a weapon attack. Guild Wars Wiki Projectile.
inline bool shields_up_projectile_weapon(unsigned item_type){
 return item_type==5||item_type==35||item_type==22||item_type==26;
}
// Add this armor to the existing capped skill-armor calculation before damage
// scaling. Classify the actual damage component, not just its visual projectile.
inline unsigned shields_up_armor(const Registry& registry,Scope target,Time now,ShieldsUpDamage kind){
 if(kind!=ShieldsUpDamage::projectile_attack)return 0;
 for(const auto& e:registry.active(target,now))if(e.skill==367){
  const auto p=shields_up(e.rank);
  if(p&&e.kind==Category::other&&e.cause&&e.cause<=65535&&e.expires&&*e.expires>=e.applied&&*e.expires-e.applied==p->duration)return p->armor;
 }
 return 0;
}
}
