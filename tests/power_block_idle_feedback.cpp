#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Power Block idle feedback regression line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Target",mode==4?2000.f:60.f,0,box,0,{}, {},100,mode==3?1:2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
  if(mode==1)check(w.cast_activity(target,28,100,200)); // Already finished at impact.
  if(mode==2){
   combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};
   check(w.physical_armor(caster,60)&&w.ordinary_weapon(target,sword,20)&&w.ordinary_attack(target,1,100));
  }
  if(mode==5)check(w.kill(target,200));
  const auto epoch=w.control_epoch(target);box->drain();
  const bool valid=w.interrupt_skill(caster,target,*skills::effects::interrupt_skill(5,12),20,300);
  check(valid==(mode<3));
  check(w.skill_disabled_until(target,28,300)==0&&w.control_epoch(target)==epoch);
  const auto packets=box->drain();
  check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,5))==(mode<3?1:0));
 }
 std::cout<<"PASS Power Block visual feedback on valid non-casting targets without disable/control changes;invalid targets produce no effect\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
