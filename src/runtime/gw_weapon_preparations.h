#pragma once
#include "gw_weapon_intent.h"
#include "gw_conditions.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::combat::weapon {
enum class PreparationContactKind {poison,melandru,kindle,wager,accuracy};
struct PreparationDefinition {unsigned skill=0,rank=0;PreparationContactKind kind{};std::uint64_t duration=0,condition_duration=0;unsigned bonus=0;};
inline std::optional<PreparationDefinition> preparation_contact_definition(unsigned skill,unsigned rank){
 const auto* extra=skills::find_skill(skill);
 if(skill==430||skill==1723){
  const bool wager=skill==430;
  if(rank>20||!extra||extra->profession!=2||extra->type!=19||extra->attribute!=(wager?23u:25u)||extra->special!=(wager?260u:0u)||
     extra->energy_cost()!=5||extra->activation!=2.f||extra->aftercast!=(wager?0.f:.75f)||extra->recharge!=(wager?24u:12u)||
     extra->duration0!=(wager?18u:36u)||extra->duration15!=(wager?18u:36u)||extra->scale0!=(wager?5u:0u)||extra->scale15!=(wager?10u:0u)||
     extra->bonus0!=(wager?10u:0u)||extra->bonus15!=(wager?10u:0u)||extra->caster_body_resource!=(wager?2077u:1554u)||extra->target_body_resource!=2077)return {};
  return PreparationDefinition{skill,rank,wager?PreparationContactKind::wager:PreparationContactKind::accuracy,(wager?18ull:36ull)*1000,0,
    skills::effects::interpolate(extra->scale0,extra->scale15,rank)};
 }
 const auto* r=skills::find_skill(skill);if(rank>20||!r||(skill!=435&&skill!=429&&skill!=433))return {};
 const bool poison=skill==435,melandru=skill==429;
 if(r->profession!=2||r->type!=19||r->attribute!=24||r->special!=(melandru?260u:256u)||
    r->energy_cost()!=(poison?15u:5u)||r->activation!=2.f||r->aftercast||r->recharge!=12||
    r->duration0!=(melandru?18u:24u)||r->duration15!=(melandru?18u:24u)||
    r->scale0!=(poison?0u:melandru?8u:3u)||r->scale15!=(poison?0u:melandru?28u:24u)||
    r->bonus0!=(poison||melandru?3u:0u)||r->bonus15!=(poison?15u:melandru?25u:0u)||
    r->caster_body_resource!=(skill==433?738u:2077u)||r->target_body_resource!=2077)return {};
 return PreparationDefinition{skill,rank,poison?PreparationContactKind::poison:melandru?PreparationContactKind::melandru:PreparationContactKind::kindle,
    (melandru?18ull:24ull)*1000,(poison||melandru)?1000ull*skills::effects::interpolate(r->bonus0,r->bonus15,rank):0,
    skills::effects::interpolate(r->scale0,r->scale15,rank)};
}
struct OwnedPreparationContact {Scope owner;skills::effects::Entry entry;};
inline bool preparation_weapon(const Owned& weapon,PreparationContactKind kind){
 if(weapon.caster_weapon)return false;
 if(kind!=PreparationContactKind::poison)return weapon.item_type==5;
 return weapon.item_type==2||weapon.item_type==5||weapon.item_type==15||weapon.item_type==27||weapon.item_type==32||weapon.item_type==35||weapon.item_type==36;
}
inline std::optional<OwnedPreparationContact> preparation_contact_snapshot(const skills::effects::Registry& registry,Scope owner,const Owned& weapon,std::uint64_t now){
 if(!registry.matches(owner))return {};
 const auto e=registry.newest(owner,skills::effects::Category::preparation,now);if(!e||e->cause!=owner.actor||!e->expires)return {};
 const auto p=preparation_contact_definition(e->skill,e->rank);
 if(!p||*e->expires-e->applied!=p->duration||!preparation_weapon(weapon,p->kind))return {};
 return OwnedPreparationContact{owner,*e};
}
inline std::optional<PreparationDefinition> preparation_contact_plan(const OwnedPreparationContact& snapshot,Scope owner,const Owned& weapon){
 const auto& e=snapshot.entry;const auto p=preparation_contact_definition(e.skill,e.rank);
 if(snapshot.owner!=owner||e.cause!=owner.actor||e.kind!=skills::effects::Category::preparation||!e.expires||
    !p||*e.expires<e.applied||*e.expires-e.applied!=p->duration||!preparation_weapon(weapon,p->kind))return {};
 return p;
}
inline Owned preparation_weapon_damage(Owned weapon,const std::optional<PreparationDefinition>& plan){
 if(plan&&plan->kind==PreparationContactKind::kindle)weapon.damage.raw_damage_type=5;
 return weapon;
}
}
