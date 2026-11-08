// Unregistered candidate ledger contract; no gameplay reaction inferred here.
#include "gw_attack_outcome_delivery.h"
#include <stdexcept>
#include <iostream>
using namespace fw::gw::combat::weapon;
void check(bool value){if(!value)throw std::runtime_error("release observer contract");}
int main(){
 AttackScope a{1,1,1},b{1,2,1},c{1,3,1};AttackOutcomes ledger(a);
 check(ledger.release(a,b,1,100,AttackDelivery::ranged));check(ledger.release(a,c,2,101,AttackDelivery::ranged));
 check(!ledger.observe_release_once(a,c,1,100,1));check(!ledger.observe_release_once(a,b,1,99,1));check(!ledger.observe_release_once(a,b,1,100,3));
 auto unseen=ledger.release_observed(a,c,2,101,1);check(unseen&&!*unseen);
 auto first=ledger.observe_release_once(a,c,2,101,1);check(first&&*first);
 auto seen=ledger.release_observed(a,c,2,101,1);check(seen&&*seen);
 auto old=ledger.observe_release_once(a,b,1,102,1);check(old&&*old); // old retained projectile is not a watermark duplicate
 auto repeated=ledger.observe_release_once(a,b,1,103,1);check(repeated&&!*repeated);
 auto independent=ledger.observe_release_once(a,b,1,103,2);check(independent&&*independent);
 std::cout<<"PASS per-pending release observer identity, chronology and independent grains\n";
}
