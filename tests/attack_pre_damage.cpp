#include "gw_attack_outcome_batch.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("Pre-damage attack phase regression");}
int main()try{
 const AttackScope source{1,2,3},target{1,4,5},other{1,6,7};AttackOutcomes ledger(source);
 check(ledger.release(source,target,1,100));
 check(!ledger.qualify_hit({1,2,4},target,1,120)&&!ledger.qualify_hit(source,{1,4,6},1,120));
 check(!ledger.qualify_hit(source,target,9,120)&&!ledger.qualify_hit(source,target,1,99));
 const auto hit=ledger.qualify_hit(source,target,1,150);check(hit&&hit->attacker==source&&hit->target==target&&hit->event==1&&hit->released==100&&hit->at==150&&ledger.size()==1);
 // Invalid/duplicate requests cannot consume the hit or poison its clock.
 check(!ledger.qualify_hit(source,target,1,10000));
 for(auto failed:{AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction})check(!ledger.finish(source,target,1,failed,10000));
 const auto done=ledger.finish(source,target,1,AttackOutcome::hit,160);check(done&&done->before_damage==150&&ledger.size()==0);
 check(!ledger.qualify_hit(source,target,1,170)&&!ledger.finish(source,target,1,AttackOutcome::hit,170));
 check(ledger.release(source,target,2,200)&&ledger.release(source,other,3,200));
 check(ledger.qualify_hit(source,other,3,250).has_value());
 check(ledger.finish(source,other,3,AttackOutcome::cancelled,260).has_value());
 check(ledger.qualify_hit(source,target,2,270).has_value());
 const std::array additions{AttackRelease{other,4}};auto staged=stage_attack_outcome_batch(ledger,source,additions,280);check(bool(staged));
 check(!staged->qualify_hit(source,target,2,290)); // Copy preserves once-only state.
 check(staged->finish(source,target,2,AttackOutcome::target_retired,290).has_value());
 staged->discard_target(other);check(!staged->qualify_hit(source,other,4,300)&&staged->size()==0);
 check(ledger.retire(source)&&!ledger.qualify_hit(source,target,2,300));
 std::cout<<"PASS pre-damage once-only hit qualification,scopes/clock,phase consistency,out-of-order completion,batch copy and retirement\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
