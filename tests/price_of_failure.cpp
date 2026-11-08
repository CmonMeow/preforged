#include "gw_price_of_failure.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool ok){if(!ok)throw std::runtime_error("Price of Failure policy regression");}
int main(){using namespace skills::effects;using namespace combat::weapon;
 const Scope victim{1,1,10},target{1,2,20};
 for(unsigned rank:{0u,12u,20u}){
  const auto plan=price_of_failure(rank);check(plan&&plan->damage==1+3*rank&&plan->miss_percent==25&&plan->duration==30000);
  AttackOutcomes outcomes(victim);const Entry hex{9,3,103,rank,Category::hex,100,30100};unsigned triggers=0,id=0;
  for(auto reason:{AttackOutcome::hit,AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::target_retired,AttackOutcome::cancelled}){
   const auto at=200+100*id;check(outcomes.release(victim,target,++id,at));const auto event=outcomes.finish(victim,target,id,reason,at+50);check(event.has_value());
   const auto reaction=price_of_failure_reaction(victim,hex,*event);triggers+=bool(reaction);
   if(reaction)check(reaction->damage==plan->damage);
   check(!outcomes.finish(victim,target,id,reason,300));
  }
  check(triggers==4);
  ResolvedAttack event{victim,target,100,200,300,AttackOutcome::miss};
  auto bad=hex;bad.applied=301;check(!price_of_failure_reaction(victim,bad,event));
  bad=hex;bad.expires=300;check(!price_of_failure_reaction(victim,bad,event));
  bad=hex;bad.skill=101;check(!price_of_failure_reaction(victim,bad,event));
  bad=hex;bad.cause=0;check(!price_of_failure_reaction(victim,bad,event));
  auto stale=victim;++stale.generation;check(!price_of_failure_reaction(stale,hex,event));
  event.at=199;check(!price_of_failure_reaction(victim,hex,event));
 }
 check(!price_of_failure(21));
 std::cout<<"PASS Price of Failure rank and scoped terminal-outcome policy\n";
}
