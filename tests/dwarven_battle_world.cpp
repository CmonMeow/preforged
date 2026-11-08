#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Dwarven World line "+std::to_string(at.line()));}
int main()try{
 {warrior_test::Fixture f(331,false);auto& w=f.world;
  check(w.status_mutation(f.source,375,123,0,100,5100,true));
  check(w.weapon_attack_timing(f.source,200)->period_ms==1173);
  check(w.attack_duration(f.source,500,200)==335&&w.attack_duration(f.source,500,200,true)==335);
  auto axe=f.weapon;axe.item_type=2;axe.revision=2;check(w.ordinary_weapon(f.source,axe,20,300));
  check(!w.weapon_attack_timing(f.source,200)&&!w.attack_duration(f.source,500,200));
  check(w.weapon_attack_timing(f.source,300)->period_ms==1330&&w.attack_duration(f.source,500,300)==500);
  auto hammer=f.weapon;hammer.revision=3;check(w.ordinary_weapon(f.source,hammer,20,400));check(w.weapon_attack_timing(f.source,400)->period_ms==1173);
  check(w.ordinary_weapon(f.source,{},20,500));check(!w.weapon_attack_timing(f.source,500)&&w.attack_duration(f.source,500,500)==500);
  hammer.revision=4;check(w.ordinary_weapon(f.source,hammer,20,600));check(w.weapon_attack_timing(f.source,5100)->period_ms==1750);
 }
 for(unsigned mode=0;mode<7;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;auto hammer=f.weapon;hammer.item_type=15;
  check(w.ordinary_weapon(f.target,mode==2?std::optional<combat::weapon::Owned>{}:std::optional(mode==1||mode==5?f.weapon:hammer),20,100));
  check(w.status_mutation(f.target,375,123,0,100,5100,true));
  if(mode==4||mode==5){auto swapped=mode==4?f.weapon:hammer;swapped.revision=2;check(w.ordinary_weapon(f.target,swapped,20,200));}
  if(mode==6)check(w.status_mutation(f.target,361,124,15,100,20100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),mode==3?5100:300));
  check(f.health()==unsigned(mode==0||mode==5||mode==6?990:980));
 }
 for(bool hammer:{false,true}){warrior_test::Fixture f(340,false);auto& w=f.world;auto gear=f.weapon;if(hammer)gear.item_type=15;
  check(w.ordinary_weapon(f.target,gear,20,100));check(w.elemental_armor(f.target,60,60));check(w.combat_armor(f.target,60));
  check(w.status_mutation(f.target,375,123,0,100,5100,true));check(w.elemental(f.source,f.target,*elemental(191,0),20,300));
  check(f.health()==unsigned(hammer?990:980));
 }
 {warrior_test::Fixture f(399,false);auto& w=f.world;auto hammer=f.weapon;hammer.item_type=15;check(w.ordinary_weapon(f.target,hammer,20,100));
  check(w.status_mutation(f.target,375,123,0,100,5100,true));check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),300));w.tick(400);check(f.health()==987);
 }
 {warrior_test::Fixture f(359,false);auto& w=f.world;Slots bar{};bar[0]={359,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,375,123,0,100,5100,true));const auto permit=w.prepare_melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(359,12),1000);
  check(permit&&permit->release_time()==1335);check(w.accepted_skill_start(f.source,{359,0},1000));check(w.attack_duration(f.source,500,1000,true)==335);
 }
 for(unsigned mode=0;mode<7;++mode){warrior_test::Fixture f(352,false);auto& w=f.world;
  if(mode!=6)check(w.status_mutation(f.source,375,123,0,100,5100,true));
  check(w.cast_activity(f.target,mode==1?1:281,100,3000));
  if(mode==2)check(w.status_mutation(f.target,16,124,12,100,10100,true));
  if(mode==3)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==4){check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
  const auto epoch=w.control_epoch(f.target);
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(352,12),mode==5?5100:1000));
  check((w.control_epoch(f.target)!=epoch)==(mode==0||mode==1));
  if(mode==2)check(w.casting_skill(f.target,1000)==281);
 }
 for(unsigned mode=0;mode<6;++mode){warrior_test::Fixture f(352,true);auto& w=f.world;
  check(w.status_mutation(f.source,375,123,0,100,5100,true));
  const auto kind=mode==2?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell;
  std::optional<Slot> slot;if(mode!=2&&mode!=4)slot=Slot{static_cast<std::uint16_t>(mode==1?1:281),0};
  const auto token=w.begin_npc_action(f.target,kind,100,mode==3?1000:3000,slot);check(token.has_value());
  if(mode==5)check(w.npc_interrupt_stance(f.target,16,12,10000,100));
  f.box->drain();check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(352,12),1000)==(mode!=4));
  const auto wire=f.box->drain();const auto stopped=mode==2?combat::weapon::stop_2022(f.target.actor):casting::terminal(59,f.target.actor);
  check(std::count(wire.begin(),wire.end(),stopped)==unsigned(mode<3));
  check(w.complete_npc_action(*token,3000)==(mode>=3));
  if(mode==4)check(f.health()==1000);
 }
 {warrior_test::Fixture f(352,false);auto& w=f.world;
  check(w.status_mutation(f.source,375,123,0,100,5100,true));check(w.cast_activity(f.target,281,100,3000));
  const auto epoch=w.control_epoch(f.target);check(w.ordinary_attack(f.source,f.target.actor,1000));w.tick(1400);
  check(w.control_epoch(f.target)==epoch&&w.casting_skill(f.target,1400)==281);w.cancel_ordinary_attack(f.source);
 }
 for(bool known:{false,true}){warrior_test::Fixture f(352,true,1000,true,true,1,known?std::optional(combat::npcs::Energy{10,20}):std::nullopt);auto& w=f.world;
  check(w.status_mutation(f.source,375,123,0,100,5100,true));check(w.npc_interrupt_stance(f.target,17,0,10000,100));
  const auto token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,100,3000,Slot{281,0});check(token.has_value());
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(352,12),1000)==known);
  check(w.complete_npc_action(*token,3000));if(!known)check(f.health()==1000);else check(w.npc_energy_snapshot(f.target)->current==0);
 }
 {warrior_test::Fixture f(352,false);auto& w=f.world;Slots bar{};bar[0]={375,0};
  casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> packets;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](auto n){check(n==5);return w.consume_energy(1,n,f.source);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};h.status_mutation=[&](auto s,auto e,auto rank,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,rank,at,until,active);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[375]=1;attributes::Ranks ranks{};ranks[17]=12;
  check(runtime.start(1000,{375,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);
  check(w.resources(1)->energy==25);check(std::count(packets.begin(),packets.end(),casting::generic(21,1,634))==1);
  check(w.status_snapshot(f.source,1001)->front().skill==375);
  check(runtime.start(1001,{375,0,1},bar,p,ranks,true,h)!=casting::Reason::accepted);
  check(runtime.start(20999,{375,0,1},bar,p,ranks,true,h)!=casting::Reason::accepted);
  check(runtime.start(21000,{375,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);
 }
 std::cout<<"PASS staged Dwarven current hammer IAS/armor,swaps/unequip/expiry,skill armor cap,elemental/fixed damage and Backbreaker timing;player/NPC hit interruption/protection and Runtime payment/body634/recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
