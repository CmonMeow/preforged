#include "dagger_radius_fixture_draft.h"
int main()try{
 for(bool npc:{false,true})for(unsigned skill:{775u,776u,777u,3061u})for(unsigned mode=0;mode<4;++mode){
  Fixture f(npc);f.offhand();check(f.world.complete_vigorous_spirit(f.source,f.source,*effects::vigorous_spirit(0),0,1000));
  f.world.set_vitals(1,{10,1000,30,30},100);
  if(mode==1){check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1000,5000));f.world.configure_attack_miss_draws([]{return 0.;});}
  if(mode==2){if(npc)check(f.world.stage_npc_shield_deflection(f.target,*effects::shield_deflection(12),1000));
   else check(f.world.status_mutation(f.target,259,50,12,1000,9000,true));f.world.configure_attack_block_draws([]{return 0.;});}
  if(mode==3){skills::Slots bar{};bar[0]={33,0};check(f.world.configure_adrenaline(f.source,bar,1000));check(f.world.status_mutation(f.source,33,123,12,1000,31000,true));}
  check(f.hit(skill));check(f.world.resources(1)->health==20); // TWO actual attacks, including miss/block/IW
  const auto events=f.world.take_attack_outcomes(f.source);check(events.size()==2&&events[0].event!=events[1].event);
  for(const auto& e:events)check(e.outcome==(mode==0?combat::weapon::AttackOutcome::hit:mode==1?combat::weapon::AttackOutcome::miss:mode==2?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::replaced));
 }
 for(unsigned skill:{775u,776u,777u,3061u,780u,3251u,778u}){
  Fixture f(false);check(f.world.complete_vigorous_spirit(f.source,f.source,*effects::vigorous_spirit(0),0,1000));f.world.set_vitals(1,{10,1000,30,30},100);
  const auto plan=*combat::weapon::attack_skill(skill,0,0,0,true);const auto release=1000+plan.windup_ms;
  const auto permit=f.world.prepare_dagger_cast_draft(f.source,f.target,f.weapon,plan,1000,release);check(bool(permit));
  auto result=f.world.complete_dagger_cast_draft(f.source,f.target,f.weapon,plan,permit,release);if(result.publication)result.publication();
  check(result.accepted&&result.prerequisite_failed&&f.world.resources(1)->health==10);
  if(result.dual_failure_feedback){auto second=f.world.complete_dagger_failed_second_draft(result.dual_failure_feedback,release+500);if(second.publication)second.publication();check(second.accepted&&f.world.resources(1)->health==10);}
  for(const auto& e:f.world.take_attack_outcomes(f.source))check(e.skill_failed&&!e.before_damage);
 }
 {Fixture f(false);f.world.join(3,"Area foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.world.set_vitals(3,{1000,1000,30,30},100);check(f.world.physical_armor(*f.world.effect_scope(3),60));
  auto axe=f.weapon;axe.item_type=2;check(f.world.ordinary_weapon(f.source,axe,20,1000,combat::weapon::CriticalProfile{1,0,20}));
  check(f.world.complete_vigorous_spirit(f.source,f.source,*effects::vigorous_spirit(0),0,1000));f.world.set_vitals(1,{10,1000,30,30},100);
  check(f.world.melee_attack(f.source,f.source,axe,*combat::weapon::attack_skill(330,0),1000));
  check(f.world.take_attack_outcomes(f.source).size()==2&&f.world.resources(1)->health==15); // ONE circular attack, multiple recipients
 }
 std::cout<<"PASS candidate actual dagger attack multiplicity (hit/miss/block/IW), failed0Vigor, circular multiple-hit once; Scythe producer and player Empathy remain gaps\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
