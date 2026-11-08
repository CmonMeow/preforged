#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Defy cast line "+std::to_string(at.line()));}
int main()try{
 for(unsigned requested:{318u,3204u})for(bool pvp:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);
  w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);Slots bar{};bar[0]={requested,0};check(w.configure_adrenaline(owner,bar,100));
  if(mode!=2)for(unsigned n=0;n<6;++n)check(w.complete_to_limit(owner,*to_limit(20),100));
  const auto before=*w.resources(1);
  casting::Runtime cast(1);casting::Hooks hooks;unsigned spends=0,completions=0;bool stale=false;
  hooks.resources=[&](auto id){return w.resources(id);};hooks.events=[&](const auto& packets){box->enqueue(packets);};
  hooks.consume_energy_at=[&](auto cost,auto at){check(cost==0);if(mode==3)stale=true;return w.consume_energy(1,cost,owner,at);};
  hooks.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(stale&&scope)++scope->generation;return scope;};hooks.target_valid=[](auto id,float,bool){return id==1;};
  hooks.effect_snapshot=[&](auto at){return w.status_snapshot(owner,at);};
  hooks.consume_adrenaline=[&](auto slot,auto at){++spends;return w.consume_adrenaline(owner,slot,at);};hooks.reconcile_adrenaline=[&](auto at){w.reconcile_adrenaline(owner,at);};
  if(mode!=1)hooks.defy_pain=[&](auto source,const auto& plan,auto at){++completions;check(plan.stance==pvp&&plan.skill==(pvp?3204u:318u));std::function<void()> publish;
   const auto ok=mode!=4&&w.stage_defy_pain_health(source,plan,at,&publish);return casting::EffectCompletion{ok,{}, {},std::move(publish)};};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[requested]=1;attributes::Ranks ranks{};ranks[17]=mode==5?21:rank;
  if(mode==6)check(w.gale(*w.effect_scope(2),owner,12,{},100));
  box->drain();const auto result=cast.start(100,{requested,0,2},bar,policy,ranks,true,hooks);
  if(mode==1||mode==5||(requested==3204&&!pvp)){check(result==casting::Reason::unsupported&&spends==0&&completions==0);continue;}
  if(mode==2){check(result==casting::Reason::adrenaline&&spends==1&&completions==0);continue;}
  check(result==casting::Reason::accepted&&!cast.active()&&spends==1&&w.adrenaline(owner)->front().units==0&&w.resources(1)->energy==100);
  if(mode==3||mode==4){check(w.resources(1)->max_health==before.max_health);continue;}
  const auto plan=*defy_pain(rank,pvp);check(completions==1&&w.resources(1)->max_health==before.max_health+plan.health_bonus&&w.resources(1)->health==before.health+plan.health_bonus);
  const auto wire=box->drain();const auto instant=casting::generic(48,1,requested),body=*casting::skill_success_body_effect(1,requested);
  check(std::count(wire.begin(),wire.end(),instant)==1&&std::count(wire.begin(),wire.end(),body)==1);
  const auto status=std::find_if(wire.begin(),wire.end(),[](const auto& p){return p.size()>1&&p[0]==65&&p[1]==0;});
  check(std::find(wire.begin(),wire.end(),instant)<std::find(wire.begin(),wire.end(),body)&&std::find(wire.begin(),wire.end(),body)<status&&status!=wire.end());
  check(std::count(wire.begin(),wire.end(),casting::terminal(58,1))==0);
  cast.update(101,hooks);check(completions==1&&cast.start(101,{requested,0,2},bar,policy,ranks,true,hooks)==casting::Reason::adrenaline);
 }
 for(bool pvp:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);
  w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);Slots bar{};bar[0]={346,0};bar[1]={318,0};bar[2]={1043,0};check(w.configure_adrenaline(owner,bar,100));
  const auto charge=[&](auto at){for(unsigned n=0;n<6;++n)check(w.complete_to_limit(owner,*to_limit(20),at));};charge(100);
  casting::Runtime cast(1);casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){box->enqueue(packets);};
  h.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(1,cost,owner,at);};h.target_valid=[](auto id,float,bool){return id==1;};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.effect_snapshot=[&](auto at){return w.status_snapshot(owner,at);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto until,bool active){return w.status_mutation(owner,skill,effect,rank,at,until,active);};
  h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(owner,slot,at);};h.reconcile_adrenaline=[&](auto at){w.reconcile_adrenaline(owner,at);};
  h.defy_pain=[&](auto source,const auto& plan,auto at){std::function<void()> publish;const bool ok=w.stage_defy_pain_health(source,plan,at,&publish);return casting::EffectCompletion{ok,{}, {},std::move(publish)};};
  h.speed_multiplier=[](double){return true;};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.pvp_match=pvp;policy.secondary=7;policy.learned={{346,1},{318,1},{1043,1}};attributes::Ranks ranks{};ranks[17]=12;ranks[21]=12;
  check(cast.start(100,{346,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
  auto active=*w.status_snapshot(owner,100);const auto defensive=std::find_if(active.begin(),active.end(),[](auto e){return e.skill==346;});check(defensive!=active.end());const auto old_id=defensive->instance;
  charge(101);const auto base=w.resources(1)->max_health;box->drain();
  const auto result=cast.start(101,{318,0,0},bar,policy,ranks,true,h);if(result!=casting::Reason::accepted)std::cerr<<"replacement pvp="<<pvp<<" reason="<<int(result)<<"\n";check(result==casting::Reason::accepted);
  if(w.resources(1)->max_health!=base+258)std::cerr<<"replacement max="<<w.resources(1)->max_health<<" base="<<base<<" pvp="<<pvp<<"\n";check(w.resources(1)->max_health==base+258);
  active=*w.status_snapshot(owner,101);check(std::any_of(active.begin(),active.end(),[&](auto e){return e.skill==(pvp?3204u:318u);})&&std::any_of(active.begin(),active.end(),[](auto e){return e.skill==346;})==!pvp);
  auto wire=box->drain();check(std::count(wire.begin(),wire.end(),casting::effect_remove(1,old_id))==(pvp?1:0));
  check(cast.start(102,{1043,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);check(w.resources(1)->max_health==base+(pvp?0:258));
  wire=box->drain();check(std::count(wire.begin(),wire.end(),casting::effect_remove(1,old_id))==(pvp?0:1));
  w.tick(8300);cast.update(8300,h);wire=box->drain();check(std::count(wire.begin(),wire.end(),casting::effect_remove(1,old_id))==0);
 }
 std::cout<<"PASS Defy Pain paid120Adrenaline instant PvE/PvP casting,rank/alias guards,scopes,knockdown,stance replacement and ordered body595/status\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
