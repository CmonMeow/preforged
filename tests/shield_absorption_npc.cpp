#include "gw_npc_combat_catalog.h"
#include <stdexcept>
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Absorption NPC damage line "+std::to_string(at.line()));}
int main()try{
 const Lifetime life{1,2,3,146,100};Definition d;d.binding.actor=100;d.binding.map=146;
 d.relation=Relation::hostile_to_players;d.spawn_health=Health{1000,1000};d.damageable=true;d.earth_armor=60;
 {
  LiveActor actor(d,life);check(actor.stage_shield_absorption(life,12,1,77,100));
  const auto plan=*skills::effects::feast_of_corruption(12);
  const auto shadow=actor.apply_damage(life,{Kind::armor_ignoring,double(plan.damage),20,151},100);
  check(shadow.reason==Reason::accepted&&shadow.damage==plan.damage-5);
  const auto steal=actor.apply_feast_followup(life,plan,{100,1000},100);
  check(steal&&steal->stolen==plan.steal&&steal->source.current==100+plan.steal);
  const auto next=actor.apply_damage(life,{Kind::armor_ignoring,50,20,42},101);check(next.damage==40);
  auto stale=life;++stale.actor_generation;check(!actor.apply_feast_followup(stale,plan,{100,1000},101));
  auto forged=plan;++forged.steal;check(!actor.apply_feast_followup(life,forged,{100,1000},101));
  check(!actor.apply_feast_followup(life,plan,{100,1000},99));
 }

 {
  auto reviewed=d;reviewed.spawn_health=Health{100,1000};reviewed.healing_allowed=true;reviewed.healing_evidence="authored conversion fixture";
  LiveActor actor(reviewed,life);check(actor.stage_shield_absorption(life,12,1,77,100));
  const auto plan=*skills::effects::feast_of_corruption(12);
  const auto capped=actor.apply_feast_followup(life,plan,{1000,1000},100);
  check(capped&&capped->stolen==plan.steal&&capped->gained==0&&capped->source.current==1000);
  const auto hit=actor.apply_damage(life,{Kind::armor_ignoring,50,20,42},101);check(hit.damage==45);
  check(actor.apply_reversal_fortune(life,12,100,78,102));
  const auto hp=actor.actor(life)->health->current;
  check(!actor.apply_feast_followup(life,plan,{100,1000},102));
  check(actor.actor(life)->health->current==hp&&actor.owned_effect_snapshot(life)->newest_skill(actor.hex_scope(),307,102));
 }
 for(bool weapon:{false,true}){
  LiveActor actor(d,life);
  check(!actor.stage_shield_absorption(life,12,65536,77,100));
  check(!actor.stage_shield_absorption(life,12,1,0,100));
  auto stale=life;++stale.actor_generation;check(!actor.stage_shield_absorption(stale,12,1,77,100));
  check(actor.stage_shield_absorption(life,12,1,77,100));
  // Invalid/nonfinite damage must not consume the first reduction step.
  if(weapon)check(!actor.apply_weapon_damage(life,-1,100));
  else check(actor.apply_damage(life,{Kind::earth,-1,20,1},100).reason!=Reason::accepted);
  for(unsigned n=1;n<=4;++n){
   const auto hit=weapon?actor.apply_weapon_damage(life,50,100+n):std::optional(actor.apply_damage(life,{Kind::earth,50,20,1},100+n));
   check(hit&&hit->reason==Reason::accepted&&hit->damage==50-5*n);
  }
  check(actor.stage_shield_absorption(life,0,1,78,200));
  auto hit=weapon?actor.apply_weapon_damage(life,50,201):std::optional(actor.apply_damage(life,{Kind::earth,50,20,1},201));
  check(hit&&hit->damage==45);
  hit=weapon?actor.apply_weapon_damage(life,50,3200):std::optional(actor.apply_damage(life,{Kind::earth,50,20,1},3200));
  check(hit&&hit->damage==50);
 }
 for(bool weapon:{false,true}){
  LiveActor actor(d,life);
  check(!actor.stage_shield_absorption(life,12,65536,77,100));
  check(!actor.stage_shield_absorption(life,12,1,0,100));
  auto stale=life;++stale.actor_generation;check(!actor.stage_shield_absorption(stale,12,1,77,100));
  check(actor.stage_shield_absorption(life,12,1,77,100));
  check(actor.apply_protective_spirit(life,{1,1,1},*skills::effects::protective_spirit(12),78,100));
  // 500 becomes the 100HP cap, then the first absorption step removes5.
  auto hit=weapon?actor.apply_weapon_damage(life,500,101):std::optional(actor.apply_damage(life,{Kind::armor_ignoring,500,20,1},101));
  check(hit&&hit->damage==95);
  // A discarded damage proposal must not spend the authoritative second step.
  auto proposal=actor;
  auto trial=weapon?proposal.apply_weapon_damage(life,500,102):std::optional(proposal.apply_damage(life,{Kind::armor_ignoring,500,20,1},102));
  check(trial&&trial->damage==90);
  hit=weapon?actor.apply_weapon_damage(life,500,102):std::optional(actor.apply_damage(life,{Kind::armor_ignoring,500,20,1},102));
  check(hit&&hit->damage==90);
 }
 {
  auto profile=d;profile.healing_allowed=true;profile.healing_evidence="test";
  LiveActor actor(profile,life);check(actor.stage_shield_absorption(life,12,1,77,100));
  check(actor.apply_reversal_fortune(life,12,100,78,100));
  const auto reversed=actor.apply_reversal_damage(life,{Kind::earth,50,20,1},101,true);
  check(bool(reversed));
  const auto next=actor.apply_damage(life,{Kind::earth,50,20,1},102);
  check(next.reason==Reason::accepted&&next.damage==40);
 }
 {
  auto profile=d;profile.healing_allowed=true;profile.healing_evidence="test";
  LiveActor before(profile,life);check(before.stage_shield_absorption(life,12,1,77,100));
  check(before.apply_reversal_fortune(life,12,100,78,100));
  auto incoming=before;const auto hit=incoming.apply_weapon_damage(life,50,101);check(hit&&hit->damage==45);
  auto converted=before;const auto receipt=converted.apply_reversal_weapon_hit(life,*hit,101);check(bool(receipt));
  check(converted.adopt_absorption_hit(before,incoming,life,101));
  const auto next=converted.apply_weapon_damage(life,50,102);check(next&&next->damage==40);
  auto forged=before;check(forged.stage_shield_absorption(life,12,1,79,101));
  check(!forged.adopt_absorption_hit(before,incoming,life,101));
 }
 {
  LiveActor current(d,life);check(current.stage_shield_absorption(life,12,1,77,100));
  auto proposal=current;check(proposal.apply_damage(life,{Kind::earth,50,20,1},101).damage==45);
  check(current.apply_damage(life,{Kind::earth,50,20,1},101).damage==45);
  auto refreshed=proposal.refresh_damage_proposal(current,life,{Kind::earth,50,20,1},102);
  check(refreshed&&refreshed->damage==40&&proposal.actor(life)->health->current==915);
  check(current.stage_shield_absorption(life,0,1,78,103));
  refreshed=proposal.refresh_damage_proposal(current,life,{Kind::earth,50,20,1},103);
  check(refreshed&&refreshed->damage==45&&proposal.actor(life)->health->current==910);
  auto stale=life;++stale.actor_generation;
  check(!proposal.refresh_damage_proposal(current,stale,{Kind::earth,50,20,1},104));
 }
 {
  auto profile=d;profile.conditions_allowed=true;
  LiveActor actor(profile,life);check(actor.stage_shield_absorption(life,12,1,77,100));
  const auto result=actor.apply_earth_aoe(life,*skills::effects::caster_earth_aoe(937,0),0,20,1,101);
  check(result&&result->hits.size()==3);
  check(result->hits[0].damage==10&&result->hits[1].damage==5&&result->hits[2].damage==0);
  const auto next=actor.apply_damage(life,{Kind::armor_ignoring,50,20,1},102);
  check(next.reason==Reason::accepted&&next.damage==30);
 }
 {
  auto profile=d;profile.conditions_allowed=true;
  LiveActor actor(profile,life);check(actor.stage_shield_absorption(life,12,1,77,100));
  const auto result=actor.apply_ash_blast(life,*skills::effects::ash_blast(0),{1,1,1},78,20,0,101);
  check(result&&result->hit.damage==30);
  const auto next=actor.apply_damage(life,{Kind::armor_ignoring,50,20,1},102);
  check(next.reason==Reason::accepted&&next.damage==40);
 }
 {
  LiveActor actor(d,life);check(actor.stage_shield_absorption(life,12,1,77,100));
  // Refused conditions abort the entire AoE, including all speculative steps.
  check(!actor.apply_earth_aoe(life,*skills::effects::caster_earth_aoe(937,0),0,20,1,101));
  const auto next=actor.apply_damage(life,{Kind::armor_ignoring,50,20,1},102);
  check(next.reason==Reason::accepted&&next.damage==45);
 }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
