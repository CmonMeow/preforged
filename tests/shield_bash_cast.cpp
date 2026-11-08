#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Shield Bash cast line "+std::to_string(at.line()));
}
int main()try{
 {warrior_test::Fixture f(340,false);auto& w=f.world;casting::Runtime runtime(1);casting::Hooks h;
  unsigned spent=0;std::vector<items::Packet> packets;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy_at=[&](auto n,auto at){spent+=n;return w.consume_energy(1,n,f.source,at);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};
  h.status_mutation=[&](auto s,auto e,auto r,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,r,at,until,active);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[363]=1;Slots bar{};bar[0]={363,0};attributes::Ranks ranks{};ranks[17]=12;
  check(runtime.start(1000,{363,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);check(spent==5);
  const auto status=*w.status_snapshot(f.source,1000);check(status.size()==1&&status[0].skill==363&&status[0].expires==11000);
  check(std::count(packets.begin(),packets.end(),casting::generic(48,1,363))==1&&std::count(packets.begin(),packets.end(),casting::generic(21,1,621))==1);
  check(runtime.start(1001,{363,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
  runtime.update(11000,h);check(w.status_snapshot(f.source,11000)->empty());
 }
 // Actual attack Runtime imports a World completion with base3s +15s after
 // its own terminal block knocks it down. No independent-disable floor is used.
 {warrior_test::Fixture f(322,false);auto& w=f.world;f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));
  check(w.shield_equipment(f.target,ShieldEquipment{1,55},12,100));check(w.status_mutation(f.target,363,110,0,100,5100,true));
  Slots bar{};bar[0]={322,7};check(w.configure_adrenaline(f.source,bar,100));
  casting::Runtime runtime(1);casting::Hooks h;std::uint64_t use=0,clock=1000;std::vector<items::Packet> packets;
  h.events=[&](const auto& b){packets.insert(packets.end(),b.begin(),b.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.consume_energy_at=[&](auto n,auto at){return w.consume_energy(1,n,f.source,at);};
  h.weapon_snapshot=[&](){return std::optional(f.weapon);};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,322);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){return w.melee_attack(f.source,target,weapon,plan,at);};
  h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};h.attack_duration=[&](auto base,auto at){return w.attack_duration(f.source,base,at);};
  h.attack_activity=[&](auto until){w.attack_activity(f.source,until,clock);};
  h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};h.control_epoch=[&](){return w.control_epoch(f.source);};
  h.prepare_skill_use=[&](auto slot,auto event,auto at){use=event;return w.prepare_diversion_skill_use(f.source,slot,event,at);};
  h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(f.source,event);};
  h.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){return w.finalize_diversion_skill_use(f.source,event,at,base,existing,instant);};
  h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(f.source,event);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[322]=8;attributes::Ranks ranks{};
  const auto admission=runtime.start(1000,{322,7,2},bar,p,ranks,true,h);
  if(admission!=casting::Reason::accepted)throw std::runtime_error("Shield Bash attacking admission reason="+std::to_string(static_cast<unsigned>(admission)));
  clock=1500;runtime.update(clock,h);
  check(w.knocked_down(f.source,1500)&&w.resources(2)->health==1000&&w.status_snapshot(f.target,1500)->empty());
  const auto receipt=w.cooldown_completion_stamp(f.source,use);check(receipt&&receipt->base_ms==3000&&receipt->extra_ms==15000&&completion_deadline(*receipt)==19500);
  check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{322,7},18))>=1);
  check(w.skill_disabled_slot(f.source,{322,7},1500)==0);
  runtime.update(1500,h);const auto local=runtime.cooldown_source_snapshot(f.source,1500,h);
  check(local&&local->timers.size()==1&&local->timers[0].until==19500&&w.publish_local_cooldowns(f.source,*local));
  check(runtime.start(4000,{322,7,2},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Shield Bash normal instant Runtime and completed attacking use under its own knockdown with18s composed base recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
