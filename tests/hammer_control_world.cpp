#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("hammer control World line "+std::to_string(at.line()));}
int main()try{
 using enum combat::conditions::Kind;
 for(unsigned skill:{355u,357u,358u,359u,360u})for(bool npc:{false,true})for(bool start:{false,true})for(bool contact:{false,true})for(bool attacking:{false,true})for(bool miss:{false,true}){
  warrior_test::Fixture f(skill,npc);const auto plan=*combat::weapon::attack_skill(skill,12,0);
  if(start)check(f.condition(weakness,1000,contact?5000:100));
  const auto permit=skill==359?f.world.prepare_melee_attack(f.source,f.target,f.weapon,plan,1000):combat::weapon::AttackPermit{};
  if(skill==359){check(bool(permit)&&permit->release_time()==1500);check(!f.world.melee_attack(f.source,f.target,f.weapon,plan,1499,permit)&&permit->open());}
  if(!start&&contact)check(f.condition(weakness,1200,5000));
  if(attacking){
   if(npc)check(bool(f.world.begin_npc_action(f.target,combat::npcs::control::Action::attack,1200,5000)));
   else check(f.world.attack_activity(f.target,5000,1200));
  }
  if(miss)check(f.world.apply_condition(f.source,f.source,blind,1200,5000));
  f.world.tick(1500);const auto before=f.health();f.box->drain();
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,1500,permit));
  check(f.health()==before-(miss?0:20+(skill==358?16:skill==359&&start?24:0)));
  const bool down=!miss&&(skill==355||skill==358||(skill==359&&contact)||(skill==357&&attacking));
  check(f.world.knocked_down(f.target,1500)==down);
  if(down)check(f.world.knocked_down(f.target,3499)&&!f.world.knocked_down(f.target,3500));
  if(!miss&&(skill==355||skill==360))check(f.world.has_condition(f.target,weakness,18499)&&!f.world.has_condition(f.target,weakness,18500));
  if(skill==355){const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(f.target.actor,f.source.actor,355))==unsigned(!miss));}
  if(permit){const auto after=f.health();check(!permit->open()&&!f.world.melee_attack(f.source,f.target,f.weapon,plan,1500,permit)&&f.health()==after);}
 }
 for(bool npc:{false,true})for(unsigned strength:{7u,8u}){
  warrior_test::Fixture f(358,npc);check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(358,12,strength),1000));
  const auto end=strength==8?5000u:3000u;check(f.world.knocked_down(f.target,end-1)&&!f.world.knocked_down(f.target,end));
 }
 for(unsigned skill:{355u,357u,358u,359u,360u})for(unsigned mode=0;mode<3;++mode){
  warrior_test::Fixture f(skill,true,1000,mode==2?std::optional<bool>{}:std::optional<bool>{false},mode!=1);
  check(f.condition(weakness,1000,5000)==(mode!=1));
  const auto plan=*combat::weapon::attack_skill(skill,12,0);
  const auto permit=skill==359?f.world.prepare_melee_attack(f.source,f.target,f.weapon,plan,1000):combat::weapon::AttackPermit{};
  const bool expected=mode!=2||skill==360;
  check(f.world.melee_target(f.source,f.target.actor,1000,skill)==expected);
  const auto before=f.health();check(f.world.melee_attack(f.source,f.target,f.weapon,plan,1500,permit)==expected);
  check(!f.world.knocked_down(f.target,1500));if(!expected)check(f.health()==before);
  if(skill==355||skill==360)check(f.world.has_condition(f.target,weakness,10000)==(expected&&mode!=1));
 }
 // A block stops both damage and knockdown/Weakness; no reaction is invented.
 for(unsigned skill:{355u,357u,358u,359u,360u}){
  warrior_test::Fixture f(skill,false);
  check(f.world.apply_health_regeneration(f.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  f.world.configure_attack_block_draws([]{return 0.;});const auto plan=*combat::weapon::attack_skill(skill,12,0);
  const auto permit=skill==359?f.world.prepare_melee_attack(f.source,f.target,f.weapon,plan,1000):combat::weapon::AttackPermit{};
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,1500,permit)&&f.health()==1000&&!f.world.knocked_down(f.target,1500));
  check(!f.world.has_condition(f.target,weakness,1500));
 }
 for(unsigned skill:{355u,357u,358u,359u,360u})for(bool npc:{false,true}){
  warrior_test::Fixture f(skill,npc,1);const auto plan=*combat::weapon::attack_skill(skill,12,0);
  const auto permit=skill==359?f.world.prepare_melee_attack(f.source,f.target,f.weapon,plan,1000):combat::weapon::AttackPermit{};
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,1500,permit)&&f.health()==0);
  check(!f.world.knocked_down(f.target,1500)&&!f.world.has_condition(f.target,weakness,1500));
  if(npc)check(f.world.take_npc_deaths(f.source).size()==1);
 }
 std::cout<<"PASS hammer player/NPC hit/miss/start-contact/action matrix,Backbreaker durations,known/unknown immunity and block suppression\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
