#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Snow Seed line "+std::to_string(at.line()));
}
int main()try{unsigned cases=0;
 for(int allowed:{-1,0,1})for(bool prevent:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=910;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;
  if(allowed>=0){d.healing_allowed=allowed!=0;d.healing_evidence="synthetic cold Seed recipient";}n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Cold Seed",id==1?0:id==2?600:id==3?760:920,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{500,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  const auto source=*w.effect_scope(1),early=*w.effect_scope(2),later=*w.effect_scope(3),monk=*w.effect_scope(4),ally=*w.effect_scope(100);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));const auto seed=*healing_seed(12);
  check(w.complete_healing_seed(monk,later,seed,0,100));if(prevent)check(w.status_mutation(later,307,80,12,100,8100,true));
  check(w.stage_snow_storm(source,early,*snow_storm(0),20,100));w.tick(1100);
  if(allowed<0&&!prevent){check(w.snow_storm_refused_pulses()==1);for(unsigned id:{2u,3u,4u})check(w.resources(id)->health==500);check(w.npc_combat_snapshot(ally)->health->current==500);}
  else if(prevent){check(!w.snow_storm_refused_pulses());check(w.resources(2)->health==470&&w.resources(3)->health==530&&w.resources(4)->health==500&&w.npc_combat_snapshot(ally)->health->current==500);}
  else{check(!w.snow_storm_refused_pulses());check(w.resources(2)->health==470+seed.healing&&w.resources(3)->health==470+seed.healing&&w.resources(4)->health==500+seed.healing);check(w.npc_combat_snapshot(ally)->health->current==500+(allowed?seed.healing:0));}
  ++cases;
 }
 std::cout<<cases<<" cold Seed whole-cohort unknown/allowed/immune healing and Reversal-zero-damage no-trigger cases passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
