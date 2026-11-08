#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Shield Bash World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<9;++mode){
  const unsigned id=mode==1?321:340;warrior_test::Fixture f(id,false);auto& w=f.world;
  f.weapon.item_type=2;check(w.ordinary_weapon(f.source,f.weapon,20));
  Slots bar{};bar[0]={static_cast<std::uint16_t>(id),7};check(w.configure_adrenaline(f.source,bar,100));
  check(w.shield_equipment(f.target,ShieldEquipment{1,mode==5?0u:55u},12,100));
  check(w.status_mutation(f.target,363,110,0,100,5100,true));
  if(mode==4)check(w.status_mutation(f.source,371,111,0,100,8100,true));
  if(mode==6)check(w.apply_condition(f.target,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==7)check(w.status_mutation(f.target,137,111,0,100,8100,true));
  const auto at=mode==8?5100u:200u;
  if(mode!=2)check(w.prepare_diversion_skill_use(f.source,{static_cast<std::uint16_t>(id),mode==3?8u:7u},1,at));
  const auto plan=*combat::weapon::attack_skill(id,12,0,7);
  const bool attempted=w.melee_attack(f.source,f.target,f.weapon,plan,at);
  if(mode==2||mode==3){check(!attempted&&w.status_snapshot(f.target,at)->size()==1);continue;}
  check(attempted);
  const bool triggered=mode<=1||mode==4;
  check(w.knocked_down(f.source,at)==(triggered&&mode!=4));
  const auto active=*w.status_snapshot(f.target,at);
  check(std::none_of(active.begin(),active.end(),[](const auto& e){return e.skill==363;})==(triggered||mode==8));
  const auto finalized=w.finalize_diversion_skill_use(f.source,1,at,9000,0);check(finalized.has_value());
  const auto completion=w.cooldown_completion_stamp(f.source,1);check(completion&&completion->slot==Slot{static_cast<std::uint16_t>(id),7});
  check(completion->base_ms==9000&&completion->extra_ms==(triggered?15000u:0u));
  check(w.skill_disabled_slot(f.source,{static_cast<std::uint16_t>(id),7},at)==0);
  check(completion_deadline(*completion)==at+9000+(triggered?15000u:0u));
  check(!w.finalize_diversion_skill_use(f.source,1,at,9000,0));
 }
 // Ranged attack skills consume at impact, without a melee pending-use or
 // retroactively extending their already-completed recharge.
 {warrior_test::Fixture f(399,false);auto& w=f.world;f.weapon.item_type=5;check(w.ordinary_weapon(f.source,f.weapon,20));
  check(w.configure_projectiles({1000,32}));check(w.shield_equipment(f.target,ShieldEquipment{1,55},12,100));
  check(w.status_mutation(f.target,363,110,0,100,5100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12,0,7),200));
  w.tick(1000);check(f.health()==1000&&!w.knocked_down(f.source,1000));
  check(w.status_snapshot(f.target,1000)->empty());check(!w.cooldown_completion_stamp(f.source,1));
 }
 // One circular attack has one bounded +15s base modifier even when two
 // defenders consume their Shield Bash. Retail multi-defender stacking remains
 // an explicitly provisional rule; each protected recipient still blocks once.
 {warrior_test::Fixture f(330,false);auto& w=f.world;f.weapon.item_type=2;check(w.ordinary_weapon(f.source,f.weapon,20));
  w.join(3,"Second defender",0,60,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,30,30},100);
  const auto third=*w.effect_scope(3);check(w.physical_armor(third,60));
  for(const auto target:{f.target,third}){check(w.shield_equipment(target,ShieldEquipment{1,55},12,100));check(w.status_mutation(target,363,110,0,100,5100,true));}
  Slots bar{};bar[0]={330,7};check(w.configure_adrenaline(f.source,bar,100));
  check(w.prepare_diversion_skill_use(f.source,{330,7},1,200));
  check(w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,12,0,7),200));
  check(w.status_snapshot(f.target,200)->empty()&&w.status_snapshot(third,200)->empty());
  check(w.resources(2)->health==1000&&w.resources(3)->health==1000&&w.knocked_down(f.source,200));
  check(w.finalize_diversion_skill_use(f.source,1,200,4000,0).has_value());
  const auto receipt=w.cooldown_completion_stamp(f.source,1);check(receipt&&receipt->extra_ms==15000&&completion_deadline(*receipt)==19200);
 }
 // An earlier recipient's Judgment kills the source before the later Bash
 // terminal. That already-attributed block still consumes its defender's buff.
 {warrior_test::Fixture f(330,false);auto& w=f.world;f.weapon.item_type=2;check(w.ordinary_weapon(f.source,f.weapon,20));
  w.join(3,"Bash defender",0,60,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,30,30},100);
  const auto third=*w.effect_scope(3);check(w.physical_armor(third,60));
  check(w.ordinary_weapon(f.target,combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20));
  check(w.player_holy_equipment(f.source,0));
  check(w.complete_shield_judgment(f.target,f.target,*shield_judgment(0),0,100));
  check(w.shield_equipment(third,ShieldEquipment{1,55},12,100));check(w.status_mutation(third,363,110,0,100,5100,true));
  w.set_vitals(1,{1,1000,30,30},100);Slots bar{};bar[0]={330,7};check(w.configure_adrenaline(f.source,bar,100));
  check(w.prepare_diversion_skill_use(f.source,{330,7},1,200));
  check(w.melee_attack(f.source,f.source,f.weapon,*combat::weapon::attack_skill(330,12,0,7),200));
  check(w.resources(1)->health==0&&w.resources(3)->health==1000&&w.status_snapshot(third,200)->empty());
  check(!w.cooldown_completion_stamp(f.source,1));
 }
 std::cout<<"PASS Shield Bash terminal block/bypass, melee knockdown/current shield, pending-use guards, ranged impact, one-use multi-recipient recharge and dead-attacker consumption\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
