#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Gated Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;effects::Scope source,target,bonded;
 Fixture(bool is_npc,unsigned hp=500,unsigned energy=80):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;
  d.cold_armor=60;d.cold_armor_evidence="synthetic";d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:700,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2?hp:1000u,1000,id==1?energy:100u,100},100);check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),{},60));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);bonded=*w.effect_scope(3);
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
 void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
 bool has(unsigned skill){const auto entries=*w.status_snapshot(target,101);return std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.skill==skill;});}
 void gate(unsigned skill){if(skill!=2193)check(w.movement_hex(source,target,*effects::movement_hex(210,12),100));}
 bool cast(unsigned skill,unsigned rank){
  if(skill==1382)return w.freezing_gust(source,target,*effects::movement_hex(skill,rank),20,101);
  if(skill==2103)return w.necrosis(source,target,*effects::necrosis(rank),20,101);
  return w.energy_blast(source,target,*effects::energy_blast(rank),20,101);
 }
};
unsigned rank_for(unsigned skill,unsigned index){return skill==2103?index==0?0:index==1?5:10:index==0?0:index==1?12:20;}
double amount(unsigned skill,unsigned rank,unsigned energy=80){if(skill==1382)return effects::freezing_gust(rank)->damage;if(skill==2103)return effects::necrosis(rank)->damage;const auto p=*effects::energy_blast(rank);return std::min(p.maximum_damage,energy*p.damage_per_energy);}
double expected(unsigned hp,double damage,int rr){const double heal=rr<0?0:std::min(damage,double(effects::reversal_fortune(rr)->maximum_healing));return std::max(0.,std::min(1000.,hp+heal)-(damage-heal));}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{1382u,2103u,2193u})for(bool npc:{false,true})for(unsigned index:{0u,1u,2u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(npc,hp);const auto rank=rank_for(skill,index);f.gate(skill);if(rr>=0)f.reversal(rr);check(f.cast(skill,rank));
  const auto wanted=expected(hp,amount(skill,rank),rr);check(std::abs(f.hp()-wanted)<1.01&&!f.has(307));if(skill==1382)check(!f.has(1382));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
 }
 // No damaging branch: no water hex, no affliction, or zero caster Energy.
 for(unsigned skill:{1382u,2103u,2193u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,500,skill==2193?0:80);f.reversal(rr);check(f.cast(skill,rank_for(skill,1)));check(f.hp()==500&&f.has(307));if(skill==1382)check(f.has(1382));
 }
 for(unsigned skill:{1382u,2103u,2193u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,10);f.gate(skill);
  if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(rr);check(f.cast(skill,rank_for(skill,1)));check(f.hp()==0&&!f.has(307));if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned energy:{0u,1u,100u}){
  ++cases;Fixture f(npc,500,energy);if(rr>=0)f.reversal(rr);check(f.cast(2193,12));check(std::abs(f.hp()-expected(500,amount(2193,12,energy),rr))<1.01);check(f.has(307)==(energy==0&&rr>=0));
 }
 // Necrosis also accepts a condition without a hex.
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc);
  if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::blind,100,10000));
  f.reversal(rr);check(f.cast(2103,5));check(std::abs(f.hp()-expected(500,75,rr))<1.01&&!f.has(307));
 }
 // NPC Bonds propagates only actual remaining loss, including capped lethal
 // damage, rather than the original hit or conversion's healing amount.
 for(unsigned skill:{1382u,2103u,2193u})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u}){
  ++cases;Fixture f(true,hp);f.gate(skill);f.reversal(rr);check(f.w.stage_npc_bonds_of_torment(f.target,f.bonded,*effects::bonds_of_torment_npc(),100));
  const auto damage=amount(skill,rank_for(skill,1)),heal=std::min(damage,double(effects::reversal_fortune(rr)->maximum_healing));const auto loss=std::min(hp+heal,damage-heal);
  check(f.cast(skill,rank_for(skill,1)));check(std::abs(double(f.w.resources(3)->health)-(1000-loss))<1.01);
 }
 check(cases==300);std::cout<<"PASS "<<cases<<" Freezing Gust/Necrosis/Energy Blast Reversal gates,Health/rank,zero Energy,healing deaths and actual Bonds;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
