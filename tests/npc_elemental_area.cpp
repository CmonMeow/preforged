#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC elemental area line "+std::to_string(at.line())+context);
}
int main()try{
 for(unsigned skill:{183u,1095u,884u})for(bool unknown:{false,true}){
  context=" skill="+std::to_string(skill)+" unknown="+std::to_string(unknown);
  world::Instance w;std::vector<world::Instance::Npc> bodies;
  for(unsigned id=100;id<105;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==102?1000.f:float(60+(id-100)*10);n.plane=id==103?1:0;
   n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
   d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
   d.relation=id==104?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=id!=104;d.conditions_allowed=id!=104;d.fire_armor=60;
   if(unknown&&id==101)d.fire_armor.reset();n.combat_definition=d;bodies.push_back(n);
  }
  w.configure_npcs(bodies);auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,50,100},100);check(w.energy_capacity(1,1,0,100,{80,-2}));
  w.join(2,"Enemy",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,50,100},100);
  const auto owner=*w.effect_scope(1);check(w.elemental_armor(*w.effect_scope(2),60,60));box->drain();
  auto plan=*elemental(skill,0);const auto selected=skill!=183?*w.effect_scope(100):owner;std::vector<world::Packet> visuals;
  const bool accepted=w.elemental(owner,selected,plan,20,100,&visuals);
  // Searing Flames' first application is Burning only and needs no armor.
  check(accepted==(!unknown||skill==884));
  for(unsigned id=100;id<105;++id){const auto scope=*w.effect_scope(id);const double health=w.npc_combat_snapshot(scope)->health->current;
   check(health==1000-((accepted&&id<102&&skill!=884)?plan.damage:0));
   check(w.has_condition(scope,combat::conditions::Kind::burning,100)==(accepted&&id<102&&(skill==884||skill==1095)));
  }
  check(w.resources(2)->health==1000-((accepted&&skill!=884)?unsigned(plan.damage):0));
  check(w.resources(1)->energy==50+((accepted&&skill==1095)?6:0));
  check(visuals.size()==(accepted&&skills::particles::targeted_body_2022(100,1,skill)?2u:0u));
  if(skill==884){check(w.elemental(owner,selected,plan,20,100,&visuals)==!unknown);
   check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==(unknown?1000:990));
   check(w.resources(2)->health==(unknown?1000u:990u));}
 }
 std::cout<<"PASS mixed NPC/player elemental areas: radius,plane,protected ally,unknown-armor atomic admission,Burning-only then damage,per-recipient Energy and deferred NPC visuals\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
