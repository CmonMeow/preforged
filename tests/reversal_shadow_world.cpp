#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal shadow World line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;bool npc;
 Fixture(bool n,unsigned hp,unsigned maximum=1000):npc(n){
  if(npc){world::Instance::Npc body;body.id=body.definition.id=100;body.x=60;body.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={double(hp),maximum};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";body.combat_definition=d;w.configure_npcs({body});
  }else{w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,maximum,100,100},100);}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,100,100},100);source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
 void reversal(unsigned rank){if(npc)check(w.stage_npc_reversal(target,rank,100));else check(w.status_mutation(target,307,5,rank,100,8100,true));}
};
int main()try{
 unsigned cases=0;
 for(unsigned skill:{133u,219u,155u,158u,1078u,102u,1067u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,600u,1000u}){
  ++cases;Fixture f(npc,hp);f.reversal(rr);const auto plan=*skills::effects::shadow_damage(skill,12);f.box->drain();
  check(f.w.shadow_damage(f.source,f.target,plan,20,1000));
  const double converted=std::min(double(plan.damage),double(skills::effects::reversal_fortune(rr)->maximum_healing));
  double expected=std::max(0.,std::min(1000.,hp+converted)-(plan.damage-converted));double stolen=0;
  if(hp>500&&plan.steal_above_half&&expected>0){stolen=std::min(double(plan.steal_above_half),expected+1);expected=std::max(0.,expected-stolen);}
  check(std::abs(f.hp()-expected)<1.1&&std::abs(double(f.w.resources(1)->health)-(100+stolen))<1.1);
  const auto statuses=*f.w.status_snapshot(f.target,1000);check(std::none_of(statuses.begin(),statuses.end(),[](const auto& e){return e.skill==307;}));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(expected<=0));
 }
 check(cases==168);
 for(unsigned skill:{133u,219u,155u,158u,1078u,102u,1067u})for(bool npc:{false,true}){
  Fixture f(npc,10);if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*skills::effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(12);f.box->drain();check(f.w.shadow_damage(f.source,f.target,*skills::effects::shadow_damage(skill,12),20,100));check(f.hp()==0);
  const auto packets=f.box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162;}));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 // The first raw hit is lethal here, but conversion can save it and
 // therefore permit the original above-half follow-up to execute.
 for(unsigned skill:{102u,1067u})for(bool npc:{false,true})for(unsigned rr:{0u,12u}){
  Fixture f(npc,51,100);f.reversal(rr);const auto plan=*skills::effects::shadow_damage(skill,20);
  check(plan.damage==60&&plan.steal_above_half==60);check(f.w.shadow_damage(f.source,f.target,plan,20,1000));
  check(f.hp()==(rr?40:0)&&f.w.resources(1)->health==(rr?160:122));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(rr==0));
 }
 // An absorbed first hit leaves Reversal for the positive life-steal component.
 // It now consumes the enchantment without manufacturing a source payout.
 for(unsigned skill:{102u,1067u}){Fixture f(false,1000);check(f.w.status_mutation(f.target,1375,8,12,100,13100,true));f.reversal(12);f.box->drain();
  check(f.w.shadow_damage(f.source,f.target,*skills::effects::shadow_damage(skill,0),20,1000));check(f.hp()==1000&&f.w.resources(1)->health==100&&!f.box->drain().empty());
  const auto statuses=*f.w.status_snapshot(f.target,1000);check(std::none_of(statuses.begin(),statuses.end(),[](const auto& e){return e.skill==307;}));
 }
 std::cout<<"PASS168 shadow conversion cases across7 plans,14 healing deaths,2 zero-first-hit life-steal conversions;paid307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
