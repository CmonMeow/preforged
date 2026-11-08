#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Battle Rage World line "+std::to_string(at.line()));
}
combat::adrenaline::Pool pool(world::Instance& w,Scope owner,unsigned skill=340){
 const auto pools=w.adrenaline(owner);check(pools.has_value());for(const auto& p:*pools)if(p.slot.skill==skill)return p;throw std::runtime_error("missing pool");
}
int main()try{
 for(bool npc:{false,true})for(unsigned mode=0;mode<9;++mode){
  if(npc&&mode==6)continue; // Hostile NPC cannot receive a friendly Guardian cast.
  warrior_test::Fixture f(340,npc);auto& w=f.world;Slots bar{};bar[0]={317,0};bar[1]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  if(mode==1||mode==2)check(w.for_great_justice(f.source,*for_great_justice(343,mode==2),100));
  if(mode==3)check(w.status_mutation(f.source,370,122,0,100,5100,true));
  check(w.status_mutation(f.source,317,123,0,100,5100,true));
  if(mode==4)check(w.status_mutation(f.source,370,124,0,100,5100,true));
  if(mode==5)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,10000));
  if(mode==6){check(w.protective_enchantment(f.target,f.target,*regeneration::reviewed(258,0),0,100));w.configure_attack_block_draws([]{return 0.;});}
  if(mode==7)check(w.status_mutation(f.source,56,124,0,100,8100,true));
  const auto at=mode==8?5100u:200u;
  check(w.ordinary_attack(f.source,f.target.actor,at));w.tick(at+(mode==4?335:500));
  const auto p=pool(w,f.source);const unsigned expected=mode==2?100:mode==4?37:mode>=5&&mode<=7?0:mode==8?25:50;
  check(p.units==expected&&p.half==unsigned(mode==4));
  if(mode==3)check(w.attack_duration(f.source,500,at)==500); // Replaces Berserker;no IAS.
 }
 // A released projectile samples the rate on actual impact,not at release.
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={317,0};bar[1]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  w.leave(2);w.join(2,"Distant target",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,30,30},100);
  f.target=*w.effect_scope(2);check(w.physical_armor(f.target,60));
  f.weapon.item_type=5;check(w.ordinary_weapon(f.source,f.weapon,20)&&w.configure_projectiles({2500,32}));
  check(w.status_mutation(f.source,317,123,0,100,5100,true));
  const auto timing=w.weapon_attack_timing(f.source,100);check(timing&&timing->windup_ms<4900);
  check(w.ordinary_attack(f.source,2,5000-timing->windup_ms));w.tick(5000);check(pool(w,f.source).units==0);
  w.tick(5700);const auto outcomes=w.take_attack_outcomes(f.source);
  check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::hit&&outcomes[0].released==5000&&outcomes[0].at>=5100);
  check(pool(w,f.source).units==25&&battle_rage_active(*w.status_snapshot(f.source,5700),5700)==false);
 }
 // Battle Rage's attack-only rate does not multiply incoming-damage gain.
 for(bool justice:{false,true}){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.target,bar,100));
  if(justice)check(w.for_great_justice(f.target,*for_great_justice(),100));
  check(w.status_mutation(f.target,317,123,0,100,5100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200));check(pool(w,f.target).units==(justice?4u:2u));
 }
 // Exact accepted non-adrenal copy ends Battle Rage. Adrenal uses preserve it,
 // rejected copies leave it active,and movement history closes at that event.
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={317,0};bar[1]={340,0};bar[2]={344,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,317,123,0,1000,6000,true));w.move(1,10000,0,1000);w.tick(2000);
  check(std::abs(w.position(1,2000).first-288*1.33)<.3);
  if(mode==0)check(w.accepted_skill_start(f.source,{344,0},2000));
  if(mode==1)check(w.accepted_skill_start(f.source,{340,0},2000));
  if(mode==2)check(!w.accepted_skill_start(f.source,{344,1},2000));
  if(mode==3)check(w.status_mutation(f.source,371,124,0,2000,10000,true));
  if(mode==4)check(w.status_mutation(f.source,317,123,0,2000,{},false));
  w.tick(3000);check(std::abs(w.position(1,3000).first-(288*1.33+288*((mode==1||mode==2)?1.33:1.)))<.5);
  check(battle_rage_active(*w.status_snapshot(f.source,3000),3000)==(mode==1||mode==2));
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={317,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,317,123,0,1000,6000,true));w.move(1,10000,0,1000);w.tick(7000);
  check(std::abs(w.position(1,7000).first-288*(5*1.33+1))<.5);check(w.status_snapshot(f.source,7000)->empty());
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={317,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,317,123,0,1000,6000,true));check(w.gale(f.target,f.source,12,{},2000));
  check(w.knocked_down(f.source,2000)&&battle_rage_active(*w.status_snapshot(f.source,2000),2000)==true);
  check(w.kill(f.source,2100)&&w.status_snapshot(f.source,2100)->empty());
 }
 std::cout<<"PASS Battle Rage actual player/NPC hit gain,Justice cap/PvP fixed extra,Berserker replacement,miss/block/forbidden,damage exclusion,movement boundaries,accepted use,control and death\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
