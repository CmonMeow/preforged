#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Flourish cast line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<7;++mode){
  warrior_test::Fixture f(400,false);auto& w=f.world;Slots bar{};bar[0]={400,0};bar[1]={389,0};
  check(w.configure_adrenaline(f.source,bar,100));
  casting::Runtime cast(1);casting::Hooks h;std::vector<world::Packet> sent;unsigned commits=0;std::optional<effects::FlourishPermit> used_permit;std::optional<effects::FlourishRecharge> used_proposal;
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,f.source,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,float,bool){return id==1;};
  h.weapon_snapshot=[&](){return std::optional{f.weapon};};
  h.melee_target=[&](auto id,auto now){return w.melee_target(f.source,id,now,400);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto now){return w.melee_attack(f.source,target,weapon,plan,now);};
  h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};
  h.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(w.cast_activity(f.source,slot,begins,ends));};
  h.control_epoch=[&](){return w.control_epoch(f.source);};h.control_event_time=[&](auto after){return w.control_event_time(f.source,after);};
  h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};
  h.weapon_attack_timing=[&](auto now){return w.weapon_attack_timing(f.source,now);};
  h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  h.skill_disabled_slot=[&](auto slot,auto now){return w.skill_disabled_slot(f.source,slot,now);};
  h.prepare_skill_use=[&](auto slot,auto event,auto now){return w.prepare_diversion_skill_use(f.source,slot,event,now);};
  h.finalize_skill_use=[&](auto event,auto now,auto base,auto existing,bool instant){return w.finalize_diversion_skill_use(f.source,event,now,base,existing,instant);};
  h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(f.source,event);};
  h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(f.source,event);};
  h.cooldown_control_source=[&](auto after){return w.cooldown_control_source(f.source,after);};
  h.prepare_flourish_reset=[&](auto owner,auto now){return w.prepare_flourish_reset(owner,now);};
  h.commit_flourish_reset=[&](const auto& proposal,const auto& permit,auto now)->std::optional<effects::CooldownStamp>{
   ++commits;used_permit=permit;used_proposal=proposal;if(mode==3)return {};if(mode==4)check(w.configure_adrenaline(f.source,bar,now));
   return w.commit_flourish_reset(proposal,permit,now);
  };
  Policy policy;policy.primary=2;policy.secondary=1;policy.campaigns=255;policy.learned[400]=policy.learned[389]=1;
  attributes::Ranks ranks{};ranks[25]=12;ranks[17]=12;
  check(cast.start(1000,{400,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(2000,h);
  auto before=*cast.cooldown_source_snapshot(f.source,2000,h);check(before.timers.size()==1&&before.timers.front().slot==Slot{400,0}&&before.timers.front().origin);
  check(w.publish_local_cooldowns(f.source,before));const auto energy=w.resources(1)->energy;
  if(mode==5)h.commit_flourish_reset={};
  check(cast.start(3000,{389,0,2},bar,policy,ranks,true,h)==(mode==5?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode!=5)check(std::count(sent.begin(),sent.end(),casting::activation(1,1,389))==1);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(3200,h);if(mode==6)h.prepare_flourish_reset={};
  cast.update(4000,h);
  const auto after=*cast.cooldown_source_snapshot(f.source,4000,h);
  const auto attack=std::find_if(after.timers.begin(),after.timers.end(),[](const auto& t){return t.slot==Slot{400,0};});
  if(mode){check(attack!=after.timers.end());check(commits==((mode==3||mode==4)?1u:0u));check(w.resources(1)->energy==energy-(mode==5?0:5));continue;}
  check(std::count(sent.begin(),sent.end(),casting::terminal(58,1))==1);
  check(commits==1&&attack==after.timers.end()&&w.resources(1)->energy==energy-5+6);
  check(after.reset_order&&after.timers.size()==1&&after.timers.front().slot==Slot{389,0}&&after.timers.front().until==12000);
  const auto wire_count=sent.size();check(!cast.reset_flourish(*effects::flourish(12),*used_permit,4000,used_proposal->event,h));
  check(commits==1&&sent.size()==wire_count&&w.resources(1)->energy==energy-5+6);
  check(w.publish_local_cooldowns(f.source,after));const auto permit=w.prepare_cooldown_checkpoint(f.source,after,4000);
  check(permit&&cast.apply_cooldown_checkpoint(*permit,4000,h));
  check(std::count(sent.begin(),sent.end(),casting::slot_event(229,1,{400,0}))==1);
  check(cast.start(5000,{389,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
  check(cast.start(5000,{400,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(6000,h);
  const auto later=*cast.cooldown_source_snapshot(f.source,6000,h);
  check(std::any_of(later.timers.begin(),later.timers.end(),[](const auto& t){return t.slot==Slot{400,0};}));
  check(w.publish_local_cooldowns(f.source,later));
 }
 std::cout<<"PASS Flourish actual cast/reset/Energy,own recharge,checkpoint,new attack and cancel/interrupt/missing hooks/stale permit rollback\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
