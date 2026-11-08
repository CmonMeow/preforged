#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Hex Breaker cast regression line "+std::to_string(at.line()));
}
int main()try{using namespace skills;using namespace casting;
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Enemy",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);Hooks hooks;unsigned commits=0;
  std::vector<world::Packet> sent;hooks.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};
  hooks.resources=[&](unsigned id){return w.resources(id);};
  hooks.consume_energy=[&](unsigned amount){check(amount==5);return w.consume_energy(1,amount,owner);};
  hooks.target_valid=[](unsigned id,float,bool){return id==1;};
  hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};
  hooks.hex_breaker=[&](auto scope,const auto& plan,auto now){++commits;check(scope==owner&&plan.rank==12&&plan.duration==65000);return w.hex_breaker(scope,plan,20,now);};
  if(mode==1)hooks.hex_breaker={};if(mode==2)hooks.target_scope={};
  if(mode==3)check(w.kill(owner,100));
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[10]=1;
  Slots slots{};slots[0]={10,0};attributes::Ranks ranks{};ranks[2]=12;Runtime cast(1);
  const auto result=cast.start(100,{10,0,2},slots,policy,ranks,true,hooks);
  if(mode){check(result==(mode==3?casting::Reason::energy:casting::Reason::unsupported));check(!commits&&w.resources(1)->energy==100);continue;}
  check(result==casting::Reason::accepted&&!cast.active()&&commits==1&&w.resources(1)->energy==95);
  const auto active=*w.status_snapshot(owner,100);check(active.size()==1&&active[0].skill==10&&active[0].expires==65100);
  check(std::count(sent.begin(),sent.end(),generic(48,1,10))==1);
  check(std::count(sent.begin(),sent.end(),generic(21,1,21))==1);
  check(std::count(sent.begin(),sent.end(),generic(58,1,0))==0);
  check(std::count(sent.begin(),sent.end(),slot_event(228,1,{10,0},15))==1);
  check(cast.start(200,{10,0,1},slots,policy,ranks,true,hooks)==casting::Reason::recharge&&w.resources(1)->energy==95);
  check(w.backfire(foe,owner,*effects::backfire(28,12),20,200));check(w.resources(2)->health==461&&w.status_snapshot(owner,200)->empty());
  cast.update(15100,hooks);check(cast.start(15100,{10,0,1},slots,policy,ranks,true,hooks)==casting::Reason::accepted&&commits==2&&w.resources(1)->energy==90);
 }
 std::cout<<"PASS Hex Breaker actual instant activation, self scope, 5Energy/15recharge, body21, prevention, recast and fail-closed hooks/death\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

