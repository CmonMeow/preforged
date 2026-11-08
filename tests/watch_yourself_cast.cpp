#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Watch cast line "+std::to_string(at.line()));}
int main()try{
 for(unsigned requested:{348u,2858u})for(bool pvp:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{10000,10000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2);
  Slots bar{};bar[0]={requested,0};check(w.configure_adrenaline(owner,bar,100));
  if(mode!=2)for(unsigned n=0;n<4;++n)check(w.complete_to_limit(owner,*to_limit(20),100));
  casting::Runtime cast(1);casting::Hooks hooks;unsigned spends=0,completions=0;bool stale=false;
  hooks.resources=[&](auto id){return w.resources(id);};hooks.events=[&](const auto& packets){box->enqueue(packets);};
  hooks.consume_energy_at=[&](auto cost,auto at){check(cost==0);if(mode==3)stale=true;return w.consume_energy(1,cost,owner,at);};
  hooks.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(stale&&scope)++scope->generation;return scope;};hooks.target_valid=[](auto id,float,bool){return id==1;};
  hooks.consume_adrenaline=[&](auto slot,auto at){++spends;return w.consume_adrenaline(owner,slot,at);};hooks.reconcile_adrenaline=[&](auto at){w.reconcile_adrenaline(owner,at);};
  if(mode!=1)hooks.watch_yourself=[&](auto source,const auto& plan,auto at){++completions;std::function<void()> publish;
   const auto ok=mode!=4&&w.stage_watch_yourself_party(source,plan,at,&publish);return casting::EffectCompletion{ok,{}, {},std::move(publish)};};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[requested]=1;attributes::Ranks ranks{};ranks[21]=mode==5?21:rank;
  if(mode==6)check(w.gale(*w.effect_scope(3),owner,12,{},100));
  box->drain();const auto result=cast.start(100,{requested,0,3},bar,policy,ranks,true,hooks);
  if(mode==1||mode==5||(requested==2858&&!pvp)){check(result==casting::Reason::unsupported&&spends==0&&completions==0);continue;}
  if(mode==2){check(result==casting::Reason::adrenaline&&spends==1&&completions==0);continue;}
  if(result!=casting::Reason::accepted)std::cerr<<"mode="<<mode<<" rank="<<rank<<" pvp="<<pvp<<" reason="<<int(result)<<"\n";
  check(result==casting::Reason::accepted&&!cast.active()&&spends==1&&w.adrenaline(owner)->front().units==0&&w.resources(1)->energy==100);
  if(mode==3||mode==4){check(w.watch_yourself_armor(owner,100)==0&&w.watch_yourself_armor(ally,100)==0);continue;}
  const auto plan=*watch_yourself(rank,pvp);check(completions==1&&w.watch_yourself_armor(owner,100)==plan.armor&&w.watch_yourself_armor(ally,100)==plan.armor);
  auto wire=box->drain();const auto instant=casting::generic(48,1,requested),body=*casting::skill_success_body_effect(1,requested);
  check(std::count(wire.begin(),wire.end(),instant)==1&&std::count(wire.begin(),wire.end(),body)==1);
  auto status=std::find_if(wire.begin(),wire.end(),[](const auto& p){return p.size()>1&&p[0]==65&&p[1]==0;});
  check(std::find(wire.begin(),wire.end(),instant)<std::find(wire.begin(),wire.end(),body)&&std::find(wire.begin(),wire.end(),body)<status&&status!=wire.end());
  check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{requested,0},4))==1&&std::count(wire.begin(),wire.end(),casting::terminal(58,1))==0);
  cast.update(101,hooks);check(completions==1&&cast.start(101,{requested,0,3},bar,policy,ranks,true,hooks)==casting::Reason::recharge&&spends==1);
 }
 std::cout<<"PASS Watch Yourself paid instant party casting,PvP plan,self normalization,recharge,scopes,missing hooks,rank guards and ordered48/body596/status publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
