#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Holy Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;effects::Scope source,target,bonded;unsigned traits;
 Fixture(bool is_npc,unsigned hp=500,unsigned mode=0):npc(is_npc),traits(mode){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";
  d.holy_traits.light_sensitive=bool(mode&1);d.light_sensitive_evidence="synthetic";d.holy_traits.summoned=bool(mode&2);d.summoned_evidence="synthetic";
  d.knockdown_allowed=true;d.knockdown_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:700,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2?hp:1000u,1000,id==1?50u:100u,100},100);check(w.energy_capacity(id,3,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),{},60));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);bonded=*w.effect_scope(3);check(w.player_holy_equipment(*w.effect_scope(2),0));
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
 void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
 bool rr(){const auto entries=*w.status_snapshot(target,101);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 void enchant(){check(w.status_mutation(source,169,700,12,100,10100,true)&&w.status_mutation(source,180,701,12,100,10100,true));}
 void attack(){if(npc)check(w.begin_npc_action(target,combat::npcs::control::Action::attack,100,2100).has_value());else check(w.attack_activity(target,2100,100));}
 bool cast(unsigned skill,unsigned rank=12){return w.holy_damage(source,target,*effects::holy_damage(skill,rank),20,101);}
};
double expected(double hp,double damage,int rank){const auto heal=rank<0?0.:std::min(damage,double(effects::reversal_fortune(rank)->maximum_healing));return std::max(0.,std::min(1000.,hp+heal)-(damage-heal));}
int main()try{
 unsigned cases=0;
 for(bool npc:{false,true})for(unsigned mode:{0u,1u,2u,3u}){
  if(!npc&&mode)continue;
  for(unsigned skill:{252u,296u,312u,2006u,1689u})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
   ++cases;Fixture f(npc,hp,mode);if(skill==1689)f.enchant();if(rr>=0)f.reversal(rr);
   const auto plan=*effects::holy_damage(skill,rank,skill==1689?2:0);const auto amount=plan.damage*((npc&&(mode&1))?2u:1u)*((skill==252&&npc&&(mode&2))?2u:1u);
   check(f.cast(skill,rank));const auto wanted=expected(hp,amount,rr);check(std::abs(f.hp()-wanted)<1.01&&!f.rr());if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));check(f.w.resources(1)->energy==50);
  }
 }
 // Holy Strike has two distinct strikes only against an already-down target.
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(npc,hp);f.reversal(rr);check(f.w.gale(f.source,f.target,12,{},100));const auto damage=effects::holy_damage(312,rank)->damage;const auto first=expected(hp,damage,rr);const auto wanted=std::max(0.,first-damage);
  check(f.cast(312,rank));check(std::abs(f.hp()-wanted)<1.01&&!f.rr());if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
 }
 for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.attack();check(f.cast(296));const auto wanted=expected(hp,50,rr);check(std::abs(f.hp()-wanted)<1.01);if(wanted>0)check(f.w.knocked_down(f.target,101));
 }
 // Castigation's Energy reward uses the pre-hit attacking snapshot, independent
 // of converted damage and whether that hit kills the target.
 for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.attack();const auto plan=*effects::holy_damage(2006,12);check(f.cast(2006));check(f.w.resources(1)->energy==50+plan.energy_if_attacking);
 }
 for(unsigned skill:{252u,296u,312u,2006u,1689u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,10);if(skill==1689)f.enchant();
  if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(rr);check(f.cast(skill));check(f.hp()==0&&!f.rr());if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc);f.reversal(rr);check(f.cast(1689));check(f.hp()==500&&f.rr());
 }
 for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(false);check(f.w.player_holy_equipment(f.target,6));f.reversal(rr);check(f.cast(1689));check(f.hp()==506&&!f.rr());
 }
 for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(true,hp);f.reversal(rr);check(f.w.gale(f.source,f.target,12,{},100));check(f.w.stage_npc_bonds_of_torment(f.target,f.bonded,*effects::bonds_of_torment_npc(),100));
  const auto heal=std::min(46u,effects::reversal_fortune(rr)->maximum_healing);const auto healed=std::min(1000.,double(hp+heal)),loss=std::min(healed,double(46-heal)),first=healed-loss;const auto second=std::min(first,46.);
  check(f.cast(312));check(std::abs(double(f.w.resources(3)->health)-(1000-loss-second))<1.01);
 }
 check(cases==1050);std::cout<<"PASS "<<cases<<" shared holy-family Reversal traits/sequential strikes/control/conditional Energy/zero/vulnerability/Bonds cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
