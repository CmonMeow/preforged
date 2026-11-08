#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("warrior condition World line "+std::to_string(at.line()));}
int main()try{
 using enum combat::conditions::Kind;
 for(unsigned skill:{320u,334u,342u,352u,849u,850u,1416u})for(bool npc:{false,true})for(bool enabled:{false,true})for(bool miss:{false,true}){
  warrior_test::Fixture f(skill,npc);const auto plan=*combat::weapon::attack_skill(skill,12,0);
  check(f.prerequisite(skill,enabled));
  if(skill==1416&&!enabled)check(f.world.status_mutation(f.source,346,123,0,100,8100,true));
  if(miss)check(f.world.apply_condition(f.source,f.source,blind,100,5000));
  f.world.take_attack_outcomes(f.source);
  const auto before=f.health();f.box->drain();
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,100));
  const auto kind=skill==320||skill==334?crippled:skill==342?weakness:skill==849||skill==1416?bleeding:deep_wound;
  check(f.world.has_condition(f.target,kind,100)==(!miss&&(skill==320||enabled)));
  if(miss)check(f.health()==before);else check(f.health()<before);
  const auto outcomes=f.world.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==(miss?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
  if(!miss&&(skill==320||enabled)){
   const auto deadline=100+combat::weapon::condition_duration(plan);
   check(f.world.has_condition(f.target,kind,deadline-1)&&!f.world.has_condition(f.target,kind,deadline));
  }
 }
 // Prerequisites expire at contact, not when a request/cast was admitted.
 for(unsigned skill:{334u,342u,850u})for(bool npc:{false,true}){
  warrior_test::Fixture f(skill,npc);check(f.prerequisite(skill,true,100,50));
  check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(skill,12,0),150));
  check(!f.world.has_condition(f.target,skill==334?crippled:skill==342?weakness:deep_wound,150));
 }
 for(unsigned skill:{352u,849u})for(bool npc:{false,true}){
  warrior_test::Fixture f(skill,npc);check(f.prerequisite(skill,true));
  check(!f.world.knocked_down(f.target,2100));
  check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(skill,12,0),2100));
  check(!f.world.has_condition(f.target,skill==352?deep_wound:bleeding,2100));
 }
 for(bool npc:{false,true}){
  warrior_test::Fixture f(1416,npc);check(f.world.status_mutation(f.source,346,123,0,100,150,true));
  check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1416,12,0),150));
  check(f.world.has_condition(f.target,bleeding,150));
 }
 // Lethal hits leave no new condition, and wrong equipment/stale scopes fail.
 for(unsigned skill:{320u,334u,342u,352u,849u,850u,1416u})for(bool npc:{false,true}){
  warrior_test::Fixture f(skill,npc,1);auto bad=f.weapon;bad.item_type=5;
  const auto plan=*combat::weapon::attack_skill(skill,12,0);
  check(!f.world.melee_attack(f.source,f.target,bad,plan,100));
  auto stale=f.target;++stale.generation;check(!f.world.melee_attack(f.source,stale,f.weapon,plan,100));
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,100)&&f.health()==0);
  for(auto kind:{bleeding,crippled,deep_wound,weakness})check(!f.world.has_condition(f.target,kind,100));
  if(npc)check(f.world.take_npc_deaths(f.source).size()==1);
 }
 std::cout<<"PASS seven attacks on players/NPCs,predicate hit/miss matrix,contact expiry,stance expiry,lethal suppression and equipment/lifetime guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
