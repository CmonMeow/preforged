#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Rigor contact regression line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;
  w.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  auto box=std::make_shared<world::Outbox>();w.join(2,"Defender",60,0,box,0,{}, {},100,2);
  w.set_vitals(1,{400,500,100,100},100);w.set_vitals(2,{500,500,100,100},100);
  const auto attacker=*w.effect_scope(1),defender=*w.effect_scope(2);check(w.physical_armor(defender,60));
  const auto guardian=*skills::regeneration::reviewed(258,12);
  check(w.apply_health_regeneration(defender,2,guardian,100,6100,1248,true,defender));
  check(w.degeneration_hex(defender,attacker,*skills::effects::degeneration_hex(123,12),100));
  box->drain();check(w.degeneration_hex(attacker,defender,*skills::effects::degeneration_hex(137,0),100));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,137))==1);
  const auto effects=*w.status_snapshot(defender,100);
  const auto rigor=std::find_if(effects.begin(),effects.end(),[](const auto& entry){return entry.skill==137;});check(rigor!=effects.end());
  if(mode==1){w.configure_attack_miss_draws([]{return .5;});check(w.blindness(defender,attacker,*skills::effects::blindness(220,0),100));}
  if(mode==2)check(w.status_mutation(defender,137,rigor->instance,0,200,{},false));
  if(mode==3)check(w.apply_health_regeneration(defender,2,guardian,8000,14000,1248,true,defender));
  unsigned block_draws=0;w.configure_attack_block_draws([&]{++block_draws;return 0.;});
  combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(attacker,sword,20));
  check(w.melee_attack(attacker,defender,sword,*combat::weapon::attack_skill(322,0),mode==3?8200:300));
  using Outcome=combat::weapon::AttackOutcome;
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==1);
  check(events[0].outcome==(mode==0?Outcome::hit:mode==1?Outcome::miss:Outcome::block));
  check(block_draws==(mode>=2?1u:0u));
  check(w.resources(1)->health==(mode==0?361u:400u));
  check(w.resources(2)->health==(mode==0?470u:500u));
  check(bool(events[0].before_damage)==(mode==0));
 }
 std::cout<<"PASS actual owned World Rigor application,target248,Guardian bypass,Blindmiss,removal/expiry and Insidious hit qualification\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
