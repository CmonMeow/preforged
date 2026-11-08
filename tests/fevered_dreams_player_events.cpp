#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered player events line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(unsigned mode=0;mode<6;++mode){world::Instance w;
  for(unsigned id:{1u,2u,3u,4u,5u}){w.join(id,"Actor",id==2?60:id==3?120:id==4?500:0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);w.set_vitals(id,{id==2&&mode==3?30u:500u,500,100,100},100);}
  const auto c=*w.effect_scope(1),a=*w.effect_scope(2),b=*w.effect_scope(3),far=*w.effect_scope(4),ally=*w.effect_scope(5);
  if(mode==1)check(w.apply_condition(c,a,Kind::blind,100,10000));
  check(w.apply_fevered_dreams(c,a,12,pvp,100,FeveredDazedSpan::attempted_duration));
  check(w.process_fevered_condition_events(100));
  if(mode==1){check(w.has_condition(a,Kind::dazed,100)==!pvp&&w.has_condition(b,Kind::dazed,100)==!pvp&&!w.has_condition(b,Kind::blind,100));continue;}
  if(mode==2)check(w.apply_fevered_dreams(c,b,12,pvp,100,FeveredDazedSpan::attempted_duration));
  if(mode==3){check(w.apply_condition(c,a,Kind::blind,100,10000));check(w.process_fevered_condition_events(100));check(w.degeneration_hex(c,a,*degeneration_hex(19,12),100));}
  else{check(w.apply_condition(c,a,Kind::blind,200,10000));check(w.process_fevered_condition_events(200));check(w.has_condition(b,Kind::blind,200)&&!w.has_condition(b,Kind::dazed,200));}
  check(w.apply_condition(c,a,Kind::poison,300,10000));
  if(mode>=4){const auto id=mode==4?1u:2u;w.leave(id);w.join(id,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},300,id);w.set_vitals(id,{500,500,100,100},100);
   check(w.process_fevered_condition_events(301)&&w.has_condition(b,Kind::poison,301));
   check(!w.apply_condition(c,a,Kind::weakness,400,10000)&&!w.has_condition(b,Kind::weakness,400)&&w.fevered_boundary_failures()==0);continue;
  }
  if(mode!=3||pvp)check(w.process_fevered_condition_events(301));
  check(w.process_fevered_condition_events(300));
  if(mode==3&&!pvp){check(w.resources(2)->health==0&&!w.has_condition(b,Kind::poison,300));continue;}
  check(w.has_condition(a,Kind::dazed,300)==!pvp&&w.has_condition(b,Kind::dazed,300)==!pvp&&w.has_condition(b,Kind::poison,300));
  for(auto target:{far,ally,c})check(!w.has_condition(target,Kind::poison,300)&&!w.has_condition(target,Kind::dazed,300));
  check(w.apply_condition(c,a,Kind::poison,400,20000)&&w.process_fevered_condition_events(400));
  check(w.fevered_boundary_failures()==0);
  check(!w.has_condition(b,Kind::poison,10300)); // renewal did not spread new20s timer
 }
 std::cout<<"PASS player-origin Fevered observation, initial Dazed-only area spread, owned-hex chains, PvP exclusion, lethal Fragility, silent renewals and same-time journal preservation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
