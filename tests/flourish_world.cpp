#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Flourish World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<10;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={389,0};bar[1]={390,0};bar[2]={437,0};check(w.configure_adrenaline(f.source,bar,100));
  w.set_vitals(1,{1000,1000,mode==1?29u:10u,30},100);
  const auto permit=*w.prepare_flourish_reset(f.source,200);
  FlourishCooldowns snapshot{f.source,1,0,200,true,{{{389,0},0,0},{{390,0},3000,0},{{437,0},4000,0}}};
  auto p=*prepare_flourish_recharge(snapshot,*flourish(12),1);check(p.energy==12);
  if(mode==2){auto next=bar;std::swap(next[1],next[2]);check(w.configure_adrenaline(f.source,next,150));}
  if(mode==3)check(w.signet_humility(f.target,f.source,*signet_humility(0),150));
  if(mode==4)p.before.owner.generation++;
  if(mode==5)p.energy++;
  if(mode==6){snapshot.slots[1].slot={390,1};p=*prepare_flourish_recharge(snapshot,*flourish(12),1);}
  if(mode==7){w.leave(1);w.join(1,"Replacement",0,0,f.box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,10,30},100);}
  if(mode==8)check(w.settle_resources(f.source,300));
  if(mode==9){check(w.cast_activity(f.source,281,100,1000));check(w.interrupt_cast(f.source,281,150));}
  const auto result=w.commit_flourish_reset(p,permit,200);check(bool(result)==(mode<2));
  check(w.resources(1)->energy==unsigned(mode==1?30:mode==0?22:10));
  if(mode==9){const auto retry=*w.prepare_flourish_reset(f.source,200);check(retry.bar==permit.bar&&retry.disable_revision==permit.disable_revision&&retry.stamp!=permit.stamp);check(w.commit_flourish_reset(p,retry,200)&&w.resources(1)->energy==22);}
  if(result){check(*result==permit.stamp&&result->owner==f.source&&result->at==200);check(!w.commit_flourish_reset(p,permit,200)&&w.resources(1)->energy==unsigned(mode==1?30:22));}
 }
 // A real copied-slot attack interruption disable clears with the reward.
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={389,0};bar[1]={390,1};check(w.configure_adrenaline(f.source,bar,100));w.set_vitals(1,{1000,1000,10,30},100);
  check(w.cast_activity(f.source,Slot{390,1},100,1000));const auto epoch=*w.control_epoch(f.source);check(w.interrupt_cast(f.source,390,150));
  const combat::weapon::InterruptedActionReceipt receipt{f.source,{390,1},epoch+1,100,1000,150,15150};
  check(w.settle_attack_interrupt_disable(f.source,*combat::weapon::interrupt_disable_plan(340),receipt));
  const auto disabled=*w.skill_disabled_slot(f.source,{390,1},200);check(disabled==35150);
  const auto permit=*w.prepare_flourish_reset(f.source,200);FlourishCooldowns snapshot{f.source,1,0,200,true,{{{389,0},0,0},{{390,1},0,disabled}}};
  const auto p=*prepare_flourish_recharge(snapshot,*flourish(12),1);check(w.commit_flourish_reset(p,permit,200)&&w.resources(1)->energy==16);
  check(w.skill_disabled_slot(f.source,{390,1},200)==0);
 }
 // An unknown/later Energy state must not clear a valid disable or consume the
 // accepted event. Retry at the actual cursor with the same event is valid.
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={389,0};bar[1]={390,0};check(w.configure_adrenaline(f.source,bar,100));
  w.set_vitals(1,{1000,1000,10,30},100);check(w.settle_resources(f.source,300));const auto permit=*w.prepare_flourish_reset(f.source,200);
  FlourishCooldowns snapshot{f.source,1,0,200,true,{{{389,0},0,0},{{390,0},5000,0}}};
  check(!w.commit_flourish_reset(*prepare_flourish_recharge(snapshot,*flourish(12),1),permit,200));snapshot.at=300;const auto retry=*w.prepare_flourish_reset(f.source,300);
  check(w.commit_flourish_reset(*prepare_flourish_recharge(snapshot,*flourish(12),1),retry,300)&&w.resources(1)->energy==16);
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={389,0};bar[1]={390,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.activate_overcast_energy(f.source,231,0,1,1000));const auto permit=*w.prepare_flourish_reset(f.source,1501);
  FlourishCooldowns snapshot{f.source,1,0,1501,true,{{{389,0},0,0},{{390,0},5000,0}}};
  check(w.commit_flourish_reset(*prepare_flourish_recharge(snapshot,*flourish(12),1),permit,1501).has_value());
  check(overcast::precise_current(*w.overcast_energy_snapshot(f.source))==20*overcast::energy_unit+49599);
 }
 for(bool burden:{false,true}){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={389,0};bar[1]={390,0};check(w.configure_adrenaline(f.source,bar,100));
  w.set_vitals(1,{1000,1000,0,30},100);check(w.energy_capacity(1,1,0,100,{0,-2}));if(burden)check(w.activate_overcast_energy(f.source,231,0,1,1000));
  const auto permit=*w.prepare_flourish_reset(f.source,1000);FlourishCooldowns snapshot{f.source,1,0,1000,true,{{{389,0},0,0},{{390,0},5000,0}}};
  check(w.commit_flourish_reset(*prepare_flourish_recharge(snapshot,*flourish(12),1),permit,1000).has_value());
  check(w.resources(1)->energy==0);if(burden)check(w.overcast_energy_snapshot(f.source)->debt==4);else {check(!w.overcast_energy_snapshot(f.source));check(w.restore_energy(1,4,f.source,1000));check(w.resources(1)->energy==0);check(w.restore_energy(1,1,f.source,1000));check(w.resources(1)->energy==1);}
 }
 std::cout<<"PASS staged Flourish World Energy/cap,replay,bar/disable revision,forged generation/reward/copy and exact snapshot checks,actual copied disable clear,Overcast fractional ceiling and debt;Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
