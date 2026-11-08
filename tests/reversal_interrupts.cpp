#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Interrupt Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;effects::Scope source,target;std::optional<combat::npcs::control::Token> token;
 Fixture(bool is_npc,unsigned hp):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{20,100};d.energy_pips=0;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?1000u:hp,1000,id==1?100u:20u,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
  if(!npc)check(w.energy_capacity(2,5,0,100,{80,-2}));
 }
 double hp()const{return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
 unsigned energy()const{return npc?w.npc_energy_snapshot(target)->current:w.resources(2)->energy;}
 void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
 void start(unsigned stance){
  if(stance)check(npc?w.npc_interrupt_stance(target,stance,12,10100,100):w.status_mutation(target,stance,950,12,100,10100,true));
  if(npc){token=w.begin_npc_action(target,combat::npcs::control::Action::spell,100,2100,skills::Slot{82,0});check(token.has_value());}
  else check(w.cast_activity(target,82,100,2100));
 }
 bool has_reversal(){const auto entries=*w.status_snapshot(target,200);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 void interrupted(bool protected_action,bool alive){
  if(npc)check(w.complete_npc_action(*token,2100)==(protected_action&&alive));
  else check(bool(w.casting_skill(target,200))==(protected_action&&alive));
 }
};
int main()try{
 unsigned cases=0;
 for(bool npc:{false,true})for(unsigned skill:{23u,860u})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u})for(unsigned stance:{0u,16u,17u}){
  ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.start(stance);
  const auto plan=*effects::interrupt_skill(skill,rank);const auto heal=rr<0?0u:std::min(plan.damage,effects::reversal_fortune(rr)->maximum_healing);
  const auto wanted=std::max(0.,std::min(1000.,double(hp+heal))-double(plan.damage-heal));
  check(f.w.interrupt_skill(f.source,f.target,plan,20,200));check(std::abs(f.hp()-wanted)<1.01&&!f.has_reversal());
  if(wanted>0)check(f.energy()==(stance==17?15u:20u));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
  f.interrupted(stance!=0,wanted>0);
 }
 for(bool npc:{false,true})for(unsigned skill:{23u,860u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,10);
  if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(rr);f.start(0);check(f.w.interrupt_skill(f.source,f.target,*effects::interrupt_skill(skill,0),20,200));
  check(f.hp()==0&&!f.has_reversal());if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 // No eligible cast means no damage and no consumption of damage conversion.
 for(bool npc:{false,true})for(unsigned skill:{23u,860u}){
  ++cases;Fixture f(npc,500);f.reversal(12);check(f.w.interrupt_skill(f.source,f.target,*effects::interrupt_skill(skill,12),20,200));check(f.hp()==500&&f.has_reversal());
 }
 check(cases==448);std::cout<<"PASS "<<cases<<" Power Spike/Signet of Disruption Reversal cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
