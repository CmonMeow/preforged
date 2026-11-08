#include "warrior_condition_fixture.h"
#include <source_location>
#include <iostream>
#include <string>
#include <stdexcept>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Bull's Charge World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<8;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={379,0};bar[1]={344,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,379,123,0,100,5100,true));
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,10000));
  if(mode==3){check(w.protective_enchantment(f.target,f.target,*regeneration::reviewed(258,0),0,100));w.configure_attack_block_draws([]{return 0.;});}
  if(mode==4)check(w.status_mutation(f.target,371,124,0,100,8100,true));
  if(mode==5){f.weapon.item_type=5;check(w.ordinary_weapon(f.source,f.weapon,20));}
  if(mode==6)check(w.accepted_skill_start(f.source,{344,0},200));
  const unsigned at=mode==7?5100:200;
  check(w.ordinary_attack(f.source,f.target.actor,at));
  const auto timing=w.weapon_attack_timing(f.source,at);check(timing.has_value());const auto contact=at+timing->windup_ms;
  if(mode!=1)w.move(2,1000,0,contact-1);
  w.tick(contact);if(mode==5)w.tick(contact+100);
  check(w.knocked_down(f.target,contact)==(mode==0));
  if(mode==0){check(!w.knocked_down(f.target,contact+2000));}
  // Ordinary attacks preserve the owned stance;the accepted action and expiry end it.
  const auto entries=w.status_snapshot(f.source,contact);check(entries.has_value());
  check(std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==379;})==(mode!=6&&mode!=7));
 }
 for(bool immune:{false,true}){
  world::Instance w;w.configure_attack_miss_draws([]{return .5;});
  world::Instance::Npc guardian;guardian.id=guardian.definition.id=100;guardian.x=1000;guardian.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::neutral;d.spawn_health=combat::Health{1000,1000};d.damageable=true;
  guardian.combat_definition=d;w.configure_npcs({guardian});const auto guard=*w.effect_scope(100);
  w.join(1,"Warrior",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,30,30},100);const auto source=*w.effect_scope(1);
  combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,combat::weapon::attack_skill(340,12)->required_type};check(w.ordinary_weapon(source,weapon,20));
  auto enemy=guardian;enemy.x=60;enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
  enemy.combat_definition->physical_armor=60;enemy.combat_definition->physical_armor_evidence="test";
  enemy.combat_definition->knockdown_allowed=!immune;enemy.combat_definition->knockdown_evidence="test";
  check(w.begin_wave_objective(source,guard,73,1,3,1)&&w.spawn_quest_wave(source,73,1,{enemy},[]{return true;}));
  const auto target=*w.effect_scope(60000);Slots bar{};bar[0]={379,0};check(w.configure_adrenaline(source,bar,100));
  check(w.status_mutation(source,379,123,0,100,5100,true));check(w.chase_quest_guardian(target,100,199));
  check(w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(340,12),200));
  check(w.knocked_down(target,200)==!immune);
 }
 // Death and an unrelated control share the common stance lifetime rules.
 {warrior_test::Fixture f(340,false);Slots bar{};bar[0]={379,0};check(f.world.configure_adrenaline(f.source,bar,100));
  check(f.world.status_mutation(f.source,379,123,0,100,5100,true));check(f.world.gale(f.target,f.source,12,{},200));
  check(f.world.knocked_down(f.source,200)&&f.world.status_snapshot(f.source,200)->size()==1);
  check(f.world.kill(f.source,300)&&f.world.status_snapshot(f.source,300)->empty());
 }
 // Fatal successful contacts do not publish control for a corpse.
 {warrior_test::Fixture f(340,false,1);Slots bar{};bar[0]={379,0};check(f.world.configure_adrenaline(f.source,bar,100));
  check(f.world.status_mutation(f.source,379,123,0,100,5100,true));f.world.move(2,1000,0,199);
  check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200));
  check(f.world.resources(2)->health==0&&!f.world.knocked_down(f.target,200));
 }
 for(unsigned mode=0;mode<4;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={379,0};bar[1]={344,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,379,123,0,1000,6000,true));w.move(1,10000,0,1000);w.tick(2000);
  check(std::abs(w.position(1,2000).first-288*1.33)<.3);
  if(mode==0)check(w.accepted_skill_start(f.source,{344,0},2000));
  if(mode==1)check(!w.accepted_skill_start(f.source,{344,1},2000));
  if(mode==2)check(w.status_mutation(f.source,371,124,0,2000,10000,true));
  if(mode==3)check(w.status_mutation(f.source,379,123,0,2000,{},false));
  w.tick(3000);check(std::abs(w.position(1,3000).first-(288*1.33+288*(mode==1?1.33:1.)))<.5);
 }
 {warrior_test::Fixture f(340,false);Slots bar{};bar[0]={379,0};check(f.world.configure_adrenaline(f.source,bar,100));check(f.world.status_mutation(f.source,379,123,0,1000,6000,true));f.world.move(1,10000,0,1000);f.world.tick(7000);check(std::abs(f.world.position(1,7000).first-288*(5*1.33+1))<.5);}
 std::cout<<"PASS Bull's Charge actual contacts,player/NPC immunity,movement history,accepted-use ending and expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
