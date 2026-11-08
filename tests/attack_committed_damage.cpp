#include "gw_attack_outcome_delivery.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace combat::weapon;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Committed damage line "+std::to_string(at.line()));
}
static_assert(!std::is_constructible_v<CommittedAttackDamage,ResolvedAttack,combat::Hit>);
int main()try{
 const AttackScope a{1,1,10},b{1,2,20},c{1,3,30};unsigned cases=0;
 for(unsigned mode=0;mode<11;++mode){++cases;AttackOutcomeDelivery ledger(a);
  check(ledger.release(a,b,1,100,AttackDelivery::ranged));
  combat::Actor actor{2,20,combat::Health{50,100},{},0,true};
  auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,mode==9?80.:mode==10?0.:20.,20,320},true);
  check(hit.reason==combat::Reason::accepted);
  check(!ledger.commit_damage(a,b,1,hit,100));
  check(bool(ledger.qualify_hit(a,b,1,200)));
  auto caller=a,target=b;unsigned event=1,at=200;
  if(mode==1)++caller.generation;if(mode==2)++target.generation;if(mode==3)++event;
  if(mode==4)--at;if(mode==5)++at;if(mode==6)++hit.target;
  if(mode==7)++hit.generation;if(mode==8)++hit.applied;
  auto receipt=ledger.commit_damage(caller,target,event,hit,at);
  const bool valid=mode==0||mode==9||mode==10;check(bool(receipt)==valid);
  if(!valid)continue;
  check(receipt->attack().target==b&&receipt->attack().attacker==a&&receipt->attack().before_damage==200);
  check(!ledger.commit_damage(a,b,1,hit,200));
  check(!ledger.finish(a,b,1,AttackOutcome::cancelled,200));
  auto done=ledger.finish(a,b,1,AttackOutcome::hit,200);check(bool(done));
  check(ledger.acknowledge(a,*done)&&ledger.buffered_size()==0);
 }
 // A newer projectile can commit first without retiring its older sibling.
 {AttackOutcomeDelivery ledger(a);check(ledger.release(a,b,1,100)&&ledger.release(a,c,2,110));
  for(auto target:{c,b}){const auto event=target==c?2:1,at=target==c?200:300;
   combat::Actor actor{target.actor,target.generation,combat::Health{50,100},{},0,true};
   auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,10,20,320},true);
   check(bool(ledger.qualify_hit(a,target,event,at))&&bool(ledger.commit_damage(a,target,event,hit,at)));
   check(bool(ledger.finish(a,target,event,AttackOutcome::hit,at)));
  }
  auto events=*ledger.buffered(a);check(events.size()==2&&events[0].event==2&&events[1].event==1);
  check(ledger.acknowledge(a,events[0])&&ledger.acknowledge(a,events[1]));
 }
 std::cout<<"PASS "<<cases<<" committed-damage validation cases plus out-of-order projectile delivery\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
