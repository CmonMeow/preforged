#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Melee outcome regression line "+std::to_string(at.line()));}
void setup(world::Instance& w,float target_x=60){
 w.configure_attack_miss_draws([]{return .5;});
 for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?target_x:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
 check(w.physical_armor(*w.effect_scope(2),60));
}
int main()try{using combat::weapon::AttackOutcome;
 for(bool attack_skill:{false,true})for(bool missed:{false,true}){
  world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);
  check(w.apply_price_of_failure(caster,attacker,12,100));
  if(missed)check(w.apply_condition(target,attacker,combat::conditions::Kind::blind,100,5000));
  combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(attacker,sword,20));
  if(attack_skill)check(w.melee_attack(attacker,target,sword,*combat::weapon::attack_skill(322,0),600));
  else{check(w.ordinary_attack(attacker,2,100));w.tick(600);w.cancel_ordinary_attack(attacker);}
  const auto events=w.take_attack_outcomes(attacker);
  std::cout<<"skill="<<attack_skill<<" miss="<<missed<<" Health="<<w.resources(1)->health<<" outcomes="<<events.size()<<'\n';
  check(w.resources(1)->health==(missed?463u:500u)&&w.resources(2)->health==(missed?500u:attack_skill?470u:480u));
  check(events.size()==1&&events[0].outcome==(missed?AttackOutcome::miss:AttackOutcome::hit)&&events[0].released==600&&events[0].at==600);
  check(events[0].before_damage==(missed?std::nullopt:std::optional<std::uint64_t>{600}));
  w.tick(700);check(w.take_attack_outcomes(attacker).empty()&&w.resources(1)->health==(missed?463u:500u));
 }
 {
  world::Instance w;setup(w,2000);const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);
  check(w.apply_price_of_failure(caster,attacker,12,100));combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};
  check(w.ordinary_weapon(attacker,sword,20)&&!w.melee_attack(attacker,target,sword,*combat::weapon::attack_skill(322,0),600));
  check(w.take_attack_outcomes(attacker).empty()&&w.resources(1)->health==500);
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
