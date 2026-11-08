#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Charge cast line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id*30.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{500,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2);check(w.apply_condition(owner,ally,combat::conditions::Kind::crippled,100,10000));
  check(w.complete_vigorous_spirit(owner,owner,*vigorous_spirit(12),0,100));
  casting::Runtime cast(1);casting::Hooks h;unsigned spends=0,completions=0;bool stale=false;
  h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool){return id==1;};h.target_scope=[&](auto id)->std::optional<Scope>{auto s=w.effect_scope(id);if(s&&stale)++s->generation;return s;};
  h.consume_energy_at=[&](unsigned amount,auto at){++spends;check(amount==5);if(mode==2)return false;const bool ok=w.consume_energy(1,amount,owner,at);if(mode==3)stale=true;return ok;};h.events=[&](const auto& packets){box->enqueue(packets);};
  h.completed_action=[&](auto skill,auto at){return w.completed_skill_action(owner,skill,at);};
  if(mode!=1)h.charge=[&](auto source,const auto& plan,auto at){++completions;std::function<void()> publish;const bool ok=mode!=4&&w.stage_charge_allies(source,plan,at,&publish);return casting::EffectCompletion{ok,{},{},std::move(publish)};};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[364]=1;Slots bar{};bar[0]={364,0};attributes::Ranks ranks{};ranks[21]=mode==5?21:rank;
  if(mode==6)check(w.gale(*w.effect_scope(3),owner,12,{},100));box->drain();const auto result=cast.start(100,{364,0,3},bar,policy,ranks,true,h);
  if(mode==1||mode==5){check(result==casting::Reason::unsupported&&spends==0&&completions==0);continue;}
  if(mode==2){check(result==casting::Reason::energy&&spends==1&&completions==0&&w.resources(1)->energy==100);continue;}
  if(result!=casting::Reason::accepted||cast.active()||spends!=1||w.resources(1)->energy!=95)std::cerr<<"mode="<<mode<<" pvp="<<pvp<<" rank="<<rank<<" reason="<<unsigned(result)<<" active="<<cast.active()<<" spends="<<spends<<" energy="<<w.resources(1)->energy<<'\n';
  check(result==casting::Reason::accepted&&!cast.active()&&spends==1&&w.resources(1)->energy==95);
  const auto charged=[&](Scope target){const auto list=*w.status_snapshot(target,100);return std::any_of(list.begin(),list.end(),[](auto e){return e.skill==364;});};
  if(mode==3||mode==4){check(!charged(owner)&&!charged(ally)&&w.has_condition(ally,combat::conditions::Kind::crippled,100));continue;}
  check(completions==1&&charged(owner)&&charged(ally)&&!w.has_condition(ally,combat::conditions::Kind::crippled,100));check(w.resources(1)->health==500); // Shout is not a spell.
  const auto wire=box->drain();const auto instant=casting::generic(48,1,364),body=*casting::skill_success_body_effect(1,364);
  check(std::count(wire.begin(),wire.end(),instant)==1&&std::count(wire.begin(),wire.end(),body)==1);
  const auto status=std::find_if(wire.begin(),wire.end(),[](const auto& p){return p.size()==20&&p[0]==65&&p[6]==108&&p[7]==1;});
  check(status!=wire.end()&&std::find(wire.begin(),wire.end(),instant)<std::find(wire.begin(),wire.end(),body)&&std::find(wire.begin(),wire.end(),body)<status);
  check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{364,0},20))==1&&std::count(wire.begin(),wire.end(),casting::terminal(58,1))==0);
  cast.update(101,h);check(completions==1&&cast.start(101,{364,0,3},bar,policy,ranks,true,h)==casting::Reason::recharge&&spends==1);
 }
 // The instant shout preserves a separately running spell;the heal is stubbed
 // to isolate continuation and completed-action delivery from Orison mechanics.
 for(bool pvp:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);const auto owner=*w.effect_scope(1);
  casting::Runtime cast(1);casting::Hooks h;unsigned shouts=0;
  h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool){return id==1;};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,owner,at);};h.events=[&](const auto& packets){box->enqueue(packets);};h.heal=[](unsigned,unsigned,float,bool){return true;};
  h.charge=[&](auto source,const auto& plan,auto at){++shouts;std::function<void()> publish;const bool ok=w.stage_charge_allies(source,plan,at,&publish);return casting::EffectCompletion{ok,{},{},std::move(publish)};};
  Policy policy;policy.primary=1;policy.secondary=3;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[281]=policy.learned[364]=1;Slots bar{};bar[0]={364,0};bar[1]={281,0};attributes::Ranks ranks{};ranks[21]=12;ranks[13]=12;
  check(cast.start(100,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted&&cast.active());
  check(cast.start(200,{364,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted&&cast.active()&&shouts==1&&w.resources(1)->energy==90);
  cast.update(1100,h);check(!cast.active()&&shouts==1);
 }
 std::cout<<"PASS44 Charge paid instant/PvP/self-normalized casting,allies outside party,Crippled cleanse,KD use,scope/cost/hooks/rank/recharge,48/622/status publication and no Vigorous spell heal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
