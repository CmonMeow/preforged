#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Discord cast line "+std::to_string(at.line()));}
int main()try{
 for(unsigned variant=0;variant<3;++variant)for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<6;++mode){
  const bool pvp=variant>0;const unsigned skill=variant==2?2863:817;
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Necro",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.status_mutation(target,99,800,0,1000,mode==5?1500:11000,true));
  Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned[skill]=1;policy.pvp_match=pvp;
  Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[5]=rank;
  casting::Hooks h;std::vector<world::Packet> packets;std::uint64_t at=1000;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());w.skill_event(1,batch,at);};
  h.resources=[&](unsigned actor){return w.resources(actor);};h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
  h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};
  h.consume_energy_at=[&](unsigned cost,auto now){return w.consume_energy(1,cost,source,now);};
  if(mode!=2)h.discord=[&](auto selected,const auto& plan,auto now)->casting::EffectCompletion{
   std::vector<world::Packet> visuals;std::function<void()> publication;const bool ok=w.discord(source,selected,plan,20,now,&visuals,&publication);return {ok,std::move(visuals),{},std::move(publication),true};};
  if(mode==1||mode==5)check(w.apply_condition(source,target,combat::conditions::Kind::blind,1000,10000));
  if(mode==4)h.before_spell_effect=[&](unsigned,std::uint64_t,auto now){return w.apply_condition(source,target,combat::conditions::Kind::blind,now,10000);};
  casting::Runtime runtime(1);check(runtime.start(1000,{skill,0,2},bar,policy,ranks,true,h)==(mode==2?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==2){check(w.resources(1)->energy==100);continue;}check(w.resources(1)->energy==95);
  if(mode==3)runtime.cancel(h);
  at=pvp?3000:2000;runtime.update(at-1,h);check(w.resources(2)->health==1000);runtime.update(at,h);
  const double damage=mode==1||mode==4?effects::damage_scale(30,110,rank):0;
  check(std::abs(double(w.resources(2)->health)-(1000-damage))<1.1);
  if(mode!=3){const auto body=*particles::targeted_body_2022(2,1,skill);
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1)),effect=std::find(packets.begin(),packets.end(),body);
   check(finish!=packets.end()&&effect!=packets.end()&&finish<effect);
   const auto outgoing=box->drain();const auto particle=std::find(outgoing.begin(),outgoing.end(),body);
   const auto damage_packet=std::find_if(outgoing.begin(),outgoing.end(),[](const auto& p){return p.size()>=18&&p[0]==162&&p[2]==55&&p[3]==0&&p[4]==0&&p[5]==0;});
   if(mode==1||mode==4)check(particle!=outgoing.end()&&damage_packet!=outgoing.end()&&particle<damage_packet);
   else check(damage_packet==outgoing.end());
   check(runtime.start(at+750,{skill,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }
 }
 std::cout<<"PASS Discord normal casting54mode/rank/variant contexts:predicate/no-effect,cost,PvP timing,cancel,pre-effect condition,expired-hex suppression,visual order,recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
