#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("attack admission line "+std::to_string(at.line()));}
int main()try{
 const auto plan=*combat::weapon::attack_skill(359,12,0);
 for(bool npc:{false,true})for(unsigned mode=0;mode<11;++mode){
  warrior_test::Fixture f(359,npc);check(f.condition(combat::conditions::Kind::weakness,1000,5000));
  auto permit=f.world.prepare_melee_attack(f.source,f.target,f.weapon,plan,1000);check(bool(permit)&&permit->open());
  check(!f.world.melee_attack(f.source,f.target,f.weapon,plan,1500)); // no World capability
  auto target=f.target;auto gear=f.weapon;auto attack=plan;std::uint64_t at=1500;
  if(mode==0)permit->cancel();
  if(mode==1)++target.generation;
  if(mode==2)++gear.damage.maximum;
  if(mode==3)attack=*combat::weapon::attack_skill(359,11,0);
  if(mode==4)at=1499;
  if(mode==5)at=91001;
  if(mode==6){combat::weapon::AttackLease lease(permit);}
  if(mode==7){f.world.leave(1);f.world.join(1,"Replacement",0,0,f.box,0,{}, {},1400,1);f.world.set_vitals(1,{1000,1000,30,30},100);check(f.world.ordinary_weapon(*f.world.effect_scope(1),gear,20));}
  if(mode==8){check(f.world.attack_activity(f.source,1500,1000));check(f.world.interrupt_attack(f.source,1100));}
  if(mode==9){
   f.world.join(3,"Foe",30,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.world.set_vitals(3,{1000,1000,30,30},100);
   check(f.world.physical_armor(f.source,60)&&f.world.ordinary_weapon(*f.world.effect_scope(3),gear,20));
   check(f.world.melee_attack(*f.world.effect_scope(3),f.source,gear,*combat::weapon::attack_skill(331,0),1100));at=5000; // even after recovery, old control epoch is retired
  }
  if(mode==10){check(f.world.kill(f.source,1100)&&f.world.resurrect(f.source,1200));at=1500;}
  const auto before=f.health();check(!f.world.melee_attack(f.source,target,gear,attack,at,permit)&&f.health()==before);
  if(mode>=1&&mode<=4){check(permit->open());check(f.world.melee_attack(f.source,f.target,f.weapon,plan,1500,permit));check(!permit->open());}
 }
 for(bool npc:{false,true}){
  warrior_test::Fixture f(359,npc);
  check(f.world.status_mutation(f.source,346,7,0,1000,9000,true));
  const auto permit=f.world.prepare_melee_attack(f.source,f.target,f.weapon,plan,1000);
  check(bool(permit)&&permit->release_time()==1335); // Frenzy duration500*.67, rounded upward
  check(!f.world.melee_attack(f.source,f.target,f.weapon,plan,1334,permit));
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,1335,permit));
 }
 std::cout<<"PASS World-only Heavy Blow capabilities,earliest contact,exact equipment/plan/lifetimes,cancel/interrupt/control epochs,expiry,single use and Frenzy timing\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
