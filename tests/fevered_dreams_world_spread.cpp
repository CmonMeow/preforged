#include "gw_npc_catalog.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered World spread line "+std::to_string(at.line()));}
int main()try{
 const std::string hash(64,'a');std::istringstream a("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\nnpc 146 101 1 1234 0 1677721600 0 8 1 1 0 120 0 0 - -\nnpc 146 102 1 1234 0 1677721600 0 8 1 1 0 500 0 0 - -\n");
 std::string metadata="gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/fevered_dreams_world_spread.cpp\n";
 for(unsigned id:{100u,101u,102u})metadata+="npc 146 "+std::to_string(id)+" 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n";
 std::istringstream m(metadata);auto population=world::load_npc_catalog(a);world::bind_combat_catalog(population,combat::npcs::load_catalog(m,hash),hash);
 for(bool pvp:{false,true})for(unsigned mode=0;mode<6;++mode){world::Instance w;auto definitions=population.for_map(146);if(mode==2)definitions[0].combat_definition->spawn_health=combat::Health{30,500};w.configure_npcs(definitions);
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{500,500,100,100},100);}
  const auto c=*w.effect_scope(1),t=*w.effect_scope(100),b=*w.effect_scope(101),far=*w.effect_scope(102),enemy=*w.effect_scope(2),ally=*w.effect_scope(3);
  if(mode==5){check(w.apply_condition(c,c,Kind::blind,100,10000));check(w.condition_transfer(c,t,*condition_transfer(154,12),100));}
  check(w.apply_fevered_dreams(c,t,12,pvp,100,FeveredDazedSpan::attempted_duration));
  if(mode==5){check(w.process_fevered_condition_events(100));check(w.has_condition(t,Kind::blind,100)&&w.has_condition(b,Kind::dazed,100)==!pvp&&w.has_condition(enemy,Kind::dazed,100)==!pvp&&!w.has_condition(b,Kind::blind,100));continue;}
  if(mode==1){check(w.apply_fevered_dreams(c,b,12,pvp,100,FeveredDazedSpan::attempted_duration));check(w.apply_fevered_dreams(c,enemy,12,pvp,100,FeveredDazedSpan::attempted_duration));}
  if(mode==2)check(w.degeneration_hex(c,t,*degeneration_hex(19,12),100));
  check(w.apply_condition(c,c,Kind::blind,100,10000)&&w.apply_condition(c,c,Kind::poison,100,10000));
  check(w.condition_transfer(c,t,*condition_transfer(154,12),200));
  if(mode==2){check(w.process_fevered_condition_events(200)&&w.npc_combat_snapshot(t)->health->current==0&&!w.has_condition(b,Kind::poison,200)&&!w.has_condition(enemy,Kind::poison,200));continue;}
  if(mode==3)check(w.shatter_delusions(c,t,*shatter_delusions(12),20,200));
  if(mode==4){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{500,500,100,100},100);}
  check(w.process_fevered_condition_events(201)); // producer already dispatched its complete batch
  check(w.process_fevered_condition_events(200));
  if(mode>=3){check(w.has_condition(b,Kind::poison,200)&&w.has_condition(enemy,Kind::poison,200)&&w.fevered_boundary_failures()==0);continue;}
  for(auto target:{t,b,enemy})check(w.has_condition(target,Kind::blind,200)&&w.has_condition(target,Kind::poison,200)&&w.has_condition(target,Kind::dazed,200)==!pvp);
  for(auto target:{far,ally,c})check(!w.has_condition(target,Kind::poison,200)&&!w.has_condition(target,Kind::dazed,200));
  check(w.fevered_boundary_failures()==0);
  const auto hp=w.resources(2)->health;check(w.process_fevered_condition_events(200)&&w.resources(2)->health==hp);
 }
 std::cout<<"PASS synchronous World NPC Fevered journal consumption, mixed recipients, PvP Dazed exclusion, radius/allegiance, same-time guard and consume-once behavior\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
