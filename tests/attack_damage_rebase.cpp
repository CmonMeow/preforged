#include "gw_attack_damage_rebase.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace combat;using namespace combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("Pre-damage Health rebase regression");}
int main()try{
 const AttackScope attacker{1,1,2},target{1,3,4};const QualifiedHit hit{attacker,target,1,100,600};
 Actor old{3,4,Health{100,500},{},0,true};auto planned=*apply_resolved(old,120,true);planned.skill=330;planned.kind=Kind::lightning;
 Actor healed{3,4,Health{139,500},{},0x800,true};auto result=rebase_attack_damage(hit,target,healed,planned);
 check(result&&result->before.current==139&&result->after.current==19&&!result->killed&&result->effects==0x800&&result->skill==330&&result->kind==Kind::lightning&&healed.health->current==19);
 old={3,4,Health{100,500},{},0,true};planned=*apply_resolved(old,20,true);planned.skill=330;
 Actor damaged{3,4,Health{50,500},{},0,true};result=rebase_attack_damage(hit,target,damaged,planned);check(result&&result->after.current==30); // Never restore old80HP.
 for(unsigned invalid=0;invalid<7;++invalid){auto q=hit;auto p=planned;auto scope=target;Actor actor{3,4,Health{139,500},{},0,true};
  if(invalid==0)++scope.generation;if(invalid==1)++q.target.instance;if(invalid==2)q.event=0;if(invalid==3)p.after.current=99;if(invalid==4)++p.generation;
  if(invalid==5)p.kind=static_cast<Kind>(255);if(invalid==6)p.effects|=0x10;
  check(!rebase_attack_damage(q,scope,actor,p)&&actor.health->current==139&&actor.effects==0);
 }
 old={3,4,Health{100,500},{},0,true};planned=*apply_resolved(old,0,true);Actor zero{3,4,Health{139,500},{},0,true};result=rebase_attack_damage(hit,target,zero,planned);check(result&&result->applied==0&&result->after.current==139);
 Actor dead{3,4,Health{0,500},{},0x10,true};check(!rebase_attack_damage(hit,target,dead,planned)&&dead.health->current==0);
 std::cout<<"PASS post-reaction Health rebasing,preserved damage/skill/kind,new death state,scope validation,zero damage and no resurrection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
