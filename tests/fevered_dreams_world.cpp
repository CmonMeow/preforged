#include "gw_npc_catalog.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered World line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(unsigned mode=0;mode<5;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:mode==3?1300:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2&&mode==4?1:id);w.set_vitals(id,{500,500,100,100},100);}
  const auto c=*w.effect_scope(1),t=*w.effect_scope(2);
  if(mode==1)check(w.apply_condition(c,t,Kind::blind,100,10000));
  if(mode==2)check(w.hex_breaker(t,*hex_breaker(12),20,100));
  box->drain();const auto success=w.apply_fevered_dreams(c,t,12,pvp,200,FeveredDazedSpan::attempted_duration);
  check(success==(mode<3));if(!success)continue;
  if(mode==2){check(w.resources(1)->health==461&&!w.has_condition(t,Kind::dazed,200));continue;}
  check(w.has_condition(t,Kind::dazed,200)==(mode==1&&!pvp));
  auto statuses=*w.status_snapshot(t,200);auto hex=std::find_if(statuses.begin(),statuses.end(),[&](const auto& e){return e.skill==(pvp?3289:55);});
  check(hex!=statuses.end()&&hex->expires==(pvp?13200:22200));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,pvp?3289:55))==1);
  check(!w.status_mutation(t,55,999,12,200,22200,true));
  check(w.apply_fevered_dreams(c,t,0,!pvp,300,FeveredDazedSpan::attempted_duration));
  statuses=*w.status_snapshot(t,300);check(std::count_if(statuses.begin(),statuses.end(),[](const auto& e){return e.skill==55||e.skill==3289;})==1);
 }
 // Real player Dazed gain must pass through Fragility and one death cleanup.
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?500u:10u,500,100,100},100);}
  const auto c=*w.effect_scope(1),t=*w.effect_scope(2);check(w.apply_condition(c,t,Kind::blind,100,10000));
  check(w.degeneration_hex(c,t,*degeneration_hex(19,12),100));const auto epoch=*w.control_epoch(t);
  check(w.apply_fevered_dreams(c,t,12,false,200,FeveredDazedSpan::attempted_duration));
  check(w.resources(2)->health==0&&*w.control_epoch(t)==epoch+1&&!w.has_condition(t,Kind::dazed,200));
 }
 const std::string hash(64,'a');std::istringstream a("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\n");std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/fevered_dreams_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");auto pop=world::load_npc_catalog(a);world::bind_combat_catalog(pop,combat::npcs::load_catalog(m,hash),hash);
 world::Instance w;w.configure_npcs(pop.for_map(146));auto box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
 const auto c=*w.effect_scope(1),t=*w.effect_scope(100);check(w.apply_fevered_dreams(c,t,12,false,100,FeveredDazedSpan::attempted_duration));check(w.npc_combat_snapshot(t)->effects&0x800);
 const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(100,1,55))==1);
 std::cout<<"PASS World Fevered application stage: native body90, player/NPC registry/timer, alias replacement, Dazed, hostility/range/protection and raw insertion guard\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
