#include "gw_mind_wrack.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <source_location>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Wrack line "+std::to_string(at.line()));}
int main()try{
 const Scope caster{7,1,11},target{7,2,12};Registry original(target);
 for(bool pvp:{false,true})for(unsigned rank:{0u,12u,15u,20u}){
  const auto plan=*mind_wrack(rank,pvp);check(plan.skill==(pvp?2734u:49u));check(plan.damage_per_energy==(pvp?0u:interpolate(5,25,rank))&&plan.zero_damage==interpolate(15,100,rank));
  auto app=prepare_mind_wrack(original,caster,target,1,plan,100);check(app&&original.size()==0&&app->registry.size()==1&&app->removed.empty());
  check(current_mind_wrack(*app,app->registry,caster,target,100)&&!current_mind_wrack(*app,app->registry,{7,1,99},target,100)&&!current_mind_wrack(*app,app->registry,caster,{7,2,99},100));
  check(!owned_mind_wrack(app->registry,app->owner,99)&&!owned_mind_wrack(app->registry,app->owner,100+plan.duration));
  for(unsigned before:{0u,1u,2u,100u}){
   MindWrackTransition loss{target,9,200,{before,100,0},{before?before-1:0,100,0}};
   MindWrackUse use{caster,target,42,9,200,true};auto reaction=mind_wrack_target_reaction(app->registry,app->owner,use,loss);check(bool(reaction));
   const bool zero=before==1;check(reaction->ends==zero&&reaction->zero_damage==(zero?plan.zero_damage:0)&&reaction->per_loss_damage==(before?plan.damage_per_energy:0));
   check(reaction->registry.size()==(zero?0u:1u)&&app->registry.size()==1);
  }
  // An arbitrary loss can trigger the terminal burst, but never the PvE
  // per-loss component belonging only to Mind Wrack's own one-Energy loss.
  MindWrackTransition drain{target,10,200,{3,100,0},{0,100,0}};auto burst=mind_wrack_energy_reaction(app->registry,app->owner,drain);check(burst&&burst->ends&&burst->zero_damage==plan.zero_damage&&!burst->per_loss_damage);
  check(!mind_wrack_target_reaction(app->registry,app->owner,{caster,target,42,10,200,true},drain));
  for(auto before: {MindWrackEnergy{0,100,0},MindWrackEnergy{0,100,5}}){MindWrackTransition none{target,11,200,before,before};auto r=mind_wrack_energy_reaction(app->registry,app->owner,none);check(r&&!r->ends&&!r->zero_damage);}
  MindWrackTransition below{target,12,200,{1,100,0},{0,90,2}};check(!mind_wrack_zero_transition(below));
  MindWrackTransition repaid{target,13,200,{0,100,1},{0,100,0}};check(!mind_wrack_zero_transition(repaid));
  auto end=end_mind_wrack(app->registry,app->owner,100+plan.duration);check(end&&end->registry.size()==0&&app->registry.size()==1);
  check(!end_mind_wrack(app->registry,app->owner,99));
  auto alias=prepare_mind_wrack(app->registry,{7,3,13},target,2,*mind_wrack(0,!pvp),300);check(alias&&alias->registry.size()==1&&alias->removed.size()==1&&alias->removed[0].instance==1&&!owned_mind_wrack(alias->registry,app->owner,300));
 }
 check(mind_wrack(12)->duration==33000&&mind_wrack(12)->zero_damage==83&&mind_wrack(12)->damage_per_energy==21&&!mind_wrack(21));
 auto app=*prepare_mind_wrack(original,caster,target,1,*mind_wrack(12),100);
 for(unsigned skill:{42u,57u,79u,21u})check(mind_wrack_qualifying_target(app.owner,{caster,target,skill,1,200,true}));
 for(unsigned skill:{0u,28u,49u,2734u,52u,194u,3433u})check(!mind_wrack_qualifying_target(app.owner,{caster,target,skill,1,200,true}));
 check(!mind_wrack_qualifying_target(app.owner,{{7,1,99},target,42,1,200,true})&&!mind_wrack_qualifying_target(app.owner,{caster,caster,42,1,200,true})&&!mind_wrack_qualifying_target(app.owner,{caster,target,42,1,200,false}));
 auto forged=*mind_wrack(12);++forged.zero_damage;check(!prepare_mind_wrack(original,caster,target,1,forged,100));
 check(!prepare_mind_wrack(original,caster,target,0,*mind_wrack(12),100)&&!prepare_mind_wrack(original,caster,caster,1,*mind_wrack(12),100)&&!prepare_mind_wrack(original,caster,target,1,*mind_wrack(12),UINT64_MAX-32999));
 MindWrackEvents ledger(target);MindWrackTransition e{target,1,200,{1,100,0},{0,100,0}};auto staged=ledger;check(staged.accept(e)&&!staged.accept(e));check(ledger.accept(e));
 MindWrackTransition fraction{target,2,200,{1,100,0,150000},{0,100,0,150000}};
 check(!mind_wrack_zero_transition(fraction));fraction.before={0,100,0,150000};fraction.after={0,100,0,0};check(mind_wrack_zero_transition(fraction));
 fraction.before={0,100,1,150000};check(!mind_wrack_zero_transition(fraction));
 fraction.before={1,100,0,300000};check(!valid_mind_wrack_transition(fraction));
 ++e.event;--e.at;check(!ledger.accept(e));++e.at;check(ledger.accept(e));++e.event;e.target.generation++;check(!ledger.accept(e));
 e.target=target;e.before={1,100,1};check(!ledger.accept(e));e.before={101,100,0};check(!ledger.accept(e));e.before={1,0,0};check(!ledger.accept(e));
 std::cout<<"PASS Mind Wrack PvE/PvP plans, own-loss vs terminal damage, true-zero vs debt, alias renewal/end ownership, confirmed Mesmer target qualification and replay/time guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
