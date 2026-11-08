#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mind Freeze Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;effects::Scope source,target;
 Fixture(bool is_npc,unsigned hp=500,unsigned energy=20):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;
  d.cold_armor=60;d.cold_armor_evidence="synthetic";d.spawn_energy=combat::npcs::Energy{energy,100};d.energy_pips=0;
  d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?1000u:hp,1000,id==1?80u:energy,100},100);check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),{},60));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
 void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
 bool has(unsigned skill){const auto entries=*w.status_snapshot(target,101);return std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.skill==skill;});}
 bool cast(unsigned skill,unsigned rank=12){return w.mind_freeze(source,target,*effects::mind_freeze(skill,rank),20,101);}
};
double expected(unsigned hp,double damage,int rank,bool extra){const double heal=rank<0?0:std::min(damage,double(effects::reversal_fortune(rank)->maximum_healing));double current=std::max(0.,std::min(1000.,hp+heal)-(damage-heal));if(extra&&current>0)current=std::max(0.,current-damage);return current;}
int main()try{
 unsigned cases=0;
 for(bool npc:{false,true})for(unsigned skill:{209u,2803u})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(unsigned energy:{20u,80u,90u}){
  ++cases;Fixture f(npc,hp,energy);if(rr>=0)f.reversal(rr);const auto plan=*effects::mind_freeze(skill,rank);
  check(f.cast(skill,rank));const auto wanted=expected(hp,plan.damage,rr,80>energy);
  check(std::abs(f.hp()-wanted)<1.01&&!f.has(307));check(f.has(skill)==(wanted>0&&80>energy));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
 }
 for(bool npc:{false,true})for(unsigned skill:{209u,2803u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,10);
  if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(rr);check(f.cast(skill));check(f.hp()==0&&!f.has(307)&&!f.has(skill));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(unsigned skill:{209u,2803u}){
  ++cases;Fixture f(false);f.reversal(12);check(f.w.stage_watch_yourself_recipient(f.target,f.target,*effects::watch_yourself(0,true),100));
  check(f.cast(skill,0));check(std::abs(f.hp()-500)<1.01&&f.has(skill));
 }
 // Hex Breaker blocks the slow, never the two independent damage events.
 for(unsigned skill:{209u,2803u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(false);f.reversal(rr);check(f.w.hex_breaker(f.target,*effects::hex_breaker(12),20,100));
  check(f.cast(skill));const auto damage=effects::mind_freeze(skill,12)->damage;
  check(std::abs(f.hp()-expected(500,damage,rr,true))<1.01&&!f.has(skill)&&!f.has(307));
 }
 check(cases==452);std::cout<<"PASS "<<cases<<" Mind Freeze PvE/PvP Reversal double-hit/survival/slow/armor/Hex Breaker cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
