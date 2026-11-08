// Actual792/PvP2868 typed Runtime and owned World completion.
#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Splinter cast line "+std::to_string(at.line())+context);}
int main()try{
 for(auto [id,pvp]:{std::pair{792u,false},std::pair{792u,true}})for(unsigned mode=0;mode<16;++mode){
  context=" id="+std::to_string(id)+" pvp="+std::to_string(pvp)+" mode="+std::to_string(mode);world::Instance w;
  w.join(1,"Ritualist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(1);check(w.energy_capacity(1,8,0,100,{80,-2}));
  w.join(2,"Ally",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);const auto target=*w.effect_scope(2);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet>wire;unsigned commits=0;std::optional<WeaponSpellPlan>completed;
  h.events=[&](const auto&p){wire.insert(wire.end(),p.begin(),p.end());};h.resources=[&](auto actor){return w.resources(actor);};h.target_scope=[&](auto actor){return w.effect_scope(actor);};
  h.target_valid=[&](auto actor,float,bool){return actor==2&&w.effect_scope(actor)==target;};
  h.consume_energy_at=[&](auto amount,auto at){check(amount==5u);return w.consume_energy(1,amount,source,at);};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
  h.weapon_spell_target_valid=[&](Scope recipient,const WeaponSpellPlan&plan,auto at){return mode!=14&&w.valid_weapon_spell_target(source,recipient,plan,at);};
  h.weapon_spell=[&](Scope recipient,const WeaponSpellPlan&plan,auto at){++commits;completed=plan;std::vector<Packet>visuals;std::function<void()>publication;
   const bool accepted=w.apply_weapon_spell(source,recipient,plan,at,&visuals,&publication);return casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};};
  Policy policy;policy.primary=8;policy.campaigns=255;policy.learned[id]=1;policy.pvp_match=pvp;Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};attributes::Ranks ranks{};ranks[33]=ranks[34]=ranks[36]=12;
  if(mode==3)h.weapon_spell={};if(mode==4)h.weapon_spell_target_valid={};if(mode==5)h.target_scope={};if(mode==6)h.before_spell_effect={};if(mode==15){policy.primary=2;policy.secondary=8;}
  const bool missing=mode>=3&&mode<=6||mode==15;const auto reason=runtime.start(1000,{id,0,2},bar,policy,ranks,true,h);
  check(reason==(missing?casting::Reason::unsupported:mode==14?casting::Reason::target:casting::Reason::accepted));
  if(missing||mode==14){check(w.resources(1)->energy==100&&commits==0);continue;}
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);if(mode==7)h.weapon_spell={};if(mode==8)h.weapon_spell_target_valid={};if(mode==9)h.target_scope={};if(mode==10)h.before_spell_effect={};
  if(mode==11){w.leave(2);w.join(2,"Replacement",50,0,std::make_shared<world::Outbox>(),0,{}, {},1500,1);w.set_vitals(2,{500,1000,100,100},100);check(*w.effect_scope(2)!=target);}
  if(mode==13){ranks[33]=ranks[34]=ranks[36]=0;policy.primary=2;policy.secondary=8;check(w.energy_capacity(1,2,0,1500,{80,-2}));}
  runtime.update(1999,h);check(commits==0);runtime.update(2000,h);if(mode==12){runtime.update(2000,h);runtime.update(2100,h);}
  const bool success=mode==0||mode==12||mode==13;check(commits==unsigned(success));check(w.resources(1)->energy==95u);
  const auto current=*w.effect_scope(2);const auto state=w.status_snapshot(current,2000);check(bool(state)&&state->size()==unsigned(success));
  const unsigned effect=pvp?2868:id;const auto*row=find_skill(effect);
  if(success){check(completed&&completed->skill==effect&&completed->rank==12&&completed->primary==8&&completed->spawning==12);
   const unsigned duration=30000;check(state->front().skill==effect&&state->front().expires==2000+duration);
   check(std::count(wire.begin(),wire.end(),casting::generic(21,2,row->target_body_resource))==0);
   check(std::count(wire.begin(),wire.end(),casting::generic(21,2,1155))==1);
   check(std::count(wire.begin(),wire.end(),casting::generic(21,1,1282))==unsigned(id==787));
   check(std::count(wire.begin(),wire.end(),casting::terminal(58,1))==1);
  }else check(std::count(wire.begin(),wire.end(),casting::terminal(59,1))==1);
  check(std::count(wire.begin(),wire.end(),casting::generic(21,1,2077))==0&&std::count(wire.begin(),wire.end(),casting::generic(21,2,2077))==0);
  if(mode==0||mode==12){check(runtime.start(2749,{id,0,2},bar,policy,ranks,true,h)==casting::Reason::busy);check(runtime.start(2750,{id,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);const auto deadline=2000+row->recharge*1000;check(runtime.start(deadline-1,{id,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);check(runtime.start(deadline,{id,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);runtime.cancel(h);}
 }
 std::cout<<"Splinter typed Runtime32 lifecycle cases passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
