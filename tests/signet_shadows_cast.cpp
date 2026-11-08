#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Shadows cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<5;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
  w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  Policy policy;policy.primary=7;policy.campaigns=0x1e;policy.learned[876]=1;
  Slots bar{};bar[0]={876,0};attributes::Ranks ranks{};ranks[30]=rank;
  casting::Hooks h;std::vector<world::Packet> packets;std::uint64_t at=1000;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());w.skill_event(1,batch,at);};
  h.resources=[&](unsigned actor){return w.resources(actor);};
  h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
  h.consume_energy_at=[&](unsigned cost,auto now){check(cost==0);return w.consume_energy(1,cost,source,now);};
  h.projectile_target_valid=[](unsigned actor,float range){return actor==2&&range==1248;};
  h.before_spell_effect=[](unsigned,std::uint64_t,std::uint64_t)->bool{throw std::runtime_error("signet triggered spell hook");};
  if(mode!=4)h.signet_shadows=[&](auto selected,const auto& plan,auto now)->casting::EffectCompletion{
   std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool ok=w.signet_shadows(source,selected,plan,20,now,&visuals,&publication);
   return {ok,std::move(visuals),{},std::move(publication),true};
  };
  casting::Runtime runtime(1);
  check(runtime.start(1000,{876,0,2},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4)continue;
  if(mode==1||mode==2)check(w.apply_condition(source,target,combat::conditions::Kind::blind,1500,mode==2?500:5000));
  if(mode==3)runtime.cancel(h);
  at=2000;runtime.update(1999,h);check(w.resources(2)->health==1000);runtime.update(at,h);
  const auto damage=mode==3?0:5+2*rank+(mode==1?15+3*rank:0);
  check(w.resources(2)->health==1000-damage);
  if(mode!=3){
   const auto body=effects::signet_shadows_impact_2022(1,2);
   check(std::count(packets.begin(),packets.end(),body)==1);
   check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body));
   check(runtime.start(2750,{876,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }
 }
 std::cout<<"PASS Signet Shadows cast: zero energy, conditional expiry, cancellation, signet-only lifecycle, ordered single impact, recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
