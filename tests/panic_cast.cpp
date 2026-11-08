#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Panic cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(unsigned mode=0;mode<7;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);casting::Hooks h;unsigned applied=0,observed=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};h.consume_energy=[&](unsigned cost){check(cost==10);return w.consume_energy(1,cost,owner);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};h.before_spell_effect=[&](unsigned skill,auto event,auto now){return w.before_spell_effect(owner,skill,event,now);};
  h.panic=[&](auto target,unsigned rank,auto now){++applied;return mode==6?false:w.apply_panic(owner,target,rank,now);};
  h.panic_skill_used=[&](const auto& use){++observed;check(use.owner==owner&&use.skill==52&&use.event&&use.began==100&&use.used==1100&&use.success);return w.panic_skill_used(use);};
  if(mode==4)h.panic_skill_used={};Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[52]=1;Slots bar{};bar[0]={52,0};attributes::Ranks ranks{};ranks[2]=12;casting::Runtime cast(1);
  check(cast.start(100,{52,0,2},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));if(mode==4){check(w.resources(1)->energy==100);continue;}
  check(w.resources(1)->energy==90);cast.update(1099,h);check(!applied&&!observed);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{500,500,100,100},100);}
  if(mode==5){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},1099,2);w.set_vitals(2,{500,500,100,100},100);}
  box->drain();cast.update(1100,h);check(!cast.active()&&applied==(mode==0||mode==6?1u:0u)&&observed==(mode==0?1u:0u));
  if(mode==0){const auto status=*w.status_snapshot(*w.effect_scope(2),1100);check(status.size()==1&&status.front().skill==52&&status.front().expires==9100);const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,52))==1);check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{52,0},15))==1);}
 }
 std::cout<<"PASS Panic actual1second/10Energy/15recharge cast, captured body87, owned successful-use timing and cancel/interrupt/lifetime/failure guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
