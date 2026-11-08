#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earthquake World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{170u,1086u})for(unsigned mode=0;mode<6;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=500;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{mode==3?1.:1000.,1000};
  d.damageable=true;d.earth_armor=60;
  d.knockdown_allowed=mode==1?std::nullopt:std::optional(mode!=2);d.knockdown_evidence="fixture";n.combat_definition=d;
  w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=5;++id){
   w.join(id,"Actor",id==1?0.f:id==3?752.f:id==4?753.f:500.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
   w.set_vitals(id,{1000,1000,100,100},100);check(w.combat_armor(*w.effect_scope(id),60));
  }
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),npc=*w.effect_scope(100);
  auto stale=target;++stale.generation;check(!w.earthquake(source,stale,*skills::effects::earthquake(skill,12),20,100));
  if(mode==4){
   // Selected foe moves 288 units during the cast; occupants must be collected
   // around its completion-time position, not the original destination.
   w.move(2,1000,0,100);
   check(w.earthquake(source,target,*skills::effects::earthquake(skill,12),20,1100));
   for(unsigned id:{2u,3u,4u})check(w.resources(id)->health==915&&w.knocked_down(*w.effect_scope(id),1100));
   check(w.npc_combat_snapshot(npc)->health->current==1000&&!w.knocked_down(npc,1100));
   continue;
  }
  if(mode==5){
   w.set_vitals(2,{0,1000,100,100},100);
   check(!w.earthquake(source,target,*skills::effects::earthquake(skill,12),20,100));
   check(w.resources(3)->health==1000&&w.npc_combat_snapshot(npc)->health->current==1000);
   continue;
  }
  check(w.earthquake(source,target,*skills::effects::earthquake(skill,12),20,100)==(mode!=1));
  for(unsigned id:{1u,4u,5u})check(w.resources(id)->health==1000&&!w.knocked_down(*w.effect_scope(id),100));
  for(unsigned id:{2u,3u}){
   check(w.resources(id)->health==(mode==1?1000:915));
   check(w.knocked_down(*w.effect_scope(id),100)==(mode!=1));
  }
  const auto health=w.npc_combat_snapshot(npc)->health->current;
  check(mode==1?health==1000:mode==3?health==0:std::abs(health-914.8)<1e-9);
  check(w.knocked_down(npc,100)==(mode==0));
  if(mode!=1){check(w.knocked_down(target,2099)&&!w.knocked_down(target,2100));}
 }
 std::cout<<"PASS Earthquake World target-centered252 boundary,hostility,all-population preflight,unknown vs immune control,lethal suppression and stale target\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
