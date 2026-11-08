#include "gw_attack_outcome_delivery.h"
#include "gw_price_of_failure.h"
#include <stdexcept>
using namespace fw::gw::combat::weapon;
static void check(bool v){if(!v)throw std::runtime_error("typed failed attack ledger");}
int main(){
 const AttackScope source{1,25,1},target{1,30,2};AttackOutcomeDelivery ledger(source);
 check(ledger.release(source,target,1,100,AttackDelivery::melee));
 auto failed=ledger.finish(source,target,1,AttackOutcome::cancelled,100,true);
 check(failed&&failed->skill_failed&&failed->failed_hit()&&!failed->before_damage);
 auto forged=*failed;forged.skill_failed=false;check(!ledger.acknowledge(source,forged));check(ledger.acknowledge(source,*failed));
 check(!ledger.finish(source,target,1,AttackOutcome::cancelled,100,true));
 check(ledger.release(source,target,2,101,AttackDelivery::melee));
 check(ledger.qualify_hit(source,target,2,101).has_value());
 check(!ledger.finish(source,target,2,AttackOutcome::cancelled,101,true));
 auto cancelled=ledger.finish(source,target,2,AttackOutcome::cancelled,101);
 check(cancelled&&!cancelled->skill_failed&&!cancelled->failed_hit());check(ledger.acknowledge(source,*cancelled));
 check(ledger.release(source,target,3,102,AttackDelivery::melee));
 check(!ledger.finish(source,target,3,AttackOutcome::miss,102,true));
 check(!ledger.finish(source,target,3,AttackOutcome::hit,102,true));
 auto ordinary=ledger.finish(source,target,3,AttackOutcome::cancelled,102);
 check(ordinary&&!ordinary->skill_failed&&!ordinary->failed_hit());
}
