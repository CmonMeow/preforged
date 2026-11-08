#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Runtime attack disable line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned attack:{340u,399u})for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(403,false);auto& w=f.world;Slots bar{};bar[0]={281,0};check(w.configure_adrenaline(f.source,bar,100));
  casting::Runtime runtime(1);casting::Hooks h;std::vector<world::Packet> events;
  unsigned calls=0,heals=0;bool fail=mode==1||mode==3;bool control_available=mode!=4;
  h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.target_valid=[](auto id,float,bool){return id==1;};h.heal=[&](auto,auto,float,bool){++heals;return true;};
  h.consume_energy_at=[&](unsigned n,auto at){return w.consume_energy(1,n,f.source,at);};
  h.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(w.cast_activity(f.source,slot,begins,ends));};
  h.control_epoch=[&](){return w.control_epoch(f.source);};h.control_event_time=[&](auto after){return w.control_event_time(f.source,after);};
  h.cooldown_control_source=[&](auto after){return control_available?w.cooldown_control_source(f.source,after):std::nullopt;};
  h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(f.source,slot,at);};
  h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  h.settle_attack_interrupt=[&](const effects::CooldownOrigin& control,Slot slot,auto base,auto now)->std::optional<std::uint64_t>{
   ++calls;check(control.stamp.owner==f.source&&control.token==1&&slot==Slot{281,0}&&now==1500&&base==3500);
   if(fail)return mode==3?std::optional<std::uint64_t>{base-1}:std::nullopt;
   if(mode==2)return 0;
   const combat::weapon::InterruptedActionReceipt receipt{f.source,slot,control.token,1000,2000,now,base};
   if(!w.settle_attack_interrupt_disable(f.source,*combat::weapon::interrupt_disable_plan(attack),receipt))return {};
   return w.skill_disabled_slot(f.source,slot,now);
  };
  Policy p;p.primary=3;p.campaigns=255;p.learned[281]=1;attributes::Ranks ranks{};
  check(runtime.start(1000,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);
  check(w.interrupt_cast(f.source,281,1500));runtime.update(2500,h);check(!heals);
  if(fail||!control_available){
   check(runtime.active()&&calls==unsigned(control_available?1:0)&&w.skill_disabled_slot(f.source,{281,0},1500)==0);
   const auto snapshot=runtime.cooldown_source_snapshot(f.source,1500,h);check(snapshot&&snapshot->timers.empty()&&snapshot->control_epoch==0);
   fail=false;control_available=true;runtime.update(2500,h);
  }
  check(!runtime.active()&&!heals&&calls==unsigned(mode==1||mode==3?2:1));
  check(std::count(events.begin(),events.end(),casting::terminal(59,1))==1);
  check(std::count(events.begin(),events.end(),casting::slot_event(228,1,{281,0},mode==2?2:22))==1);
  const auto settled_calls=calls;runtime.update(2500,h);check(calls==settled_calls&&!heals);
  const auto snapshot=runtime.cooldown_source_snapshot(f.source,2500,h);check(snapshot&&snapshot->timers.size()==1);
  check(snapshot->timers.front().until==3500&&snapshot->timers.front().slot==Slot{281,0}); // Independent layer.
  check(w.skill_disabled_slot(f.source,{281,1},2500)==0);
  check(w.skill_disabled_slot(f.source,{281,0},2500)==unsigned(mode==2?0:23500));
  runtime.update(3500,h);check(runtime.start(3500,{281,0,1},bar,p,ranks,true,h)==(mode==2?casting::Reason::accepted:casting::Reason::recharge));
 }
 std::cout<<"PASS Runtime ordinary recharge versus independent attack disable,failed/invalid settlement rollback,retry without spell execution,missing control,no-request,terminal/UI deadline and repeated update;Renewal/contact producers pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
