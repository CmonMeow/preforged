#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
static void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Signet of Corruption cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=3;++id){w.join(id,"Actor",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{},0,100,id==1?1:2);
   w.set_vitals(id,{500,500,0,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.degeneration_hex(owner,target,*effects::degeneration_hex(101,0),100));
  casting::Runtime cast(1);casting::Hooks h;std::vector<Packet> wire;
  h.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,owner,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(owner,id,range,100);};
  h.allegiance_rank=[&](unsigned){return std::optional<unsigned>{0};};
  h.signet_of_corruption=[&](auto foe,const auto& plan,auto at){return w.signet_of_corruption(owner,foe,plan,20,at);};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[2093]=1;policy.pvp_match=mode==4;
  Slots bar{};bar[0]={2093,0};attributes::Ranks ranks{};
  if(mode==3)h.signet_of_corruption={};
  const auto reason=cast.start(100,{2093,0,2},bar,policy,ranks,true,h);
  if(mode>=3){check(reason==(mode==3?casting::Reason::unsupported:casting::Reason::pve_only));continue;}
  if(reason!=casting::Reason::accepted)std::cerr<<"mode "<<mode<<" reason "<<unsigned(reason)<<'\n';
  check(reason==casting::Reason::accepted);
  if(mode==1)cast.cancel(h);
  if(mode==2){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{},0,100,2);w.set_vitals(2,{500,500,0,100},100);}
  cast.update(1099,h);check(w.resources(2)->health==500);
  cast.update(1100,h);
  check(w.resources(2)->health==(mode?500u:480u));
  check(w.resources(3)->health==(mode?500u:480u));
  if(!mode){
   check(w.resources(1)->energy==2);
   const auto body=casting::success_body_effect(1,2093);
   check(body&&std::count(wire.begin(),wire.end(),*body)==1);
   check(cast.start(2000,{2093,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }
 }
 std::cout<<"PASS Signet of Corruption cast/rank/PvE admission,completion,cancel,stale target and recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
