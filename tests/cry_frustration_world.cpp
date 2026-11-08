#include "gw_npc_catalog.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cry World line "+std::to_string(at.line()));}
int main()try{using namespace skills;using namespace effects;
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;for(unsigned id=1;id<=6;++id){float x=id==1?0:id==2?100:id==3?200:id==4?422:id==5?423:200;
   w.join(id,"Actor",x,0,std::make_shared<world::Outbox>(),0,{}, {},mode==5&&id==3?300:100,id==1||id==6?1:2);w.set_vitals(id,{500,500,0,100},100);}
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2),other=*w.effect_scope(3);
  if(mode!=1)check(w.cast_activity(target,1,100,mode==4?200:1100));
  check(w.cast_activity(other,28,100,1100));
  const auto before=w.control_epoch(other);
  if(mode==5)check(w.status_mutation(target,16,123,15,100,10000,true));
  if(mode==2){check(w.status_mutation(target,16,123,15,100,10000,true));check(w.status_mutation(other,17,124,15,100,10000,true));}
  auto selected=target;if(mode==3)++selected.generation;
  check(w.cry_frustration(caster,selected,*cry_frustration(12),20,200)==(mode!=3&&mode!=5));
  const bool triggered=mode==0||mode==2;
  for(unsigned id:{2u,3u,4u})check(w.resources(id)->health==(triggered?437u:500u));
  for(unsigned id:{1u,5u,6u})check(w.resources(id)->health==500);
  check((w.control_epoch(other)==before)==(mode!=0));
  if(mode==5)check(w.status_snapshot(target,200)->size()==1&&w.casting_skill(target,200)==1);
  if(mode==2){check(w.casting_skill(target,200)==1&&w.casting_skill(other,200)==28);check(w.resources(3)->energy==0);check(w.status_snapshot(target,200)->empty()&&w.status_snapshot(other,200)->empty());}
 }
 for(bool selected_npc:{false,true}){
  const std::string hash(64,'a');std::ostringstream a,m;a<<"gw-npcs 1\n";m<<"gw-npc-combat 2 "<<hash<<"\nsource test authored "<<hash<<" tests/cry_frustration_world.cpp\n";
  for(unsigned id:{100u,101u,102u}){a<<"npc 146 "<<id<<" 1 1234 0 1677721600 0 8 1 1 0 "<<(id-99)*100<<" 0 0 - -\n";
   m<<"npc 146 "<<id<<" 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n";}
  std::istringstream appearances(a.str()),metadata(m.str());auto population=world::load_npc_catalog(appearances);world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
  world::Instance w;w.configure_npcs(population.for_map(146));w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.join(3,"Bonded",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u,3u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),player=*w.effect_scope(2),bonded=*w.effect_scope(3),npc=*w.effect_scope(100),attacker=*w.effect_scope(101);
  check(w.cast_activity(player,28,100,1100));
  const auto spell=w.begin_npc_action(npc,combat::npcs::control::Action::spell,100,1100),attack=w.begin_npc_action(attacker,combat::npcs::control::Action::attack,100,1100);check(spell&&attack);
  check(w.elemental_armor(bonded,60,60));
  check(w.stage_npc_bonds_of_torment(attacker,bonded,*bonds_of_torment_npc(),150));
  check(w.cry_frustration(caster,selected_npc?npc:player,*cry_frustration(12),20,200));
  check(w.resources(2)->health==437&&!w.casting_skill(player,200));
  check(w.resources(3)->health==477); // Level-eight caster's cold hit loses 23 after armor.
  for(unsigned id:{100u,101u,102u})check(w.npc_combat_snapshot(*w.effect_scope(id))->health->current==437);
  check(!w.complete_npc_action(*spell,1100)&&!w.complete_npc_action(*attack,1100));
 }
 std::cout<<"PASS Cry player/NPC mixed area damage/control, initial idle/expiry gate, protection independence,322 boundary and scope rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
