#include "gw_insidious_parasite.h"
#include "gw_attack_outcome_batch.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;using namespace combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("Insidious Parasite policy regression");}
int main()try{
 check(!insidious_parasite(21));
 for(unsigned rank=0;rank<=20;++rank){const auto p=insidious_parasite(rank);check(p&&p->amount==15+2*rank&&p->duration==std::lround(5.+10.*rank/15.)*1000&&p->before_attack_damage&&p->ends_on_caster_death);}
 const Scope attacker{1,2,3},victim{1,4,5};Entry hex{9,42,123,12,Category::hex,100,13100};
 const ResolvedAttack hit{attacker,victim,1,100,600,AttackOutcome::hit};check(insidious_parasite_hit(attacker,hex,hit)->amount==39);
 for(auto outcome:{AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::target_retired,AttackOutcome::cancelled}){auto e=hit;e.outcome=outcome;check(!insidious_parasite_hit(attacker,hex,e));}
 for(unsigned invalid=0;invalid<10;++invalid){auto e=hit;auto h=hex;
  if(invalid==0)e.attacker.generation++;if(invalid==1)e.target.instance++;if(invalid==2)e.target.generation=0;if(invalid==3)e.event=0;
  if(invalid==4)e.at=99;if(invalid==5)e.at=13100;if(invalid==6)h.cause=0;if(invalid==7)h.skill=101;if(invalid==8)h.expires.reset();if(invalid==9)h.kind=Category::enchantment;
  check(!insidious_parasite_hit(attacker,h,e));
 }
 auto expired=hit;expired.at=13099;check(insidious_parasite_hit(attacker,hex,expired).has_value());
 // Wiki explicitly specifies every hit. Each consumed recipient qualifies;
 // this does not borrow Price of Failure's unresolved failure cardinality.
 AttackOutcomes current(attacker);const std::array releases{AttackRelease{victim,1},AttackRelease{{1,6,7},2},AttackRelease{{1,8,9},3}};
 auto staged=stage_attack_outcome_batch(current,attacker,releases,100);check(bool(staged));unsigned total=0;
 for(const auto& r:releases){const auto e=staged->finish(attacker,r.target,r.event,AttackOutcome::hit,600);check(bool(e));total+=insidious_parasite_hit(attacker,hex,*e)->amount;check(!staged->finish(attacker,r.target,r.event,AttackOutcome::hit,600));}
 check(total==117&&staged->size()==0);
 std::cout<<"PASS Insidious Parasite rank/duration,terminal-hit qualification,expiry,scope and consumed multi-recipient outcomes;qualification only;see world/casting suites\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
