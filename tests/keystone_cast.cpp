#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Keystone cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<10;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*60-60),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);Slots bar{};bar[0]={63,0};bar[1]={62,0};
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[63]=policy.learned[62]=1;attributes::Ranks ranks{};ranks[0]=12;
  casting::Hooks h;std::vector<world::Packet> sent;unsigned commits=0;
  h.diagnostic=[&](auto skill,const char* message){std::cout<<"mode="<<mode<<" skill="<<skill<<" phase="<<message<<'\n';};
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,owner,now);};h.consume_energy=[&](auto cost){return w.consume_energy(1,cost,owner);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,float,bool){return id==1;};h.projectile_target_valid=[](auto id,float){return id==2;};
  h.skill_disabled_slot=[&](Slot slot,auto now){return w.skill_disabled_slot(owner,slot,now);};h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  h.prepare_keystone_reset=[&](auto source){return w.disable_reset_revision(source);};
  h.commit_keystone_reset=[&](auto source,const auto& plan,const auto& cleared,const auto& permit,auto event,auto now){++commits;
   if(mode==4)return false;if(mode==5)check(w.signet_humility(foe,owner,*effects::signet_humility(0),now));
   return w.keystone_signet(source,plan,cleared,permit,event,now);};
  h.keystone_skill_used=[&](const auto& use){return w.keystone_skill_used(use,20);};
  h.signet_humility=[&](auto target,const auto& plan,auto now){return w.signet_humility(owner,target,plan,now);};
  if(mode==3)h.commit_keystone_reset={};casting::Runtime cast(1);box->drain();
  const auto reason=cast.start(100,{63,0,2},bar,policy,ranks,true,h);check(reason==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==6)h.prepare_keystone_reset={};if(mode==7)h.commit_keystone_reset={};if(mode==8)h.current_skillbar={};if(mode==9)h.keystone_skill_used={};
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(200,h);cast.update(2000,h);
  if(mode){check(!w.keystone_snapshot(owner,2000));check(commits==((mode==4||mode==5)?1u:0u));continue;}
  const auto initial=w.keystone_snapshot(owner,2000);std::cout<<"commits="<<commits<<" charges="<<(initial?int(initial->remaining):-1)<<" energy="<<w.resources(1)->energy<<'\n';
  check(commits==1&&initial&&initial->remaining==5&&w.resources(1)->energy==100);
  world::Packet body;world::u16(body,158);world::u32(body,21);world::u32(body,1);world::u32(body,112);
  auto packets=box->drain();check(std::count(packets.begin(),packets.end(),body)==1&&std::count(sent.begin(),sent.end(),body)==0);
  check(cast.start(3000,{62,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(5000,h);
  check(w.resources(2)->health==500&&w.resources(3)->health==449&&w.keystone_snapshot(owner,5000)->remaining==4);
  check(cast.start(6000,{63,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Keystone Runtime initial cast/reset, post-primary Humility reaction, own recharge, cancel/interrupt/missing-hook and stale-permit failures\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
