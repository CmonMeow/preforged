// World regression for Earth batch Seed admission before any cast mutation.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earth Seed admission line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,early,later,monk,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,bool cross_survival=false){
  world::Instance::Npc body;body.id=body.definition.id=100;body.x=cross_survival?390:170;
  body.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition definition;definition.binding.map=146;
  definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;
  definition.spawn_health={500,1000};definition.damageable=true;definition.healing_allowed=allowed;
  if(allowed.has_value())definition.healing_evidence="synthetic admission fixture";
  body.combat_definition=definition;w.configure_npcs({body});
  for(unsigned id:{1u,2u,3u,4u}){
   const float x=id==1?0:id==2?(cross_survival?100.f:-100.f):id==3?(cross_survival?250.f:160.f):(cross_survival?400.f:323.f);
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:cross_survival&&id==3?1u:500u,1000,100,100},100);
   check(w.combat_armor(*w.effect_scope(id),60));check(w.elemental_armor(*w.effect_scope(id),60,60));
  }
  source=*w.effect_scope(1);early=*w.effect_scope(2);later=*w.effect_scope(3);
  monk=*w.effect_scope(4);npc=*w.effect_scope(100);
  check(w.status_mutation(early,1399,800,12,100,6100,true));
  if(!cross_survival)check(w.status_mutation(later,1399,801,12,100,6100,true));
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  check(w.complete_healing_seed(monk,later,*effects::healing_seed(12),0,100));
  if(cross_survival)check(w.complete_healing_seed(monk,early,*effects::healing_seed(12),0,100));
  box->drain();
 }
 void counter_unspent(){
  check(w.before_spell_effect(source,42,1,100));
  check(w.energy_drain(source,early,*effects::energy_drain(42,12),20,100,nullptr,1));
  // The first reduction is5. A rejected earlier Earth sequence must not spend it.
 }
};
unsigned earth_packets(const std::vector<world::Packet>& packets){
 return unsigned(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,937)));
}
int main()try{
 const auto plan=*effects::caster_earth_aoe(937,12);
 const auto healing=effects::healing_seed(12)->healing;
 for(const auto allowed:{std::optional<bool>{},std::optional<bool>{false},std::optional<bool>{true}}){
  Fixture f(allowed);const bool accepted=f.w.shockwave(f.source,plan,20,100);
  const auto packets=f.box->drain();
  if(!allowed.has_value()){
   check(!accepted);
   check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.resources(4)->health==500);
   check(f.w.npc_combat_snapshot(f.npc)->health->current==500&&earth_packets(packets)==0);
   for(const auto scope:{f.early,f.later})for(const auto kind:{combat::conditions::Kind::weakness,combat::conditions::Kind::cracked_armor,combat::conditions::Kind::blind})
    check(!f.w.has_condition(scope,kind,100));
   f.counter_unspent();check(f.w.resources(2)->health==433);
  }else{
   check(accepted&&f.w.resources(2)->health==377&&f.w.resources(3)->health==377+3*healing);
   check(f.w.resources(4)->health==500+3*healing);
   check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(*allowed?3*healing:0));
   check(earth_packets(packets)==6);
  }
 }
 // The later1HP recipient initially appears to die on its first Earth hit.
 // Earlier target's three Seed triggers heal it before its sequence, enabling
 // a later Seed trigger that exposes the unknown-healing NPC. Admission must
 // share projected Health across targets instead of independently skipping it.
 {
  Fixture f(std::nullopt,true);
  check(1+3*healing>plan.strikes[0].earth_damage);
  check(!f.w.shockwave(f.source,plan,20,100));
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==1&&f.w.resources(4)->health==500);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500&&earth_packets(f.box->drain())==0);
  // Use the Earth path itself here: only the early recipient is within240.
  // Its first1399contact removes5, then Seed heals that recipient.
  // EnergyBurn currently has a separate recorded missing Seed consumer.
  check(f.w.shockwave(f.source,*effects::caster_earth_aoe(174,0),20,100));
  check(f.w.resources(2)->health==500-(effects::caster_earth_aoe(174,0)->strikes[0].earth_damage-5)+healing);
 }
 // Unknown healing eligibility is irrelevant when the only seeded contact
 // is reduced to zero, or entirely converted. Do not reject harmless contacts
 // merely because a nearby NPC has not yet been classified.
 for(bool converted:{false,true}){
  Fixture f(std::nullopt);
  if(converted)check(f.w.status_mutation(f.later,307,802,12,100,8100,true));
  else {check(f.w.combat_armor(f.later,400));check(f.w.elemental_armor(f.later,400,400));}
  check(f.w.shockwave(f.source,*effects::caster_earth_aoe(174,0),20,100));
  const auto loss=effects::caster_earth_aoe(174,0)->strikes[0].earth_damage-5;
  check(f.w.resources(2)->health==500-loss&&f.w.resources(3)->health==(converted?500+loss:500u));
  check(f.w.resources(4)->health==500&&f.w.npc_combat_snapshot(f.npc)->health->current==500);
 }
 std::cout<<"PASS Earth Seed unknown/immune/allowed admission and shared later-recipient survival\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
