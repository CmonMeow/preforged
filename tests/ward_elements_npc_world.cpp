#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Ward175 actual NPC line "+std::to_string(at.line()));}
world::Instance::Npc make_ward_npc(unsigned id){
 world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};n.x=id==100?100:200;
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=id==100?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
 d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
 d.creature_type="human";d.creature_type_evidence="authored test";d.physical_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=60;
 n.combat_definition=d;return n;
}
int main()try{
 for(unsigned raw:{0u,3u,4u,5u,11u}){
  double losses[2]{};
  for(unsigned warded=0;warded<2;++warded){world::Instance w;w.configure_npcs({make_ward_npc(100),make_ward_npc(101)});
   w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
   if(warded)check(w.apply_ward_elements(*w.effect_scope(1),*skills::effects::ward_elements_plan(0),100));
   const auto ally=*w.effect_scope(100),enemy=*w.effect_scope(101);
   const combat::ai::ordinary::Reviewed weapon{20,20,12,raw,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"authored elemental NPC contact fixture"};
   check(w.configure_npc_opposed_ordinary(enemy,weapon,1000));check(bool(w.begin_npc_opposed_ordinary(enemy,ally,1000)));w.tick(1750);
   losses[warded]=1000-w.npc_combat_snapshot(ally)->health->current;check(losses[warded]>0);
   if(warded){const auto before=w.npc_combat_snapshot(ally)->health->current;
    check(bool(w.begin_npc_opposed_ordinary(enemy,ally,8100)));w.tick(8850);
    const auto after=w.npc_combat_snapshot(ally)->health->current;check(std::abs(before-after-losses[0])<.001);}
  }
  check(std::abs(losses[1]-losses[0]*(raw?std::exp2(-24./40.):1.))<.001);
 }
 std::cout<<"Ward175 actual opposing NPC physical/four-elemental contacts and post-expiry armor passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
