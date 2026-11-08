#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Power Block casting regression line "+std::to_string(at.line()));}
int main()try{using namespace skills;using namespace casting;
 for(unsigned mode=0;mode<10;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Target",60,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  if(mode!=9)check(w.cast_activity(target,28,100,3100));
  if(mode==8)check(w.status_mutation(target,16,3000,0,100,10000,true));
  const auto epoch=w.control_epoch(target);Hooks hooks;unsigned commits=0;std::vector<world::Packet> sent;
  hooks.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};
  hooks.resources=[&](unsigned id){return w.resources(id);};hooks.consume_energy=[&](unsigned amount){return w.consume_energy(1,amount,source);};
  hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};hooks.control_epoch=[&]{return w.control_epoch(source);};
  hooks.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};
  hooks.interrupt_skill=[&](auto selected,const auto& plan,auto now){++commits;return w.interrupt_skill(source,selected,plan,20,now);};
  hooks.skill_disabled_until=[&](unsigned skill,auto now){return w.skill_disabled_until(source,skill,now);};
  if(mode==4)hooks.interrupt_skill={};
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[5]=1;Slots slots{};slots[0]={5,0};attributes::Ranks ranks{};ranks[2]=12;Runtime cast(1);
  hooks.current_skillbar=[&]()->std::optional<Slots>{return slots;};
  check(cast.start(100,{5,0,2},slots,policy,ranks,true,hooks)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(w.resources(1)->energy==(mode==4?100u:85u));if(mode==4)continue;
  cast.update(199,hooks);check(!commits);
  if(mode==1)cast.cancel(hooks);if(mode==2)cast.interrupt(200,hooks);
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,box,0,{}, {},200,2);w.set_vitals(2,{500,500,100,100},100);}
  if(mode==5)check(w.kill(source,200));if(mode==6)check(w.kill(target,200));
  if(mode==7){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{500,500,100,100},100);}
  cast.update(349,hooks);check(!commits);box->drain();cast.update(350,hooks);cast.update(351,hooks);
  const bool applied=mode==0||mode==8,completed=applied||mode==9;
  check(commits==(completed?1u:0u));
  check(w.skill_disabled_until(*w.effect_scope(2),23,351)==(applied?10350u:0u));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,5))==(completed?1:0));
  if(completed)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{5,0},20))==1);
  if(applied){
   check((w.control_epoch(target)==epoch)==(mode==8));
   Runtime victim(2);Hooks ui;Slots bar{};bar[0]={23,0};bar[1]={28,0};std::vector<world::Packet> feedback;
   ui.events=[&](const auto& batch){feedback.insert(feedback.end(),batch.begin(),batch.end());};
   ui.current_skillbar=[&]()->std::optional<Slots>{return bar;};ui.skill_disabled_until=[&](unsigned skill,auto now){return w.skill_disabled_until(target,skill,now);};
   victim.update(351,ui);check(feedback.size()==2&&feedback[0]==slot_event(228,2,{23,0},10)&&feedback[1]==slot_event(228,2,{28,0},10));
  }
  if(mode==1)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{5,0},20))==0);
  if(mode==2)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{5,0},20))==1);
 }
 std::cout<<"PASS actual Power Block15Energy/.25second/20recharge,protected/idle casts,targetbody11,victim UI,cancel/interrupt and actor lifetime guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
