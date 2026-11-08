#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Rotting Flesh Disease line "+std::to_string(at.line()));
}
int main()try{
 for(bool npc_source:{false,true})for(bool reused_caster:{false,true}){
  world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u}){
   if(!npc_source&&id==100)continue;
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?60.f:220.f;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{id==100?1000.:1.,1000};
   d.damageable=d.conditions_allowed=true;d.fleshy=true;d.fleshy_evidence="fixture";
   d.creature_type="human";d.creature_type_evidence="fixture";d.disease_immune=false;d.disease_immunity_evidence="fixture";
   n.combat_definition=d;npcs.push_back(n);
  }
  w.configure_npcs(npcs);
  w.join(1,"Caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc_source){w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  const auto origin=*w.effect_scope(1),source=*w.effect_scope(npc_source?100:2),recipient=*w.effect_scope(101);
  check(w.rotting_flesh(origin,source,*rotting_flesh(0),100));
  check(w.has_condition(source,Kind::disease,100)&&!w.has_condition(recipient,Kind::disease,100));
  w.tick(200);check(w.has_condition(recipient,Kind::disease,200));
  if(reused_caster){
   w.leave(1);w.join(1,"Replacement",1000,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{1000,1000,100,100},100);
   check(*w.effect_scope(1)!=origin);
   // Another condition under the reused numeric ID must not steal Disease credit.
   check(w.condition_spell(*w.effect_scope(1),recipient,*condition_spell(117,0),250));
  }
  w.tick(400);check(w.npc_combat_snapshot(recipient)->health->current==0);
  check(w.take_npc_deaths(origin).size()==unsigned(!reused_caster));
  if(reused_caster)check(w.take_npc_deaths(*w.effect_scope(1)).empty());
 }
 std::cout<<"PASS Rotting Flesh single-target then shared player/NPC contagion,original Disease death credit and reused-caster rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
