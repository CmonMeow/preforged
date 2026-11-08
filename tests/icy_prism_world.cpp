#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Icy Prism World line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source;
 Fixture(bool unknown=false,unsigned primary_health=1000){std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?100:352;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{id==100?double(primary_health):1000.,1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";if(!(unknown&&id==101))d.cold_armor=60;n.combat_definition=d;population.push_back(n);}
  w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u,4u,5u}){w.join(id,"Actor",id==1?0:id==2||id==5?100:id==3?352:353,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  source=*w.effect_scope(1);
 }
 effects::Scope scope(unsigned id){return *w.effect_scope(id);}
 double hp(unsigned id){return id<100?w.resources(id)->health:w.npc_combat_snapshot(scope(id))->health->current;}
};
int main()try{
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  Fixture f(mode==4||mode==5);const auto target=f.scope(npc?100:2);const bool water=mode==1||mode==3||mode==4;
  if(water)check(f.w.movement_hex(f.source,target,*effects::movement_hex(210,0),100));
  if(mode==2)check(f.w.soul_barbs(f.source,target,*effects::soul_barbs(12),20,100));
  const unsigned at=mode==3?8100:1000;const bool expanded=water&&mode!=3;
  std::vector<world::Packet> visuals;f.box->drain();
  const bool ok=f.w.stage_icy_prism(f.source,target,*effects::icy_prism(12),20,at,&visuals);
  check(ok==(mode!=4));
  if(mode==4){for(unsigned id:{1u,2u,3u,4u,5u,100u,101u})check(f.hp(id)==1000);check(visuals.empty());continue;}
  check(f.hp(target.actor)==937);
  check(f.hp(npc?2:100)==(expanded?937:1000)&&f.hp(3)==(expanded?937:1000)&&f.hp(101)==(expanded?937:1000));
  check(f.hp(4)==1000&&f.hp(5)==1000&&f.hp(1)==1000);
  check(visuals.size()==1&&visuals[0]==*particles::targeted_body_2022(target.actor,1,903));
  const auto sent=f.box->drain();check(std::count(sent.begin(),sent.end(),visuals[0])==0);
  f.w.tick(at+100);check(f.hp(target.actor)==937);
 }
 {Fixture f;const auto target=f.scope(2);check(f.w.movement_hex(f.source,target,*effects::movement_hex(210,0),100));
  check(f.w.protective_enchantment(target,target,*regeneration::reviewed(299,0),0,100));
  check(f.w.stage_icy_prism(f.source,target,*effects::icy_prism(12),20,1000));check(f.hp(2)==940&&f.hp(3)==937);
 }
 {Fixture f;auto target=f.scope(2);++target.generation;check(!f.w.stage_icy_prism(f.source,target,*effects::icy_prism(12),20,1000));
  auto bad=*effects::icy_prism(12);++bad.damage;check(!f.w.stage_icy_prism(f.source,f.scope(2),bad,20,1000));check(f.hp(2)==1000&&f.hp(100)==1000);
 }
 for(bool npc:{false,true}){
  Fixture f(false,10);const auto target=f.scope(npc?100:2);
  if(!npc)f.w.set_vitals(2,{10,1000,100,100},100);
  check(f.w.movement_hex(f.source,target,*effects::movement_hex(210,0),100));
  std::vector<world::Packet> visuals;check(f.w.stage_icy_prism(f.source,target,*effects::icy_prism(12),20,1000,&visuals));
  check(f.hp(target.actor)==0&&f.hp(3)==937&&f.hp(101)==937);
  check(visuals.size()==1&&visuals[0]==*particles::targeted_body_2022(target.actor,1,903));
  if(npc){const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1&&deaths[0].time==1000);}
  f.w.tick(1100);check(f.hp(target.actor)==0&&f.hp(3)==937&&f.hp(101)==937);
  if(npc)check(f.w.take_npc_deaths(f.source).empty());
 }
 for(bool leaving:{false,true})for(bool explicit_tick:{false,true}){
  world::Instance w;world::Instance::Npc guard;guard.id=guard.definition.id=100;guard.x=leaving?1000:0;guard.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::neutral;
  d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.cold_armor=60;guard.combat_definition=d;w.configure_npcs({guard});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  const auto caster=*w.effect_scope(1),primary=*w.effect_scope(2),guardian=*w.effect_scope(100);
  auto enemy=guard;enemy.x=leaving?100:500;enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
  check(w.begin_wave_objective(caster,guardian,73,1,3,1)&&w.spawn_quest_wave(caster,73,1,{enemy},[]{return true;}));
  const auto moving=*w.effect_scope(60000);check(w.chase_quest_guardian(moving,100,100));
  check(w.movement_hex(caster,primary,*effects::movement_hex(210,0),100));if(explicit_tick)w.tick(3100);
  check(w.stage_icy_prism(caster,primary,*effects::icy_prism(12),20,3100));
  const auto state=w.npc_combat_snapshot(moving);if(state->health->current!=(leaving?1000:937))throw std::runtime_error("moving leaving="+std::to_string(leaving)+" tick="+std::to_string(explicit_tick)+" hp="+std::to_string(state->health->current));
 }
 for(bool npc:{false,true})for(bool water:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  Fixture f(false,hp);f.w.set_vitals(2,{hp,1000,100,100},100);const auto target=f.scope(npc?100:2);const auto plan=*effects::icy_prism(12);
  if(water)check(f.w.movement_hex(f.source,target,*effects::movement_hex(210,0),100));
  for(unsigned id:{2u,3u,100u,101u}){const auto rank=id==target.actor?rr:id==101?0u:12u;
   if(id<100)check(f.w.status_mutation(f.scope(id),307,id+10,rank,100,8100,true));else check(f.w.stage_npc_reversal(f.scope(id),rank,100));}
  std::map<unsigned,unsigned> owned;for(unsigned id:{2u,3u,100u,101u}){const auto entries=*f.w.status_snapshot(f.scope(id),100);for(const auto& entry:entries)if(entry.skill==307)owned[id]=entry.instance;}check(owned.size()==4);
  f.box->drain();std::vector<world::Packet> visuals;check(f.w.stage_icy_prism(f.source,target,plan,20,1000,&visuals));
  for(unsigned id:{2u,3u,100u,101u}){const double before=id==2||id==100?hp:1000;const unsigned rank=id==target.actor?rr:id==101?0u:12u;
   const double raw=id==target.actor?plan.damage:plan.secondary_damage,converted=std::min(raw,double(effects::reversal_fortune(rank)->maximum_healing));
   const double expected=water||id==target.actor?std::max(0.,std::min(1000.,before+converted)-(raw-converted)):before;check(std::abs(f.hp(id)-expected)<1.1);
  }
  check(f.hp(4)==1000&&f.hp(5)==1000&&visuals.size()==1);
  const auto packets=f.box->drain();for(const auto& [id,effect]:owned){world::Packet retirement{67,0};combat::detail::put(retirement,id);combat::detail::put(retirement,effect);
   check(std::count(packets.begin(),packets.end(),retirement)==unsigned(water||id==target.actor));
   if(!water&&id!=target.actor){const auto entries=*f.w.status_snapshot(f.scope(id),1000);check(std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;}));}
  }
 }
 std::cout<<"PASS trusted Icy Prism World player/NPC registries,Water-only nearby splash,expiry,no double primary,unknown cold atomic rejection,protection and captured primary body;moving recipients,lethal primary and once-only deaths;rendered verification separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
