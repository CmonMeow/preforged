#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Projectile outcome regression line "+std::to_string(at.line()));}
void setup(world::Instance& w){
 w.configure_attack_miss_draws([]{return .5;});
 for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,20,20},100);}
 check(w.combat_armor(*w.effect_scope(2),60)&&w.physical_armor(*w.effect_scope(2),60)&&w.configure_projectiles({100,32}));
}
int main()try{using combat::weapon::AttackOutcome;
 for(unsigned mode:{0u,1u,2u,3u,4u}){
  world::Instance w;setup(w);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(a,wand,20)&&w.ordinary_attack(a,2,100));w.tick(600);check(w.take_attack_outcomes(a).empty());
  if(mode==1)check(w.apply_condition(b,a,combat::conditions::Kind::blind,800,5000));
  if(mode==2)w.move(2,500,0,700);
  if(mode==3){w.leave(2);w.join(2,"New target",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,20,20},100);}
  if(mode==4){++wand.revision;check(w.ordinary_weapon(a,wand,20));}
  w.tick(1600);w.cancel_ordinary_attack(a);const auto events=w.take_attack_outcomes(a);
  const auto expected=mode==0?AttackOutcome::hit:mode==1?AttackOutcome::miss:mode==2?AttackOutcome::dodge:mode==3?AttackOutcome::target_retired:AttackOutcome::cancelled;
  check(events.size()==1&&events[0].outcome==expected&&events[0].event&&events[0].released==600&&events[0].at==1600&&events[0].attacker==a&&events[0].target==b);
  check(events[0].before_damage==(mode==0?std::optional<std::uint64_t>{1600}:std::nullopt));
  check(w.resources(2)->health==(mode==0?480u:500u));w.tick(1700);check(w.take_attack_outcomes(a).empty());
 }
 {
  world::Instance w;setup(w);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);combat::weapon::Owned bow{1,1,{20,20,1},12,false,false,5};
  check(w.ordinary_weapon(a,bow,20)&&w.melee_attack(a,b,bow,*combat::weapon::attack_skill(396,0),100));
  w.tick(200);const auto events=w.take_attack_outcomes(a);
  check(events.size()==2&&events[0].event!=events[1].event&&events[0].outcome==AttackOutcome::hit&&events[1].outcome==AttackOutcome::hit);
  check(events[0].at==140&&events[1].at==140&&events[0].before_damage==140&&events[1].before_damage==140);
  check(w.resources(2)->health==470);w.tick(300);check(w.take_attack_outcomes(a).empty());
 }
 {
  world::Instance w;setup(w);const auto old=*w.effect_scope(1);combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(old,wand,20)&&w.ordinary_attack(old,2,100));w.tick(600);w.leave(1);
  w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,20,20},100);
  w.tick(1600);check(w.take_attack_outcomes(*w.effect_scope(1)).empty()&&w.resources(2)->health==500);
 }
 std::cout<<"PASS production projectile outcome IDs,hit/miss/dodge/retire/cancel,dual-arrow identity and lifetime isolation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
