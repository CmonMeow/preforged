#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Humility cast regression line "+std::to_string(at.line()));}
int main()try{using namespace skills;using namespace casting;
 for(unsigned mode=0;mode<8;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Foe",60,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);Hooks hooks;unsigned commits=0;std::vector<world::Packet> sent;
  hooks.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};hooks.resources=[&](unsigned id){return w.resources(id);};
  hooks.consume_energy=[&](unsigned amount){check(amount==0);return w.consume_energy(1,amount,source);};
  hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};hooks.control_epoch=[&]{return w.control_epoch(source);};
  hooks.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};
  hooks.signet_humility=[&](auto selected,const auto& plan,auto now){++commits;check(plan.rank==12);return w.signet_humility(source,selected,plan,now);};
  hooks.skill_disabled_until=[&](unsigned skill,auto now){return w.skill_disabled_until(source,skill,now);};
  if(mode==4)hooks.signet_humility={};
  Policy policy;policy.primary=5;policy.secondary=3;policy.campaigns=255;Slots bar{};
  bar[0]={62,0};bar[1]={281,0};bar[2]={3,0};bar[3]={5,0};for(const auto& slot:bar)if(slot.skill)policy.learned[slot.skill]=1;
  hooks.current_skillbar=[&]()->std::optional<Slots>{return bar;};attributes::Ranks ranks{};ranks[3]=12;Runtime cast(1);
  check(cast.start(100,{62,0,2},bar,policy,ranks,true,hooks)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(w.resources(1)->energy==100);if(mode==4)continue;
  cast.update(199,hooks);check(!commits);
  if(mode==1)cast.cancel(hooks);if(mode==2)cast.interrupt(200,hooks);
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,box,0,{}, {},200,2);w.set_vitals(2,{500,500,100,100},100);}
  if(mode==5)check(w.kill(source,200));if(mode==6)check(w.kill(target,200));
  if(mode==7){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{500,500,100,100},100);}
  cast.update(3099,hooks);check(!commits);box->drain();cast.update(3100,hooks);cast.update(3101,hooks);
  check(commits==(mode==0?1u:0u));
  check(w.skill_disabled_until(*w.effect_scope(2),5,3101)==(mode==0?16100u:0u));
  if(mode==0){
   check(w.skill_disabled_until(source,281,3101)==13100&&w.skill_disabled_until(source,3,3101)==13100&&w.skill_disabled_until(source,62,3101)==0);
   const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,62))==1);
   check(std::count(sent.begin(),sent.end(),slot_event(228,1,{62,0},20))==1&&std::count(sent.begin(),sent.end(),slot_event(228,1,{281,0},10))==1);
   check(cast.start(4000,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge&&w.resources(1)->energy==100);
   check(w.skill_disabled_until(target,5,16100)==0&&w.skill_disabled_until(source,281,13100)==0);
  }
  if(mode==1)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{62,0},20))==0);
  if(mode==2)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{62,0},20))==1);
 }
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Source",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Target",mode==1?2000.f:60.f,0,box,0,{}, {},100,mode==2?1:2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);auto selected=target;auto plan=*effects::signet_humility(12);
  if(mode==3)++selected.generation;if(mode==4)check(w.kill(target,150));if(mode==5)plan.self_disable=0;
  box->drain();check(w.signet_humility(source,selected,plan,200)==(mode==0));
  check(w.skill_disabled_until(source,281,200)==(mode==0?10200u:0u)&&w.skill_disabled_until(target,5,200)==(mode==0?13200u:0u));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,62))==(mode==0?1:0));
  if(mode==0){
   check(w.cast_activity(target,28,300,3300)&&w.interrupt_skill(source,target,*effects::interrupt_skill(5,20),20,400));
   check(w.skill_disabled_until(target,5,400)==16400&&w.skill_disabled_until(target,259,400)==13200);
   auto stale=target;++stale.generation;check(!w.signet_humility(source,stale,*effects::signet_humility(20),500));
   check(w.skill_disabled_until(source,281,500)==10200&&w.skill_disabled_until(target,259,500)==13200);
  }
 }
 std::cout<<"PASS Humility actual zeroEnergy/3second/20recharge cast,paired disable/body111/selfUI,cancel/interrupt/lifetime and atomic World rejection/PowerBlock overlap\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
