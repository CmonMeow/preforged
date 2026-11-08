#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Ward176 World line "+std::to_string(at.line()));}
int main()try{
 using Outcome=combat::weapon::AttackOutcome;
 // Actual player skill/ordinary/Cyclone contacts preserve misses and bypasses.
 for(unsigned mode=0;mode<8;++mode){warrior_test::Fixture f(mode==6?330:340,false);auto&w=f.world;
  check(w.apply_ward_melee(f.target,*ward_melee_plan(0),100));
  if(mode==5){skills::Slots bar{};bar[0]={321,0};check(w.configure_adrenaline(f.source,bar,100));f.weapon.item_type=2;f.weapon.revision=2;check(w.ordinary_weapon(f.source,f.weapon,20,100));}
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return mode==1?.9:.1;});
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==3)check(w.status_mutation(f.target,137,124,0,100,8100,true));
  const unsigned at=mode==4?5100:300;
  if(mode==7){check(w.ordinary_attack(f.source,2,200));w.tick(700);}
  else if(!w.melee_attack(f.source,mode==6?f.source:f.target,f.weapon,*combat::weapon::attack_skill(mode==6?330:mode==5?321:340,mode==5?0:12),at))
   throw std::runtime_error("Ward176 player melee admission mode "+std::to_string(mode));
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1);
  const auto expected=mode==2?Outcome::miss:(mode==1||mode==3||mode==4||mode==5)?Outcome::hit:Outcome::block;
  check(events[0].outcome==expected);check(draws==unsigned(!(mode==2||mode==3||mode==4||mode==5)));
  if(expected!=Outcome::hit)check(f.health()==1000);
 }
 {warrior_test::Fixture f(426,false);auto&w=f.world;check(w.apply_ward_melee(f.target,*ward_melee_plan(0),100));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});
   check(w.ordinary_attack(f.source,2,200));w.tick(700);w.tick(2000);const auto events=w.take_attack_outcomes(f.source);
  check(events.size()==1&&events[0].outcome==Outcome::hit&&draws==0&&f.health()<1000);
 }
 // Weapon of Warding and a separate caster's Ward may share numeric ID1.
 for(unsigned mode=0;mode<6;++mode){warrior_test::Fixture f(340,false);auto&w=f.world;
  if(mode==5){skills::Slots bar{};bar[0]={321,0};check(w.configure_adrenaline(f.source,bar,100));f.weapon.item_type=2;f.weapon.revision=2;check(w.ordinary_weapon(f.source,f.weapon,20,100));}
  w.join(3,"Ward caster",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);
  const auto caster=*w.effect_scope(3);check(w.apply_ward_melee(caster,*ward_melee_plan(0),100));
  check(w.apply_weapon_spell(f.target,f.target,*weapon_spell(793,0,8,0),100));
  const auto entries=w.status_snapshot(f.target,100);check(entries&&entries->size()==1&&entries->front().instance==w.ward_melee_source(f.target,100)->instance);
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return mode==0?.1:mode==1?.6:.9;});
  if(mode==3)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==4)check(w.status_mutation(f.target,137,124,0,100,8100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(mode==5?321:340,mode==5?0:12),300));
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1);
  check(events[0].outcome==(mode==3?Outcome::miss:mode<2?Outcome::block:Outcome::hit));check(draws==unsigned(mode<3));
  check(w.status_snapshot(f.target,300)->size()==unsigned(mode==4?2:1));
 }
 // A Ward-attributed block must not charge Distortion or reward Bonetti.
 for(unsigned stance:{11u,380u,372u}){warrior_test::Fixture f(340,false);auto&w=f.world;
  check(w.apply_ward_melee(f.target,*ward_melee_plan(0),100));
  skills::Slots bar{};bar[0]={stance,0};check(w.configure_adrenaline(f.target,bar,100));
  const auto duration=stance==11?distortion(12)->duration:stance==380?bonetti_defense(12)->duration:gladiator_defense(12)->duration;
  check(w.status_mutation(f.target,stance,123,12,100,100+duration,true));
  w.configure_attack_block_draws([&]{return .8;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].outcome==Outcome::block);
  check(w.resources(2)->energy==30&&f.health()==1000&&w.resources(1)->health==1000);
 }
 // Contact queries sample the already-moving recipient at the event time.
 {warrior_test::Fixture f(340,false);auto&w=f.world;check(w.apply_ward_melee(f.target,*ward_melee_plan(0),100));
  w.move(1,400,0,200);w.move(2,460,0,200);unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  check(w.take_attack_outcomes(f.source).front().outcome==Outcome::block&&draws==1);
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),1500));
  check(w.take_attack_outcomes(f.source).front().outcome==Outcome::hit&&draws==1);
 }
 {warrior_test::Fixture f(340,false);auto&w=f.world;check(w.apply_ward_melee(f.target,*ward_melee_plan(0),100));
  w.move(2,200,0,4900);unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),5100));
  check(w.take_attack_outcomes(f.source).front().outcome==Outcome::hit&&draws==0);
 }
 {world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=3;++id){w.join(id,"Actor",id==3?400:0,0,box,0,{}, {},100,id==3?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto caster=*w.effect_scope(1),ally=*w.effect_scope(2),enemy=*w.effect_scope(3);
  check(w.apply_ward_melee(caster,*ward_melee_plan(0),100));const auto source=*w.ward_melee_source(ally,100);
  check(source.skill==176&&source.owner==caster&&!w.ward_melee_source(enemy,100));
  check(w.apply_ward_melee(ally,*ward_melee_plan(20),100));check(w.ward_melee_source(ally,100)->owner==caster);
  w.move(2,400,0,200);w.tick(2000);check(!w.ward_melee_source(ally,2000));
  w.move(2,0,0,2100);w.tick(4000);check(w.ward_melee_source(ally,4000).has_value());
  w.leave(1);w.join(1,"Reused actor",400,0,box,0,{}, {},4100,1);w.set_vitals(1,{1000,1000,100,100},100);
  check(w.ward_melee_source(ally,4100)->owner==caster); // Stationary retained-lifetime private policy.
  check(w.ward_melee_source(ally,5099)->owner==caster);check(w.ward_melee_source(ally,5100)->owner==ally);
  check(!w.ward_melee_source(ally,25100));
  world::Instance other;other.join(2,"Other",0,0,box,0,{}, {},100,1);other.set_vitals(2,{1000,1000,100,100},100);
  check(!w.ward_melee_source(*other.effect_scope(2),4000));
 }
 std::cout<<"Ward176 player contacts, ordinary/Cyclone, attribution/no reactive charge, overlap, entry/exit and half-open lifetime passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
