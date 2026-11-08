#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Endure cast line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);
  w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(1);
  check(w.stage_to_limit_health(owner,*to_limit(0),100));if(mode==6)check(w.gale(*w.effect_scope(2),owner,12,{},100));
  casting::Runtime cast(1);casting::Hooks hooks;unsigned payments=0,completions=0;bool stale=false;
  hooks.resources=[&](auto id){return w.resources(id);};hooks.events=[&](const auto& p){box->enqueue(p);};
  hooks.consume_energy_at=[&](auto cost,auto at){++payments;check(cost==5);if(mode==2)return false;const auto paid=w.consume_energy(1,cost,owner,at);if(mode==3)stale=true;return paid;};
  hooks.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(stale&&scope)++scope->generation;return scope;};hooks.target_valid=[](auto id,float,bool){return id==1;};
  if(mode!=1)hooks.endure_pain=[&](auto source,const auto& plan,auto at){++completions;std::function<void()> publish;const bool ok=mode!=4&&w.stage_endure_pain_health(source,plan,at,&publish);return casting::EffectCompletion{ok,{}, {},std::move(publish)};};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[347]=1;Slots bar{};bar[0]={347,0};attributes::Ranks ranks{};ranks[17]=mode==5?21:rank;
  box->drain();const auto result=cast.start(100,{347,0,2},bar,policy,ranks,true,hooks);
  if(mode==1||mode==5){check(result==casting::Reason::unsupported&&payments==0&&completions==0);continue;}
  if(mode==2){check(result==casting::Reason::energy&&payments==1&&completions==0);continue;}
  check(result==casting::Reason::accepted&&!cast.active()&&payments==1&&w.resources(1)->energy==95);
  if(mode==3||mode==4){check(w.resources(1)->max_health==510);continue;}
  const auto plan=*endure_pain(rank);check(completions==1&&w.resources(1)->max_health==510+plan.health_bonus&&w.resources(1)->health==410+plan.health_bonus);
  const auto wire=box->drain();const auto instant=casting::generic(48,1,347),body=*casting::skill_success_body_effect(1,347);
  check(std::count(wire.begin(),wire.end(),instant)==1&&std::count(wire.begin(),wire.end(),body)==1);
  const auto status=std::find_if(wire.begin(),wire.end(),[](const auto& p){return p.size()>1&&p[0]==65&&p[1]==0;});
  check(std::find(wire.begin(),wire.end(),instant)<std::find(wire.begin(),wire.end(),body)&&std::find(wire.begin(),wire.end(),body)<status&&status!=wire.end());
  check(std::count(wire.begin(),wire.end(),casting::terminal(58,1))==0&&std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{347,0},30))==1);
  check(cast.start(101,{347,0,2},bar,policy,ranks,true,hooks)==casting::Reason::recharge&&payments==1);cast.update(101,hooks);check(completions==1);
 }
 std::cout<<"PASS Endure Pain paid instant PvE/PvP casting,Health coexistence,knockdown,scopes,recharge and ordered body595/status publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
