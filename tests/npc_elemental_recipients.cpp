#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC elemental recipients line "+std::to_string(at.line()));
}
Definition definition(){Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
 d.relation=Relation::hostile_to_players;d.spawn_health=Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
 d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.physical_armor=60;
 d.spawn_energy=Energy{40,40};d.energy_pips=0;return d;}
int main()try{
 const Lifetime life{1,2,3,146,100};
 for(unsigned skill:{191u,89u,1379u,2192u,846u}){
  LiveActor actor(definition(),life);const auto result=actor.apply_elemental(life,*skills::effects::elemental(skill,0),20,1,100);
  check(bool(result));check(result->hits.size()==(skill==89?2u:1u));
  check(actor.actor(life)->health->current==1000-(skill==191||skill==846?20:skill==89?10:5));
  check(actor.has_condition(life,conditions::Kind::burning,100)==(skill==191));
 }
 {auto d=definition();d.fire_multiplier=2;d.fire_multiplier_evidence="authored fixture";LiveActor actor(d,life);
  check(bool(actor.apply_elemental(life,*skills::effects::elemental(191),20,1,100)));
  check(actor.actor(life)->health->current==960);
  const auto steam=actor.apply_elemental(life,*skills::effects::elemental(846),20,1,100);
  check(steam&&actor.actor(life)->health->current==940&&actor.has_condition(life,conditions::Kind::blind,100));
  const auto gaze=actor.apply_elemental(life,*skills::effects::elemental(1379,0,12),20,1,100);check(gaze&&gaze->energy==11);
 }
 {auto d=definition();d.conditions_allowed=false;LiveActor actor(d,life);const auto hit=actor.apply_elemental(life,*skills::effects::elemental(191),20,1,100);
  check(hit&&hit->additions.empty()&&actor.actor(life)->health->current==980);}
 {LiveActor actor(definition(),life);check(actor.activate_storm_chaser(life,12,123,100));
  check(bool(actor.apply_elemental(life,*skills::effects::elemental(89),20,1,100)));
  check(actor.energy(life)->current==34); // Cold hit qualifies; additional shadow damage does not.
  check(actor.advance_energy(life,200));check(!actor.apply_elemental(life,*skills::effects::elemental(191),20,1,199));
  check(actor.actor(life)->health->current==990);}
 for(unsigned mode=0;mode<4;++mode){auto d=definition();if(mode==0)d.fire_armor.reset();if(mode==1)d.relation=Relation::neutral;
  LiveActor actor(d,life);auto expected=life;if(mode==2)++expected.actor_generation;
  auto plan=*skills::effects::elemental(191);if(mode==3)++plan.damage;
  check(!actor.apply_elemental(expected,plan,20,1,100)&&actor.actor(life)->health->current==1000);}
 for(unsigned skill:{191u,89u,1379u,2192u,846u}){
  world::Instance w;world::Instance::Npc body;body.id=body.definition.id=100;body.x=60;body.definition.level=20;
  body.definition.visual_adjustment={0,0,0,100};body.combat_definition=definition();w.configure_npcs({body});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,50,100},100);box->drain();
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);std::vector<world::Packet> visuals;
  check(w.elemental(owner,target,*skills::effects::elemental(skill),20,100,&visuals));
  check(w.npc_combat_snapshot(target)->health->current==1000-(skill==191||skill==846?20:skill==89?10:5));
  check(!box->drain().empty());auto stale=target;++stale.generation;
  check(!w.elemental(owner,stale,*skills::effects::elemental(skill),20,100));
 }
 for(unsigned mode=0;mode<4;++mode){world::Instance w;world::Instance::Npc body;body.id=body.definition.id=100;
  body.x=mode==0?1300:60;body.plane=mode==1?1:0;body.definition.level=20;body.definition.visual_adjustment={0,0,0,100};
  body.combat_definition=definition();w.configure_npcs({body});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,50,100},100);
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);
  if(mode<2){check(!w.elemental(owner,target,*skills::effects::elemental(191),20,100));continue;}
  const auto hex=skills::effects::movement_hex(mode==2?210u:76u,0);check(bool(hex));
  check(w.movement_hex(owner,target,*hex,100));
  check(w.elemental(owner,target,*skills::effects::elemental(2192,0,12),20,100));
  check(w.resources(1)->energy==(mode==2?61u:50u));
 }
 std::cout<<"PASS NPC elemental recipients: five plans, damage, fire vulnerability, conditions, Energy predicates, stale/unknown/forged rejection and World publication\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
