#include "gw_npc_catalog.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Panic use line "+std::to_string(at.line()));}
int main()try{using namespace skills;using namespace effects;
 for(unsigned mode=0;mode<8;++mode){
  std::cout<<"mode="<<mode<<"\n";
  const std::string hash(64,'a');std::ostringstream a,m;a<<"gw-npcs 1\n";m<<"gw-npc-combat 2 "<<hash<<"\nsource test authored "<<hash<<" tests/panic_world_use.cpp\n";
  for(unsigned id:{100u,101u,102u}){int x=id==100?0:id==101?252:253;a<<"npc 146 "<<id<<" 1 1234 0 1677721600 0 8 1 1 0 "<<x<<" 0 0 - -\n";m<<"npc 146 "<<id<<" 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n";}
  std::istringstream appearances(a.str()),metadata(m.str());auto population=world::load_npc_catalog(appearances);world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
  world::Instance w;w.configure_npcs(population.for_map(146));
  for(unsigned id=1;id<=6;++id){if(mode==3&&id==3)continue;float x=id==1?-100:id==3?252:id==4?253:0;w.join(id,"Actor",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto caster=*w.effect_scope(1),trigger=*w.effect_scope(mode==6?100:2);
  if(mode!=4)check(w.apply_panic(caster,trigger,12,100));
  if(mode==3){w.join(3,"Future",252,0,std::make_shared<world::Outbox>(),0,{}, {},300,2);w.set_vitals(3,{500,500,100,100},100);}
  for(unsigned id:{2u,3u,4u,5u})check(w.cast_activity(*w.effect_scope(id),281,100,10100));
  check(w.status_mutation(*w.effect_scope(6),16,123,15,100,10100,true)); // Idle foe's protection must not be consumed.
  if(mode==1||mode==3)check(w.status_mutation(*w.effect_scope(3),16,124,15,100,10100,true));
  const auto n0=w.begin_npc_action(*w.effect_scope(100),combat::npcs::control::Action::spell,100,3100);
  const auto n1=w.begin_npc_action(*w.effect_scope(101),combat::npcs::control::Action::attack,100,3100);
  const auto n2=w.begin_npc_action(*w.effect_scope(102),combat::npcs::control::Action::spell,100,3100);check(n0&&n1&&n2);
  if(mode==5){w.leave(1);w.join(1,"Replacement",-100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);}
  PanicUse use{trigger,281,1,150,mode==7?8100u:200u,mode!=2};
  check(w.panic_skill_used(use)==(mode!=3&&mode!=5));
  if(mode==3||mode==5){check(w.casting_skill(*w.effect_scope(3),200)==281&&w.resources(1)->health==500);if(mode==3)check(w.status_snapshot(*w.effect_scope(3),300)->size()==1);check(w.complete_npc_action(*n1,3100));continue;}
  if(mode==4){check(w.apply_panic(caster,trigger,12,201));use.used=202;check(!w.panic_skill_used(use));check(w.casting_skill(*w.effect_scope(3),202)==281);continue;}
  const bool reacts=mode==0||mode==1||mode==6;
  check(bool(w.casting_skill(*w.effect_scope(3),use.used))==(!reacts||mode==1));
  check(w.casting_skill(*w.effect_scope(4),use.used)==281&&w.casting_skill(*w.effect_scope(5),use.used)==281);
  const auto idle_status=*w.status_snapshot(*w.effect_scope(6),use.used);check(std::any_of(idle_status.begin(),idle_status.end(),[](const auto& e){return e.skill==16;}));
  if(mode!=6)check(w.casting_skill(*w.effect_scope(2),use.used)==281);
  check(!w.panic_skill_used(use));
  if(mode==1){check(w.status_snapshot(*w.effect_scope(3),200)->size()==1);++use.event;++use.used;check(w.panic_skill_used(use)&&!w.casting_skill(*w.effect_scope(3),201));}
  for(unsigned id=1;id<=6;++id)check(w.resources(id)->health==500);
  check(w.complete_npc_action(*n0,11000)==(!reacts||mode==6));check(w.complete_npc_action(*n1,11000)==!reacts);check(w.complete_npc_action(*n2,11000));
 }
 std::cout<<"PASS Panic trusted player/NPC uses, other-foe control/protection, no damage/self interruption, replay/expiry/caster lifetime and atomic future-recipient guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


