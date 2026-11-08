#include "gw_shield_judgment_contacts.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;using namespace combat::weapon;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Shield Judgment contacts line "+std::to_string(at.line()));}
int main()try{const Scope caster{1,1,1},defender{1,2,1},attacker{1,3,1};const Entry effect{10,1,262,12,Category::enchantment,1000,19000};unsigned cases=0;
 for(auto delivery:{AttackDelivery::melee,AttackDelivery::ranged})for(unsigned mode=0;mode<13;++mode){++cases;ShieldJudgmentContacts contacts(defender);AttackOutcomes outcomes(attacker);
  check(outcomes.release(attacker,defender,1,1000,delivery));check(contacts.release(defender,attacker,1,1000,delivery));const auto hit=outcomes.qualify_hit(attacker,defender,1,1100);check(bool(hit));
  check(contacts.prepare(defender,caster,effect,*hit,true,{false,{}},20));check(!contacts.prepare(defender,caster,effect,*hit,true,{false,{}},20));
  const auto outcome=mode<5?AttackOutcome::hit:mode==5?AttackOutcome::cancelled:mode==6?AttackOutcome::target_retired:AttackOutcome::hit;
  const auto terminal=outcomes.finish(attacker,defender,1,outcome,1100);check(bool(terminal));auto changed=*terminal;
  if(mode==1)++changed.target.generation;if(mode==2)++changed.attacker.generation;if(mode==3)++changed.event;if(mode==4)changed.outcome=static_cast<AttackOutcome>(99);
  if(mode==7)changed.before_damage.reset();if(mode==8)++changed.released;if(mode==9)++changed.at;if(mode==10)changed.delivery=delivery==AttackDelivery::melee?AttackDelivery::ranged:AttackDelivery::melee;
  if(mode==11)changed.before_damage=1099;if(mode==12)check(contacts.retire(defender));
  const auto reaction=contacts.consume(defender,changed);check(bool(reaction)==(mode==0));
  if(mode>=1&&mode<=4){check(contacts.size()==1);check(bool(contacts.consume(defender,*terminal)));}else check(contacts.size()==0);
  check(!contacts.release(defender,attacker,1,1000,delivery));check(!contacts.consume(defender,*terminal));check(!contacts.prepare(defender,caster,effect,*hit,true,{false,{}},20));
 }
 // Projectiles may land out of release order. Event IDs are monotonically
 // assigned at release,not at contact:prepare must allow this exact scenario.
 {ShieldJudgmentContacts contacts(defender);AttackOutcomes a(attacker);check(a.release(attacker,defender,1,1000,AttackDelivery::ranged));check(a.release(attacker,defender,2,1001,AttackDelivery::ranged));check(contacts.release(defender,attacker,1,1000,AttackDelivery::ranged));check(contacts.release(defender,attacker,2,1001,AttackDelivery::ranged));
  const auto second=a.qualify_hit(attacker,defender,2,1100);check(bool(second));check(contacts.prepare(defender,caster,effect,*second,true,{false,{}},20));
  check(bool(contacts.consume(defender,*a.finish(attacker,defender,2,AttackOutcome::hit,1100))));
  const auto first=a.qualify_hit(attacker,defender,1,1200);check(bool(first));check(contacts.prepare(defender,caster,effect,*first,true,{false,{}},20));
  check(bool(contacts.consume(defender,*a.finish(attacker,defender,1,AttackOutcome::hit,1200))));
 }
 // Unprepared contacts still retire on terminal failure. Capacity is bounded
 // by outstanding releases,not the total number of attacks in a long session.
 {ShieldJudgmentContacts contacts(defender);
  for(unsigned event=1;event<=4096;++event)check(contacts.release(defender,attacker,event,1000,AttackDelivery::ranged));
  check(!contacts.release(defender,attacker,4097,1000,AttackDelivery::ranged));check(contacts.size()==4096);
  for(unsigned event=1;event<=4096;++event){ResolvedAttack terminal{attacker,defender,event,1000,1100,AttackOutcome::cancelled,{},AttackDelivery::ranged};check(!contacts.consume(defender,terminal));}
  check(contacts.size()==0&&contacts.release(defender,attacker,4097,1200,AttackDelivery::ranged));
  check(!contacts.release(defender,attacker,1,1200,AttackDelivery::ranged));
 }
 // Actor reuse cannot retarget the old generation's contact. Retire only the
 // exact source;another source's in-flight release remains intact.
 {ShieldJudgmentContacts contacts(defender);auto replacement=attacker;++replacement.generation;const Scope another{1,4,1};
  check(contacts.release(defender,attacker,1,1000,AttackDelivery::ranged));check(contacts.release(defender,another,1,1000,AttackDelivery::ranged));
  QualifiedHit old{attacker,defender,1,1000,1100,AttackDelivery::ranged};check(contacts.prepare(defender,caster,effect,old,true,{false,{}},20));
  auto wrong_owner=defender;++wrong_owner.generation;check(!contacts.retire_attacker(wrong_owner,attacker)&&contacts.size()==2);
  check(contacts.retire_attacker(defender,attacker)&&contacts.size()==1);check(!contacts.consume(defender,{attacker,defender,1,1000,1100,AttackOutcome::hit,1100,AttackDelivery::ranged}));
  check(contacts.release(defender,replacement,1,1000,AttackDelivery::ranged));auto fresh=old;fresh.attacker=replacement;
  check(contacts.prepare(defender,caster,effect,fresh,true,{false,{}},20));const auto reaction=contacts.consume(defender,{replacement,defender,1,1000,1100,AttackOutcome::hit,1100,AttackDelivery::ranged});check(reaction&&reaction->attacker==replacement);
  check(contacts.size()==1);check(contacts.retire(defender)&&contacts.size()==0);check(!contacts.release(defender,replacement,2,1200,AttackDelivery::ranged));
 }
 std::cout<<"PASS26 scoped Shield of Judgment terminal consumption cases plus out-of-order projectiles,4096-release capacity and source-generation retirement\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
