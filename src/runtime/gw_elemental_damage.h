#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_reversal_fortune.h"
#include "gw_conditions.h"
#include "gw_effect_registry.h"
#include "gw_earthquake.h"
#include "gw_liquid_flame.h"
#include "gw_essence_strike.h"
#include <compare>
namespace fw::gw::skills::effects {
struct ElementalOutcome {bool instant_recharge=false;};
struct ElementalPlan{
 unsigned skill=0,rank=0,storage=0;combat::Kind kind=combat::Kind::fire;double damage=0,shadow_above_half=0;
 float range=1248,radius=0;bool caster_center=false,projectile=false;std::uint64_t burning_ms=0,blind_if_burning_ms=0;unsigned energy_per_hit=0,energy_if_burning=0;
 bool burn_or_damage=false;unsigned energy_if_water_hex=0;std::uint64_t self_bleeding_ms=0;
 bool half_if_enchanted=false;
 double bonus_if_burning=0;bool interrupt_if_burning=false;
 bool area_if_target_active=false;unsigned energy_if_spirit=0;
 auto operator<=>(const ElementalPlan&)const=default;
};
inline std::optional<ElementalPlan> elemental(unsigned skill,unsigned rank=0,unsigned storage=0){
 if(rank>20||storage>20)return {};ElementalPlan p;p.skill=skill;p.rank=rank;p.storage=storage;
 switch(skill){
 case 919:{const auto* r=find_skill(skill);
  if(!r||r->campaign!=2||r->profession!=8||r->attribute!=34||r->type!=5||r->special||
     r->energy_cost()!=5||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=6||
     r->scale0!=5||r->scale15!=50||r->bonus0!=1||r->bonus15!=5||
     r->caster_body_resource!=2077||r->target_body_resource!=1104)return {};
  p.kind=combat::Kind::lightning;p.damage=damage_scale(5,50,rank);
  p.burning_ms=1000ull*interpolate(1,5,rank);break;}
 case 1227:{const auto q=essence_strike(rank);if(!q)return {};
  p.kind=combat::Kind::lightning;p.damage=q->lightning_damage;
  p.energy_if_spirit=q->spirit_energy;break;}
 case 170:case 1086:{const auto q=earthquake(skill,rank);if(!q)return {};p.kind=combat::Kind::earth;p.damage=q->damage;p.range=q->range;p.radius=q->radius;break;}
 case 1068:{const auto* row=find_skill(skill);
  if(!row||row->type!=5||row->profession!=4||row->attribute!=5||row->energy_cost()!=5||
     row->activation!=1.f||row->aftercast!=.75f||row->recharge!=10||row->scale0!=15||row->scale15!=60||
     row->caster_body_resource!=2077||row->target_body_resource!=986)return {};
  p.kind=combat::Kind::cold;p.damage=damage_scale(15,60,rank);break;
 }
 case 866:{const auto* row=find_skill(skill);
  if(!row||row->type!=5||row->profession!=6||row->attribute!=11||row->energy_cost()!=10||
     row->activation!=1.5f||row->aftercast!=.75f||row->recharge!=7||!(row->special&1)||row->overcast!=5||
     row->scale0!=15||row->scale15!=135||row->caster_body_resource!=999||row->target_body_resource!=998)return {};
  p.kind=combat::Kind::cold;p.damage=damage_scale(15,135,rank);p.half_if_enchanted=true;break;
 }
 case 145:case 3059:{
  const auto* row=find_skill(skill);
  if(!row||row->type!=7||row->profession!=4||row->attribute!=4||row->energy_cost()!=0||row->health_sacrifice!=10||
     row->activation!=.75f||row->scale0!=10||row->scale15!=70||row->caster_body_resource!=259||row->target_body_resource!=2077||
     row->recharge!=(skill==145?8u:20u))return {};
  p.kind=combat::Kind::armor_ignoring;p.damage=damage_scale(10,70,rank);p.caster_center=true;p.radius=252;p.self_bleeding_ms=25000;break;
 }
 case 163:p.kind=combat::Kind::cold;p.damage=damage_scale(15,75,rank);p.caster_center=true;p.radius=166;break;
 case 188:{const auto* row=find_skill(skill);
  if(!row||row->type!=5||row->profession!=6||row->attribute!=10||row->special||
     row->energy_cost()!=15||row->health_sacrifice||row->adrenaline||row->activation!=.75f||
     row->aftercast!=.75f||row->recharge!=5||row->scale0!=15||row->scale15!=120||
     row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
  p.damage=damage_scale(15,120,rank);p.caster_center=true;p.radius=252;break;
 }
 case 1081:{const auto* row=find_skill(skill);
  if(!row||row->type!=5||row->profession!=6||row->attribute!=8||row->energy_cost()!=5||row->health_sacrifice||
     row->activation!=1.f||row->aftercast!=.75f||row->recharge!=8||row->scale0!=10||row->scale15!=40||
     row->bonus0!=40||row->bonus15!=80||row->caster_body_resource!=287||row->target_body_resource!=2077)return {};
  p.kind=combat::Kind::cold;p.damage=damage_scale(10,40,rank);p.radius=166;
  p.bonus_if_burning=damage_scale(40,80,rank);p.interrupt_if_burning=true;break;
 }
 case 845:{const auto* row=find_skill(skill);
  if(!row||row->type!=5||row->profession!=6||row->attribute!=10||row->energy_cost()!=10||row->health_sacrifice||
     row->activation!=1.f||row->aftercast!=.75f||row->recharge!=15||row->scale0!=7||row->scale15!=112||
     row->caster_body_resource!=2077||row->target_body_resource!=996)return {};
  p.damage=damage_scale(7,112,rank);p.radius=252;p.area_if_target_active=true;break;
 }
 case 194:p.damage=damage_scale(20,65,rank);p.projectile=true;break; // Flare, pinned 2022 scale.
 case 183:p.damage=damage_scale(30,135,rank);p.caster_center=true;p.radius=166;break;
 case 186:p.damage=damage_scale(7,112,rank);p.projectile=true;p.radius=166;break;
 // Pinned2022 row+0x6c is240,despite the generic nearby label/252 constant.
 case 189:{const auto* row=find_skill(skill);if(!row||row->aoe_range!=240)return {};
  p.damage=damage_scale(15,120,rank);p.radius=row->aoe_range;p.burning_ms=std::uint64_t(interpolate(1,3,rank))*1000;break;}
 case 191:p.damage=damage_scale(20,75,rank);p.burning_ms=std::uint64_t(interpolate(1,3,rank))*1000;break;
 case 1379:p.damage=damage_scale(5,50,rank);p.energy_if_burning=5+storage/2;break;
 case 884:p.damage=damage_scale(10,100,rank);p.radius=252;p.burning_ms=std::uint64_t(interpolate(1,7,rank))*1000;p.burn_or_damage=true;break;
 case 2192:p.kind=combat::Kind::cold;p.damage=damage_scale(5,50,rank);p.energy_if_water_hex=5+storage/2;break;
 case 1095:p.damage=damage_scale(7,112,rank);p.range=144;p.radius=322;p.burning_ms=std::uint64_t(interpolate(1,4,rank))*1000;p.energy_per_hit=2;break;
 case 846:p.kind=combat::Kind::cold;p.damage=damage_scale(20,60,rank);p.blind_if_burning_ms=std::uint64_t(interpolate(5,10,rank))*1000;break;
 case 89:p.kind=combat::Kind::cold;p.damage=damage_scale(5,50,rank);p.shadow_above_half=damage_scale(5,50,rank);break;
 default:return {};
 }return p;
}
inline bool valid_elemental(const ElementalPlan& p){const auto expected=elemental(p.skill,p.rank,p.storage);return expected&&*expected==p;}
struct ElementalResult{bool accepted=false;combat::Actor actor;combat::conditions::State conditions;std::vector<combat::Hit> hits;std::vector<combat::conditions::Kind> additions;unsigned energy=0;bool interrupt=false;std::vector<combat::DamageStep> projections;std::vector<std::optional<ReversalDamageReceipt>> reversals;};
inline bool water_hex(const Registry& registry,Scope scope,std::uint64_t now){
 for(const auto& entry:registry.active(scope,now)){const auto* row=find_skill(entry.skill);if(entry.kind==Category::hex&&row&&row->type==4&&row->attribute==11)return true;}return false;
}
inline ElementalResult elemental_target(const ElementalPlan& plan,combat::Actor actor,combat::conditions::State conditions,unsigned level,unsigned caster,std::uint64_t now,bool water_hexed=false,unsigned fire_multiplier=1,bool allow_conditions=true,std::optional<bool> enchanted={},const combat::DamageProjection& projection={},SpiritProximity spirits=SpiritProximity::unknown){
 if(plan.skill==919&&spirits==SpiritProximity::unknown)return {};
 if(plan.skill==170||plan.skill==1086)return {}; // World must own area damage and knockdown together.
 if(!valid_elemental(plan)||fire_multiplier<1||fire_multiplier>4||!caster||caster>65535||!actor.health||!combat::valid(*actor.health)||actor.health->current<=0||!actor.id||actor.id>65535||!actor.damageable||!level||level>255)return {};const bool burning=conditions.has(combat::conditions::Kind::burning,now),above_half=actor.health->current>double(actor.health->maximum)/2;
 if(plan.burn_or_damage&&!burning){ElementalResult result{true,actor,conditions,{},{},0};if(allow_conditions){if(!result.conditions.apply(combat::conditions::Kind::burning,caster,now,plan.burning_ms))return {};result.additions.push_back(combat::conditions::Kind::burning);}return result;}
 if(plan.kind==combat::Kind::fire||plan.kind==combat::Kind::cold||plan.kind==combat::Kind::lightning){
  auto& armor=plan.kind==combat::Kind::fire?actor.fire_armor:plan.kind==combat::Kind::cold?actor.cold_armor:actor.lightning_armor;
  if(!armor)return {};armor=conditions.armor(*armor,now);
 }
 if(plan.half_if_enchanted&&!enchanted)return {};
 const double conditional=plan.half_if_enchanted&&*enchanted?.5:1.;
 // Teinai's Wind combines its Burning bonus into one armor/protection hit.
 // It is not a second packet or a second flat-reduction opportunity.
 const auto step=combat::project_damage(actor,{plan.kind,(plan.damage+(burning?plan.bonus_if_burning:0))*conditional*(plan.kind==combat::Kind::fire?fire_multiplier:1),level,plan.skill},projection);if(!step)return {};
 ElementalResult result{true,actor,conditions,{step->incoming},{},plan.energy_per_hit+(burning?plan.energy_if_burning:0)+(water_hexed?plan.energy_if_water_hex:0)};
 if(projection)result.projections.push_back(*step);
 result.interrupt=burning&&plan.interrupt_if_burning;
 if(result.actor.health->current>0&&above_half&&plan.shadow_above_half){auto shadow=combat::project_damage(result.actor,{combat::Kind::armor_ignoring,plan.shadow_above_half,level,plan.skill},projection);if(!shadow)return {};result.hits.push_back(shadow->incoming);if(projection)result.projections.push_back(*shadow);}
 if(result.actor.health->current<=0)return result;
 for(auto [kind,duration]:std::array<std::pair<combat::conditions::Kind,std::uint64_t>,2>{{{combat::conditions::Kind::burning,(plan.burn_or_damage||(plan.skill==919&&spirits!=SpiritProximity::present))?0:plan.burning_ms},{combat::conditions::Kind::blind,burning?plan.blind_if_burning_ms:0}}})if(duration&&allow_conditions){if(!result.conditions.apply(kind,caster,now,duration))return {};result.additions.push_back(kind);}
 return result;
}
}
