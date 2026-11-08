#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Guardian reactive-hit regression line "+std::to_string(at.line()));
}
int main()try{
 for(bool blocked:{true,false}){
  world::Instance w;
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0.f:id==2?60.f:100.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?400u:500u,500,100,100},100);
  }
  const auto source=*w.effect_scope(1),defender=*w.effect_scope(2);
  check(w.physical_armor(defender,60));
  check(w.degeneration_hex(defender,source,*skills::effects::degeneration_hex(123,12),100));
  check(w.degeneration_hex(source,defender,*skills::effects::degeneration_hex(101,12),100));
  check(w.degeneration_hex(source,defender,*skills::effects::degeneration_hex(150,12),100));
  const auto guardian=*skills::regeneration::reviewed(258,12);
  check(w.apply_health_regeneration(defender,2,guardian,100,6100,1248,true,defender));
  w.configure_attack_block_draws([=]{return blocked?0.:.9;});
  combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};
  check(w.ordinary_weapon(source,sword,20)&&w.ordinary_attack(source,2,200));
  w.tick(700);w.cancel_ordinary_attack(source);
  check(w.resources(1)->health==(blocked?400u:361u));
  check(w.resources(2)->health==(blocked?500u:468u));
  check(w.resources(3)->health==(blocked?500u:466u));
  const auto events=w.take_attack_outcomes(source);check(events.size()==1);
  check(events[0].outcome==(blocked?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));
  check(bool(events[0].before_damage)==!blocked);
  w.tick(701);check(w.take_attack_outcomes(source).empty());
  check(w.resources(1)->health==(blocked?400u:361u)&&w.resources(3)->health==(blocked?500u:466u));
 }
 std::cout<<"PASS Guardian block prevents Insidious pre-hit steal,Barbs damage and Mark collateral;unblocked control triggers all exactly once\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
