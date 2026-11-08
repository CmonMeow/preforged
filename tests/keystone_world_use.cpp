#include "gw_npc_catalog.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Keystone use line "+std::to_string(at.line()));
}
int main()try{
 for(bool reversal:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float((id-1)*60),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),side=*w.effect_scope(3);
  check(w.keystone_signet(source,*keystone_signet(12),{},*w.disable_reset_revision(source),1,200));
  check(w.status_mutation(side,1399,80,12,200,6200,true));if(reversal)check(w.status_mutation(side,307,81,12,200,8200,true));
  check(w.cast_activity(side,28,300,1100));const auto epoch=w.control_epoch(side);
  check(w.keystone_skill_used({source,target,58,1,400,true,true},20));check(w.resources(3)->health==(reversal?500u:454u));
  check(!w.casting_skill(side,400)&&w.control_epoch(side)!=epoch);
  check(w.keystone_skill_used({source,target,58,2,401,true,true},20));check(w.resources(3)->health==(reversal?459u:413u));
 }

 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;for(unsigned id=1;id<=8;++id){float x=id==1?0:id==2?100:id==4?266:id==5?267:200;
   w.join(id,"Actor",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==6?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2),active=*w.effect_scope(3),protected_actor=*w.effect_scope(8);
  check(w.keystone_signet(owner,*keystone_signet(mode==3?2:12),{},*w.disable_reset_revision(owner),1,200));
  check(w.cast_activity(active,28,300,1100)&&w.cast_activity(protected_actor,28,300,1100));
  check(w.status_mutation(protected_actor,mode==4?17:16,123,15,300,10000,true));check(w.kill(*w.effect_scope(7),300));
  if(mode==1)check(w.kill(target,350));
  const auto epoch=w.control_epoch(active),protected_epoch=w.control_epoch(protected_actor);
  KeystoneUse use{owner,target,58,1,400,mode!=2,true};
  if(mode==0){auto invalid=use;++invalid.target.generation;check(!w.keystone_skill_used(invalid,20));check(w.keystone_snapshot(owner,400)->remaining==5&&w.resources(3)->health==500);}
  check(w.keystone_skill_used(use,20));
  if(mode==2){check(w.resources(3)->health==500&&w.keystone_snapshot(owner,400)->remaining==5);use.event=2;use.success=true;check(w.keystone_skill_used(use,20));}
  const unsigned damage=mode==3?21:51;
  for(unsigned id:{3u,4u,8u})check(w.resources(id)->health==500-damage);
  for(unsigned id:{1u,5u,6u})check(w.resources(id)->health==500);
  check(w.resources(2)->health==(mode==1?0:500)&&w.resources(7)->health==0);
  check(w.control_epoch(active)!=epoch&&w.control_epoch(protected_actor)==protected_epoch);
  if(mode==4)check(!w.status_snapshot(protected_actor,400)->empty()&&w.resources(8)->energy<100);
  else check(w.status_snapshot(protected_actor,400)->empty());
  check(!w.keystone_skill_used(use,20)&&w.resources(3)->health==500-damage);
  if(mode==3)check(!w.keystone_snapshot(owner,400)&&w.status_snapshot(owner,400)->empty());
  else check(w.keystone_snapshot(owner,400)->remaining==4);
 }
 {world::Instance w;
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",float(id*50-50),0,std::make_shared<world::Outbox>(),0,{}, {},id==4?800:100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2),protected_actor=*w.effect_scope(3);
  check(w.keystone_signet(owner,*keystone_signet(12),{},*w.disable_reset_revision(owner),1,200));
  check(w.cast_activity(protected_actor,28,300,1300)&&w.status_mutation(protected_actor,16,123,15,300,10000,true));
  check(!w.keystone_skill_used({owner,target,58,1,600,true,false},20));
  check(w.keystone_snapshot(owner,600)->remaining==5&&w.resources(3)->health==500&&w.casting_skill(protected_actor,600)==28&&w.status_snapshot(protected_actor,600)->size()==1);
  check(w.keystone_skill_used({owner,target,58,1,900,true,false},20));
  check(w.keystone_snapshot(owner,900)->remaining==4&&w.resources(3)->health==449&&w.resources(4)->health==449&&w.status_snapshot(protected_actor,900)->empty());
 }
 {world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*50-50),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
  check(!w.status_mutation(owner,63,9000,12,100,20100,true));
  check(w.keystone_signet(owner,*keystone_signet(12),{},*w.disable_reset_revision(owner),1,200));
  const auto effect=w.keystone_snapshot(owner,200)->effect;
  check(!w.status_mutation(owner,16,effect,12,300,10000,true)&&w.keystone_snapshot(owner,300));
  check(w.status_mutation(owner,63,effect,12,300,{},false)&&!w.keystone_snapshot(owner,300));
  check(w.keystone_skill_used({owner,target,58,1,400,true,false},20)&&w.resources(3)->health==500);
 }
 const std::string hash(64,'a');std::ostringstream a,m;a<<"gw-npcs 1\n";m<<"gw-npc-combat 2 "<<hash<<"\nsource test authored "<<hash<<" tests/keystone_world_use.cpp\n";
 for(unsigned id:{100u,101u,102u}){a<<"npc 146 "<<id<<" 1 1234 0 1677721600 0 8 1 1 0 "<<(id-99)*100<<" 0 0 - -\n";
  m<<"npc 146 "<<id<<" 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n";}
 std::istringstream appearances(a.str()),metadata(m.str());auto population=world::load_npc_catalog(appearances);world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
 world::Instance w;w.configure_npcs(population.for_map(146));w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
 w.join(2,"Bonded",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);
 const auto owner=*w.effect_scope(1),selected=*w.effect_scope(100),neighbor=*w.effect_scope(101),bonded=*w.effect_scope(2);
 check(w.elemental_armor(bonded,60,60));
 check(w.keystone_signet(owner,*keystone_signet(12),{},*w.disable_reset_revision(owner),1,200));
 check(w.stage_npc_bonds_of_torment(neighbor,bonded,*bonds_of_torment_npc(),250));
 const auto action=w.begin_npc_action(neighbor,combat::npcs::control::Action::spell,300,1100);check(bool(action));
 check(w.keystone_skill_used({owner,selected,58,1,400,true,true},20));
 check(w.npc_combat_snapshot(selected)->health->current==500&&w.npc_combat_snapshot(neighbor)->health->current==449&&w.npc_combat_snapshot(*w.effect_scope(102))->health->current==500);
 check(w.resources(2)->health==482); // Level-eight caster's cold hit is armor-reduced.
 check(!w.complete_npc_action(*action,1100));
 std::cout<<"PASS Keystone player/NPC other-foe damage,166 boundary, protected interruption, dead primary, exhaustion and event replay\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
