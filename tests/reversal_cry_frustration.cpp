#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Cry Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,selected;std::vector<effects::Scope> targets;
 std::vector<combat::npcs::control::Token> actions;
 Fixture(bool npc_selected,unsigned hp=500,unsigned wounded=0){
  const unsigned primary=npc_selected?100:2;std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?60:100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(id==wounded?10:id==primary?hp:500),1000};
   d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";d.spawn_energy=combat::npcs::Energy{20,100};d.energy_pips=0;n.combat_definition=d;population.push_back(n);
  }w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==wounded?10u:id==primary?hp:id==1?1000u:500u,1000,20,100},100);}
  source=*w.effect_scope(1);selected=*w.effect_scope(primary);for(unsigned id:{2u,3u,100u,101u})targets.push_back(*w.effect_scope(id));
 }
 double hp(effects::Scope t){return t.actor>=100?w.npc_combat_snapshot(t)->health->current:w.resources(t.actor)->health;}
 void reversal(unsigned rank){for(auto t:targets)check(t.actor>=100?w.stage_npc_reversal(t,rank,100):w.status_mutation(t,307,900,rank,100,8100,true));}
 void start(unsigned stance){for(auto t:targets){
  if(stance)check(t.actor>=100?w.npc_interrupt_stance(t,stance,12,10100,100):w.status_mutation(t,stance,950,12,100,10100,true));
  if(t.actor>=100){const auto a=w.begin_npc_action(t,t.actor==100?combat::npcs::control::Action::spell:combat::npcs::control::Action::attack,100,2100);check(a.has_value());actions.push_back(*a);}
  else check(w.cast_activity(t,82,100,2100));
 }}
};
double converted(unsigned hp,unsigned damage,int rr){const auto heal=rr<0?0u:std::min(damage,effects::reversal_fortune(rr)->maximum_healing);return std::max(0.,std::min(1000.,double(hp+heal))-(damage-heal));}
int main()try{
 unsigned cases=0;
 for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(unsigned stance:{0u,16u,17u}){
  ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.start(stance);const auto damage=effects::cry_frustration(rank)->damage;
  check(f.w.cry_frustration(f.source,f.selected,*effects::cry_frustration(rank),20,200));
  unsigned npc_deaths=0;
  for(auto t:f.targets){const auto wanted=converted(t==f.selected?hp:500,damage,rr);check(std::abs(f.hp(t)-wanted)<1.01);
   const auto entries=*f.w.status_snapshot(t,200);check(std::none_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;}));
   if(t.actor>=100){npc_deaths+=wanted==0;const auto token=std::find_if(f.actions.begin(),f.actions.end(),[&](const auto& a){return a.actor==t;});check(token!=f.actions.end());check(f.w.complete_npc_action(*token,2100)==bool(stance&&wanted>0));}
   else check(bool(f.w.casting_skill(t,200))==bool(stance&&wanted>0));
   if(wanted>0){const auto energy=t.actor>=100?f.w.npc_energy_snapshot(t)->current:f.w.resources(t.actor)->energy;check(energy==(stance==17?15u:20u));}
  }check(f.w.take_npc_deaths(f.source).size()==npc_deaths);
 }
 for(bool npc:{false,true})for(unsigned wounded:{2u,3u,100u,101u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,500,wounded);const auto t=*f.w.effect_scope(wounded);
  if(wounded>=100){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,t,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,t,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(rr);f.start(0);check(f.w.cry_frustration(f.source,f.selected,*effects::cry_frustration(12),20,200));
  for(auto target:f.targets)check(std::abs(f.hp(target)-(target==t?0:converted(500,63,rr)))<1.01);
  check(f.w.take_npc_deaths(f.source).size()==unsigned(wounded>=100));
 }
 // Primary player death explodes before the NPC pass; its25 damage consumes
 // NPC100's Reversal. Cry then applies63 to CURRENT525, yielding462,not563.
 {++cases;Fixture f(false,10);const auto npc=*f.w.effect_scope(100);
  check(f.w.stage_npc_reversal(npc,12,100));check(f.w.degeneration_hex(f.source,f.selected,*effects::degeneration_hex(2058,0),100));f.start(0);
  check(f.w.cry_frustration(f.source,f.selected,*effects::cry_frustration(12),20,200));check(f.hp(f.selected)==0&&f.hp(npc)==462);
  check(f.hp(*f.w.effect_scope(101))==412&&f.hp(*f.w.effect_scope(3))==412);
 }
 check(cases==241);std::cout<<"PASS "<<cases<<" Cry Reversal mixed-recipient/protection/healing-death/intervening-explosion cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
