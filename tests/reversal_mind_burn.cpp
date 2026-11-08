#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mind Burn Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,player,npc,selected;
 Fixture(bool npc_selected,unsigned hp=500,unsigned energy=20,unsigned wounded=0){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(wounded==100?10:hp),1000};d.damageable=d.conditions_allowed=true;
  d.fire_armor=60;d.fire_armor_evidence="synthetic";d.spawn_energy=combat::npcs::Energy{energy,100};d.energy_pips=0;
  d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?1000u:wounded==2?10u:hp,1000,id==1?80u:energy,100},100);check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),60,{}));}
  source=*w.effect_scope(1);player=*w.effect_scope(2);npc=*w.effect_scope(100);selected=npc_selected?npc:player;
 }
 double hp(effects::Scope t){return t==npc?w.npc_combat_snapshot(t)->health->current:w.resources(t.actor)->health;}
 void reversal(unsigned rank){check(w.stage_npc_reversal(npc,rank,100)&&w.status_mutation(player,307,900,rank,100,8100,true));}
 bool rr(effects::Scope t){const auto s=*w.status_snapshot(t,101);return std::any_of(s.begin(),s.end(),[](const auto& e){return e.skill==307;});}
};
double expected(unsigned hp,double damage,int rank,bool extra){const double heal=rank<0?0:std::min(damage,double(effects::reversal_fortune(rank)->maximum_healing));double current=std::max(0.,std::min(1000.,hp+heal)-(damage-heal));if(extra&&current>0)current=std::max(0.,current-damage);return current;}
int main()try{
 unsigned cases=0;
 for(bool npc_selected:{false,true})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(unsigned energy:{20u,80u,90u}){
  ++cases;Fixture f(npc_selected,hp,energy);if(rr>=0)f.reversal(rr);const auto plan=*effects::mind_burn(185,rank);
  check(f.w.mind_burn(f.source,f.selected,plan,20,101));const auto wanted=expected(hp,plan.damage,rr,80>energy);
  for(auto t:{f.player,f.npc}){check(std::abs(f.hp(t)-wanted)<1.01&&!f.rr(t));check(f.w.has_condition(t,combat::conditions::Kind::burning,101)==(wanted>0&&80>energy));}
  check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
 }
 for(bool selected:{false,true})for(unsigned wound:{2u,100u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(selected,500,20,wound);const auto target=wound==2?f.player:f.npc;
  if(wound==100){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,target,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,target,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(rr);check(f.w.mind_burn(f.source,f.selected,*effects::mind_burn(185,12),20,101));check(f.hp(target)==0&&!f.rr(target)&&!f.w.has_condition(target,combat::conditions::Kind::burning,101));
  check(std::abs(f.hp(wound==2?f.npc:f.player)-expected(500,51,rr,true))<1.01);check(f.w.take_npc_deaths(f.source).size()==unsigned(wound==100));
 }
 // The first fully converted hit must not spend a Watch Yourself hit-charge:
 // the second hit still sees the same armor rather than the naked60 rating.
 {++cases;Fixture f(false);f.reversal(12);check(f.w.stage_watch_yourself_recipient(f.player,f.player,*effects::watch_yourself(0,true),100));
  check(f.w.mind_burn(f.source,f.player,*effects::mind_burn(185,0),20,101));check(std::abs(f.hp(f.player)-500)<1.01);
 }
 check(cases==229);std::cout<<"PASS "<<cases<<" Mind Burn Reversal per-recipient Energy/double-hit/Burning/healing-death/armor-sequence cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
