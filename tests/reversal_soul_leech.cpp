#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal Soul Leech line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target;
 Fixture(bool n,unsigned hp,unsigned source_hp):npc(n){
  if(npc){world::Instance::Npc a;a.id=a.definition.id=100;a.x=60;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,0};d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=d.fleshy=true;
   d.healing_evidence=d.fleshy_evidence="synthetic Soul Leech recipient";a.combat_definition=d;w.configure_npcs({a});}
  w.join(1,"Source",0,0,box,0,{}, {},100,1);w.set_vitals(1,{source_hp,1000,100,100},100);
  if(!npc){w.join(2,"Afflicted",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 void reversal(int rank){if(rank>=0)check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,5,rank,100,8100,true));}
 bool has_reversal(unsigned at=100){const auto entries=*w.status_snapshot(target,at);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
};
int main()try{
 unsigned cases=0;
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})
 for(unsigned hp:{10u,300u,1000u})for(unsigned source_hp:{100u,990u,1000u})for(bool absorption:{false,true}){
  Fixture f(npc,hp,source_hp);const auto plan=*effects::soul_leech(rank);
  check(f.w.stage_soul_leech(f.source,f.target,plan,100));
  if(absorption)check(npc?f.w.stage_npc_shield_absorption(f.target,12,100):f.w.status_mutation(f.target,1399,80,12,100,6100,true));
  f.reversal(rr);f.box->drain();
  const double converted=rr<0?0:std::min(plan.amount,effects::reversal_fortune(rr)->maximum_healing);
  const double healed=std::min(1000.,hp+converted);
  const double stolen=std::min(double(plan.amount)-converted,healed+1);
  const double expected=std::max(0.,healed-stolen),source=std::min(1000.,source_hp+stolen);
  check(f.w.before_spell_effect(f.target,281,1,100)==(expected>0));
  if(f.hp()!=expected||f.w.resources(1)->health!=source)std::cerr<<"npc="<<npc<<" rank="<<rank<<" rr="<<rr<<" hp="<<hp<<" actual="<<f.hp()<<'/'<<f.w.resources(1)->health<<" expected="<<expected<<'/'<<source<<'\n';
  check(f.hp()==expected&&f.w.resources(1)->health==source&&!f.has_reversal());
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(expected<=0));
  f.box->drain();check(!f.w.before_spell_effect(f.target,281,1,100)&&f.box->drain().empty());++cases;
 }
 // Deep Wound debt is settled by conversion healing before a caster payout.
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  Fixture f(npc,10,100);check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  check(f.w.stage_soul_leech(f.source,f.target,*effects::soul_leech(12),100));f.reversal(rr);f.box->drain();
  check(!f.w.before_spell_effect(f.target,281,1,100));check(f.hp()==0&&f.w.resources(1)->health==100&&!f.has_reversal());
  if(npc)check(f.w.take_npc_deaths(f.source).size()==1);++cases;
 }
 // Shielding Hands can reduce a player transfer to zero. Damage-only
 // absorption above cannot, and must not consume its own counter either.
 for(unsigned rr:{0u,12u,20u}){
  Fixture f(false,300,100);check(f.w.stage_soul_leech(f.source,f.target,*effects::soul_leech(0),100));
  check(f.w.status_mutation(f.target,299,80,20,100,10100,true));f.reversal(rr);f.box->drain();
  check(f.w.before_spell_effect(f.target,281,1,100));check(f.hp()==300&&f.w.resources(1)->health==100&&f.has_reversal());++cases;
 }
 for(bool npc:{false,true}){
  Fixture f(npc,300,100);check(f.w.stage_soul_leech(f.source,f.target,*effects::soul_leech(12),100));f.reversal(12);
  check(f.w.before_spell_effect(f.target,281,1,8100));check(f.hp()==233&&f.w.resources(1)->health==167&&!f.has_reversal(8100));++cases;
 }
 std::cout<<cases<<" Soul Leech reserved spell conversion, cap, immunity-to-damage-only-reduction, zero input, wound death, expiry and duplicate cases passed; paid307 unbound\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
