#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered deadline line "+std::to_string(at.line()));}
int main()try{
 for(unsigned extra:{0u,1u,100u,2000u,4000u})for(bool reverse:{false,true})for(bool tick:{false,true}){world::Instance w;
  const auto root=reverse?3u:2u,neighbor=reverse?2u:3u;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==root?60:120,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto c=*w.effect_scope(1),a=*w.effect_scope(root),b=*w.effect_scope(neighbor);
  check(w.apply_fevered_dreams(c,a,12,false,100,FeveredDazedSpan::attempted_duration));
  check(w.degeneration_hex(c,b,*degeneration_hex(2998,12),100));
  check(w.status_mutation(a,37,100,0,100,5100,true));
  const auto now=5100+extra;if(tick)w.tick(now);else{check(w.settle_resources(c,now)&&w.settle_resources(a,now)&&w.settle_resources(b,now));}
  check(w.has_condition(a,Kind::crippled,now)==(extra<3000)&&w.has_condition(b,Kind::crippled,now)==(extra<3000));
  check(w.resources(neighbor)->health==(extra<3000?483u:466u)&&w.fevered_boundary_failures()==0);
 }
 {world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:120,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto c=*w.effect_scope(1),a=*w.effect_scope(2),b=*w.effect_scope(3);check(w.apply_fevered_dreams(c,a,12,false,100,FeveredDazedSpan::attempted_duration)&&w.status_mutation(a,37,100,0,100,5100,true));
  w.join(4,"Late arrival",120,0,std::make_shared<world::Outbox>(),0,{}, {},5200,2);w.set_vitals(4,{500,500,100,100},100);
  check(w.has_condition(b,Kind::crippled,5200)&&!w.has_condition(*w.effect_scope(4),Kind::crippled,5200)&&w.fevered_boundary_failures()==0);
 }
 std::cout<<"PASS global Fevered expiry boundaries before actor/movement advancement, both iteration orders, delayed tick gain/loss Fragility and no historical replay failure\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
