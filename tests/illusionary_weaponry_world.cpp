#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("IW ordinary World line "+std::to_string(at.line()));
}
int main()try{
 for(bool npc:{false,true})for(bool expired:{false,true}){
  warrior_test::Fixture f(383,npc);auto& w=f.world;
  skills::Slots bar{};bar[0]={33,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,33,123,12,100,30100,true));
  check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,40000));
  check(w.conjure_enchantment(f.source,*skills::effects::conjure(182,12),100));
  if(!npc)check(w.status_mutation(f.target,453,124,12,100,10100,true));
  unsigned block_draws=0,miss_draws=0;
  w.configure_attack_block_draws([&]{++block_draws;return 0.;});
  w.configure_attack_miss_draws([&]{++miss_draws;return 0.;});
  const auto at=expired?30100u:100u;
  check(w.ordinary_attack(f.source,f.target.actor,at));w.tick(at+500);w.cancel_ordinary_attack(f.source);
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1);
  check(events[0].outcome==(expired?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::replaced));
  check(!events[0].before_damage);
  check(f.health()==(expired?1000:966)); // No weapon hit or Conjure bonus.
  check(block_draws==0&&miss_draws==unsigned(expired));
 }
 for(bool npc:{false,true})for(unsigned skill:{383u,331u,326u,390u}){
  warrior_test::Fixture f(skill,npc,1000,std::nullopt);auto& w=f.world;
  if(!f.weapon.item_type){f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));}
  skills::Slots bar{};bar[0]={33,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,33,123,12,100,30100,true));
  check(f.condition(combat::conditions::Kind::bleeding,1000,5000));
  check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1000,5000));
  if(!npc)check(w.status_mutation(f.target,453,124,12,1000,11000,true));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});w.configure_attack_miss_draws([&]{++draws;return 0.;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(skill,12),1000));
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::replaced&&!events[0].before_damage);
  check(f.health()==966&&draws==0&&!w.has_condition(f.target,combat::conditions::Kind::deep_wound,1000)&&!w.knocked_down(f.target,1000));
 }
 std::cout<<"PASS IW ordinary and targeted-skill player/NPC replacement bypasses Blind/block RNG,omits weapon/Conjure/condition/knockdown effects and expires normally;full skill wiring pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
