#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Vigorous skill attacks line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{341u,386u,356u,396u,403u})for(bool npc:{false,true})for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(skill,npc);auto& w=f.world;w.set_vitals(1,{500,1000,30,30},100);
  check(w.complete_vigorous_spirit(f.source,f.source,*vigorous_spirit(12),0,100));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return 0.;});
  if(mode==1)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,10000));
  if(mode==2){if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));
   else check(w.apply_health_regeneration(f.target,f.target.actor,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));}
  auto plan=*combat::weapon::attack_skill(skill,12);if(mode==3)++plan.bonus;
  f.box->drain();check(w.melee_attack(f.source,f.target,f.weapon,plan,1000)==(mode!=3));
  const unsigned attacks=mode==3?0:plan.arrows;check(w.resources(1)->health==500+17*attacks);
  auto packets=f.box->drain();const auto heal=skills::particles::health_change_2022(1,1,17,1000);
  check(std::count(packets.begin(),packets.end(),heal)==attacks);
  w.tick(1500);w.tick(1600);check(w.resources(1)->health==500+17*attacks);
  packets=f.box->drain();check(std::count(packets.begin(),packets.end(),heal)==0);
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==attacks);
  for(const auto& outcome:outcomes){const auto expected=mode==1?world::Instance::AttackOutcome::miss:mode==2&&skill!=403?world::Instance::AttackOutcome::block:world::Instance::AttackOutcome::hit;if(outcome.outcome!=expected)std::cerr<<"skill="<<skill<<" npc="<<npc<<" mode="<<mode<<" outcome="<<unsigned(outcome.outcome)<<'\n';check(outcome.outcome==expected);}
 }
 for(unsigned skill:{330u,395u,2144u})for(unsigned count:{0u,1u,3u,7u})for(unsigned mode=0;mode<4;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,30,30},100);const auto source=*w.effect_scope(1);
  combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,skill==330?2u:5u};check(w.ordinary_weapon(source,weapon,20));
  for(unsigned id=2;id<2+count;++id){w.join(id,"Foe",60+id,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{1000,1000,30,30},100);if(mode!=3||id!=count+1)check(w.physical_armor(*w.effect_scope(id),60));
   if(mode==2)check(w.apply_health_regeneration(*w.effect_scope(id),id,*skills::regeneration::reviewed(258,12),100,6100,1248,true,*w.effect_scope(id)));}
  check(w.complete_vigorous_spirit(source,source,*vigorous_spirit(12),0,100));
  if(mode==1)check(w.apply_condition(source,source,combat::conditions::Kind::blind,100,10000));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return 0.;});box->drain();
  const auto target=skill==330?source:count?*w.effect_scope(2):skills::effects::Scope{source.instance,2,1};
  const bool selected_unreviewed=mode==3&&count&&(skill==330||count<=(skill==395?6u:4u));
  const bool accepted=(skill==330||count)&&!selected_unreviewed;const auto plan=*combat::weapon::attack_skill(skill,12);
  check(w.melee_attack(source,target,weapon,plan,1000)==accepted);
  const unsigned attacks=!accepted?0:skill==330?1:std::min(count,skill==395?6u:4u);
  check(w.resources(1)->health==500+17*attacks);
  auto packets=box->drain();const auto heal=skills::particles::health_change_2022(1,1,17,1000);check(std::count(packets.begin(),packets.end(),heal)==attacks);
  w.tick(1500);w.tick(1600);check(w.resources(1)->health==500+17*attacks);packets=box->drain();check(std::count(packets.begin(),packets.end(),heal)==0);
  const auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==(!accepted?0:skill==330?count:attacks));
 }
 // Illusionary replacement still constitutes one actual melee attack.
 for(bool area:{false,true})for(bool npc:{false,true}){
  warrior_test::Fixture f(area?330:341,npc);auto& w=f.world;w.set_vitals(1,{500,1000,30,30},100);skills::Slots bar{};bar[0]={33,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,33,123,12,100,30100,true));check(w.complete_vigorous_spirit(f.source,f.source,*vigorous_spirit(12),0,100));f.box->drain();
  check(w.melee_attack(f.source,area?f.source:f.target,f.weapon,*combat::weapon::attack_skill(area?330:341,12),1000));check(w.resources(1)->health==517);
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==world::Instance::AttackOutcome::replaced);
 }
 std::cout<<"PASS92 Vigorous melee/bow skill releases,multiple arrows,area attack versus recipient count,miss/block,rejected plans,empty Cyclone,IW and no impact duplication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
