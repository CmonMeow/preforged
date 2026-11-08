#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Guardian World regression line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Attacker",0,0,box,0,{}, {},100,1);
  w.join(2,"Defender",mode==3?900.f:60.f,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.physical_armor(target,60));
  const auto guardian=*skills::regeneration::reviewed(258,12);
  check(w.apply_health_regeneration(target,2,guardian,100,6100,1248,true,target));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return mode==5?1.:0.;});
  combat::weapon::Owned weapon{1,1,{20,20,2},12,false,false,mode==2?2u:mode==3?5u:27u};
  check(w.ordinary_weapon(source,weapon,20));
  const unsigned at=mode==4?6200:200;
  if(mode==0||mode==4){check(w.ordinary_attack(source,2,at));w.tick(at+500);w.cancel_ordinary_attack(source);}
  else if(mode==3){check(w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(394,12),at));w.tick(at+360);}
  else if(mode==2)check(w.melee_attack(source,source,weapon,*combat::weapon::attack_skill(330,12),at));
  else {const bool accepted=w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(322,0),at);check(accepted==(mode!=5));}
  check(w.resources(2)->health==(mode==4?480u:500u));
  check(draws==(mode==4?0u:1u));
  const auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==1);
  const auto expected=mode==4?combat::weapon::AttackOutcome::hit:mode==5?combat::weapon::AttackOutcome::cancelled:combat::weapon::AttackOutcome::block;
  check(outcomes[0].outcome==expected);
  check(bool(outcomes[0].before_damage)==(mode==4));
 }
 std::cout<<"PASS staged Guardian player defense blocks ordinary,melee,Cyclone and fixed-damage bow impacts without hit qualification;expiry and invalid draws checked;casting/NPC defense/rendering pending\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
