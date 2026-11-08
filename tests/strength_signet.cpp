#include "gw_strength_signet.h"
#include <iostream>
#include <string>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;using namespace combat::weapon;
void check(bool b,std::source_location l=std::source_location::current()){if(!b)throw std::runtime_error("Strength Signet line "+std::to_string(l.line()));}
int main()try{
 const Scope owner{1,25,1},target{1,31,1};
 for(unsigned rank=0;rank<=20;++rank){
  const auto p=strength_signet(rank);check(p&&p->attacks==rank+1&&p->bonus==5);
  Registry registry(owner);check(registry.apply(owner,{67,25,944,rank,Category::other,100,{}},Replace::same_skill).accepted);
  StrengthSignetLedger original(owner);check(original.attach(owner,registry,67,100));auto ledger=original;auto copy=registry;
  for(unsigned n=1;n<=p->attacks;++n){AttackOutcomes attacks(owner);check(attacks.release(owner,target,n,100+n,AttackDelivery::melee));const auto hit=*attacks.qualify_hit(owner,target,n,200+n);
   check(ledger.consume_landed_hit(owner,copy,hit)==5&&ledger.remaining()==p->attacks-n);
   check(!ledger.consume_landed_hit(owner,copy,hit));check(bool(copy.lookup(owner,67))==(n<p->attacks));}
  check(original.remaining()==p->attacks&&registry.lookup(owner,67));
 }
 check(!strength_signet(21));Registry registry(owner);check(registry.apply(owner,{67,25,944,2,Category::other,100,{}},Replace::same_skill).accepted);StrengthSignetLedger ledger(owner);check(ledger.attach(owner,registry,67,100));
 QualifiedHit hit{owner,target,1,101,200,AttackDelivery::ranged};
 auto bad=hit;bad.released=99;check(!ledger.consume_landed_hit(owner,registry,bad));bad=hit;bad.target.generation=0;check(!ledger.consume_landed_hit(owner,registry,bad));bad=hit;bad.delivery=AttackDelivery::unknown;check(!ledger.consume_landed_hit(owner,registry,bad));
 check(!ledger.consume_landed_hit({1,25,2},registry,hit));
 check(ledger.consume_landed_hit(owner,registry,hit)==5);hit.event=3;hit.at=201;check(ledger.consume_landed_hit(owner,registry,hit)==5);hit.event=2;hit.at=202;check(ledger.consume_landed_hit(owner,registry,hit)==5); // Release IDs may finish out of order.
 check(registry.apply(owner,{68,25,944,1,Category::other,203,{}},Replace::same_skill).accepted);check(ledger.attach(owner,registry,68,203));hit.at=204;hit.released=203;check(!ledger.consume_landed_hit(owner,registry,hit)); // Replacement preserves replay receipts.
 check(!ledger.attach(owner,registry,67,203));hit.event=4;check(ledger.consume_landed_hit(owner,registry,hit)==5);check(registry.remove(owner,68).has_value());hit.event=5;hit.at=205;check(!ledger.consume_landed_hit(owner,registry,hit));
 check(ledger.detach(owner,205));check(!ledger.detach(owner,204));ledger.retire();check(!ledger.attach(owner,registry,69,206));
 std::cout<<"PASS21 Signet of Strength plans and detached landed-hit charges; duplicate/out-of-order/replacement/removal/lifetime guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
