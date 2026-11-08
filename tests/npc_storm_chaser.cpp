#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC Storm Chaser line "+std::to_string(at.line()));
}
Definition definition(){Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
 d.relation=Relation::hostile_to_players;d.spawn_health=Health{5000,5000};d.damageable=true;d.conditions_allowed=true;
 d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.physical_armor=60;
 d.spawn_energy=Energy{40,40};d.energy_pips=0;d.knockdown_allowed=true;d.knockdown_evidence="authored fixture";return d;}
int main()try{
 const Lifetime life{1,2,3,146,100};
 for(unsigned kind=0;kind<7;++kind){LiveActor actor(definition(),life);check(actor.activate_storm_chaser(life,12,123,100));
  check(actor.energy(life)->current==30&&actor.movement_multiplier(life,100)==1.25f);
  if(kind<4){const auto element=kind==0?Kind::earth:kind==1?Kind::fire:kind==2?Kind::cold:Kind::lightning;
   check(actor.apply_damage(life,{element,kind==0?0.:20.,20,194},100).reason==Reason::accepted);
  }
  if(kind==4)check(actor.apply_caster_weapon_damage(life,0,5,100).has_value());
  if(kind==5)check(actor.apply_weapon_damage(life,20).has_value());
  if(kind==6)check(actor.apply_damage(life,{Kind::armor_ignoring,20,20,133},100).reason==Reason::accepted);
  check(actor.energy(life)->current==(kind<5?34u:30u));
 }
 for(auto relation:{Relation::neutral,Relation::protected_ally}){auto d=definition();d.relation=relation;if(relation==Relation::protected_ally){d.damageable=false;d.conditions_allowed=false;}LiveActor actor(d,life);
  check(actor.activate_storm_chaser(life,12,123,100)&&actor.energy(life)->current==30&&actor.movement_multiplier(life,100)==1.25f);
  if(relation==Relation::neutral){check(actor.apply_guardian_damage(life,{Kind::cold,20,20,194},100).reason==Reason::accepted);check(actor.energy(life)->current==34);}
 }
 for(unsigned raw:{6u,7u}){LiveActor actor(definition(),life);check(actor.activate_storm_chaser(life,12,123,100));
  check(actor.apply_caster_weapon_damage(life,20,raw,100).has_value()&&actor.energy(life)->current==30);
 }
 {LiveActor actor(definition(),life);check(actor.activate_storm_chaser(life,12,123,100));
  const auto result=actor.apply_earth_aoe(life,*skills::effects::caster_earth_aoe(937,0),0,20,1,100);check(bool(result));
  check(result->hits.size()==3&&actor.energy(life)->current==40);
 }
 for(unsigned mode=0;mode<5;++mode){auto d=definition();if(mode==0){d.spawn_energy.reset();d.energy_pips.reset();}if(mode==1)d.energy_pips.reset();if(mode==2)d.spawn_energy=Energy{9,40};
  LiveActor actor(d,life);auto stale=life;++stale.actor_generation;
  check(!actor.activate_storm_chaser(mode==3?stale:life,mode==4?21u:12u,123,100));
  check(actor.movement_multiplier(life,100)==1&&(!d.spawn_energy||actor.energy(life)==d.spawn_energy));
 }
 {LiveActor actor(definition(),life);check(actor.activate_storm_chaser(life,0,123,100));
  check(!actor.activate_storm_chaser(life,0,124,20099)&&actor.energy(life)->current==30);
  check(actor.movement_multiplier(life,8100)==1);
  check(actor.activate_storm_chaser(life,0,124,20100)&&actor.energy(life)->current==20);
  check(actor.advance_energy(life,21000));const auto before=*actor.actor(life);
  check(actor.apply_damage(life,{Kind::fire,20,20,194},20999).reason!=Reason::accepted);
  check(actor.actor(life)->health->current==before.health->current&&actor.energy(life)->current==20);
 }
 {LiveActor actor(definition(),life);check(actor.activate_storm_chaser(life,12,123,100));
  check(std::abs(actor.movement_integral(life,100,1100)-1250.)<.001);
  auto old=actor;check(actor.remove_hex_effect(life,123,1100));
  check(old.preserve_current_hexes(actor,life,1100)&&old.movement_multiplier(life,1100)==1);
 }
 {LiveActor actor(definition(),life);check(actor.activate_storm_chaser(life,12,123,100));
  check(actor.apply_interrupt_stance(life,16,0,124,200,1000));check(actor.movement_multiplier(life,200)==1);
  check(actor.apply_damage(life,{Kind::fire,20,20,194},200).reason==Reason::accepted&&actor.energy(life)->current==30);
 }
 {LiveActor actor(definition(),life);
  check(actor.configure_spellbook(life,{{{194,0},1248,"authored spell/action fixture"}}));
  const ai::SpellTarget foe{{1,1,1},100,true,true,true,true};
  const auto cast=actor.begin_spell(life,{194,0},foe,100);check(bool(cast));const auto epoch=actor.control_epoch(life);
  check(actor.activate_storm_chaser(life,12,123,100)&&actor.energy(life)->current==25);
  check(actor.control_epoch(life)==epoch&&actor.complete_action(life,*cast,cast->completes));
 }
 {world::Instance w;world::Instance::Npc body;body.id=body.definition.id=100;body.x=60;body.definition.level=20;body.definition.visual_adjustment={0,0,0,100};body.combat_definition=definition();w.configure_npcs({body});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Player",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);box->drain();
  const auto owner=*w.effect_scope(100);check(!w.npc_storm_chaser(owner,12,100,""));
  check(w.npc_storm_chaser(owner,12,100,"authored test loadout; native base cost; no inferred creature binding"));
  check(w.npc_energy_snapshot(owner)->current==30);
  const world::Packet body_packet{158,0,21,0,0,0,100,0,0,0,255,2,0,0};
  const auto sent=box->drain();check(std::find(sent.begin(),sent.end(),body_packet)!=sent.end());
  check(!w.npc_storm_chaser(owner,12,200,"same authored test loadout"));
  check(w.elemental(*w.effect_scope(1),owner,*skills::effects::elemental(194,0),20,200));w.tick(1000);
  check(w.npc_energy_snapshot(owner)->current==34);
 }
 std::cout<<"PASS NPC Storm Chaser known-resource payment,cooldown,all elemental/zero/multiple/weapon hits,cap,movement,stale/future/unknown rejection and host publication\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
