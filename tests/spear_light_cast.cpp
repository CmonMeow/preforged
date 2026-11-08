#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Spear cast line "+std::to_string(at.line()));}
int main()try{
 unsigned cases=0;
 for(bool pvp:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<5;++mode){
  world::Instance w;w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",900,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  auto owner=*w.effect_scope(1);check(w.player_holy_equipment(*w.effect_scope(2),0)&&w.configure_projectiles({1800,32}));
  Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[1130]=1;policy.pvp_match=pvp;
  Slots bar{};bar[0]={1130,0};attributes::Ranks ranks{};ranks[14]=rank;
  casting::Hooks h;std::vector<world::Packet> packets;unsigned launches=0;std::uint64_t time=1000;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());w.skill_event(1,batch,time);};
  h.resources=[&](unsigned id){return w.resources(id);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};
  h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};
  h.consume_energy_at=[&](unsigned cost,auto now){return w.consume_energy(1,cost,owner,now);};
  if(mode!=2)h.spear_light=[&](auto target,const auto& plan,auto now)->casting::EffectCompletion{
   ++launches;if(mode==3)return {false};std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool ok=w.spear_light(owner,target,plan,20,now,&visuals,&publication);
   return {ok,std::move(visuals),{},std::move(publication),true};};
  casting::Runtime runtime(1);check(runtime.start(1000,{1130,0,2},bar,policy,ranks,true,h)==(mode==2?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==2){check(w.resources(1)->energy==100);continue;}
  check(w.resources(1)->energy==95);
  if(mode==1)runtime.cancel(h);
  if(mode==4){w.leave(2);w.join(2,"Reused",900,0,std::make_shared<world::Outbox>(),0,{}, {},1500,2);w.set_vitals(2,{1000,1000,100,100},100);}
  time=2000;runtime.update(time,h);check(w.resources(2)->health==1000);
  w.tick(2400);check(w.resources(2)->health==1000);w.tick(2500);
  if(mode==0){check(launches==1&&w.resources(2)->health==1000-(26+2*rank));
   auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   auto launch=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==29&&p[0]==163&&p[1]==128;});
   check(finish!=packets.end()&&launch!=packets.end()&&finish<launch);
   time=2750;check(runtime.start(time,{1130,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }else check(w.resources(2)->health==1000);
  ++cases;
 }
 std::cout<<"PASS Spear normal Runtime/World "<<cases<<" contexts plus missing-hook rejection; modes,cost,finish-before-launch,cancel,lifetime,delayed damage,recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
