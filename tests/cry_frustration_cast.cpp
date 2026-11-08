#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cry cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);check(w.cast_activity(target,1,100,3100));
  casting::Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy=[&](unsigned cost){check(cost==10);return w.consume_energy(1,cost,owner);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};
  h.before_spell_effect=[&](unsigned skill,auto event,auto now){return w.before_spell_effect(owner,skill,event,now);};
  h.cry_frustration=[&](auto selected,const auto& plan,auto now){++commits;return w.cry_frustration(owner,selected,plan,20,now);};
  if(mode==4)h.cry_frustration={};if(mode==5)check(w.status_mutation(target,16,123,15,100,10000,true));
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[57]=1;Slots bar{};bar[0]={57,0};attributes::Ranks ranks{};ranks[2]=12;casting::Runtime cast(1);
  check(cast.start(100,{57,0,2},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4){check(w.resources(1)->energy==100);continue;}
  check(w.resources(1)->energy==90);cast.update(349,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(349,h);
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},349,2);w.set_vitals(2,{500,500,100,100},100);}
  cast.update(350,h);check(!cast.active());
  check(commits==(mode==0||mode==5?1u:0u));check(w.resources(2)->health==(mode==0||mode==5?437u:500u));
  if(mode==0||mode==5){check(w.resources(3)->health==437);check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{57,0},15))==1);check(bool(w.casting_skill(target,350))==(mode==5));}
 }
 std::cout<<"PASS Cry Runtime quarter-second/10Energy/15recharge, area damage, selected protection, cancel/interrupt/stale target/missing-hook rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
