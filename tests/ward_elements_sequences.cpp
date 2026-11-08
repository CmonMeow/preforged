#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Ward175 sequence line "+std::to_string(at.line()));}
int main()try{
 {world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.combat_armor(source,60)&&w.combat_armor(target,60));check(w.apply_ward_elements(source,*ward_elements_plan(0),100));
  check(w.stage_watch_yourself_recipient(target,target,*watch_yourself(0,true),100));
  check(w.shockwave(foe,*caster_earth_aoe(174,0),20,200));
  const double hit=26*std::exp2(-double(stacked_skill_armor(24,5))/40.);
  check(w.resources(2)->health==unsigned(std::ceil(1000-hit))&&w.watch_yourself_armor(target,200)==0);
 }
 {combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.earth_armor=60;
  combat::npcs::LiveActor live(d,{1,1,1,146,100});const auto life=live.lifetime();
  live.environmental_elemental_armor([](auto,auto){return 24u;});
  check(bool(live.apply_watch_yourself(life,live.hex_scope(),*watch_yourself(0,true),1,100)));
  const auto result=live.apply_earth_aoe(life,*caster_earth_aoe(174,0),0,20,2,200);
  check(result&&result->hits.size()==1&&std::abs(result->hits.front().applied-26*std::exp2(-double(stacked_skill_armor(24,5))/40.))<.001);
 }
 for(unsigned schedule=0;schedule<3;++schedule){world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.lightning_armor(target,60));check(w.apply_ward_elements(source,*ward_elements_plan(0),100));
  check(w.activate_overcast_energy(foe,219,0,1,200));check(w.lightning(foe,target,*lightning(222,0),20,1000));
  const double hit=5*std::exp2((60.-84*.75)/40.);
  check(w.resources(2)->health==unsigned(std::ceil(1000-hit)));
  if(schedule==0)w.tick(4000);
  else if(schedule==1)w.tick(10000);
  else check(w.apply_ward_elements(source,*ward_elements_plan(0),10000));
  check(w.resources(2)->health==unsigned(std::ceil(1000-2*hit)));
 }
 std::cout<<"Ward175 survives player/NPC Watch Yourself armor reconstruction and delayed Lightning Strike222 armor resolution\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

