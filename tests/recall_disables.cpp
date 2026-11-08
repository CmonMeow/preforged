#include "gw_recall_disables.h"
#include <stdexcept>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok){if(!ok)throw std::runtime_error("Recall disables");}
int main(){
 const Scope owner{1,1,1};Slots slots{};slots[0]={925,0};slots[1]={74,0};slots[2]={293,0};slots[3]={281,1};slots[4]={281,2};
 CooldownBarAuthority authority(owner);const auto bar=authority.publish(owner,slots,1000);check(bool(bar));
 RecallDisables original(owner);const auto proposed=original.propose(owner,*bar,1,2000);check(bool(proposed));
 for(unsigned n=0;n<5;++n){check(original.disabled_until(owner,slots[n],2000)==0);
  check(proposed->disabled_until(owner,slots[n],11999)==12000);check(proposed->disabled_until(owner,slots[n],12000)==0);}
 check(proposed->disabled_until(owner,{281,0},2000)==0); // Exact copy identities.
 check(!proposed->propose(owner,*bar,1,2001)); // One end event cannot be replayed.
 check(!original.propose(owner,*bar,0,2000));
 check(!original.propose(owner,*bar,1,UINT64_MAX));
 auto stale=owner;++stale.generation;check(!proposed->disabled_until(stale,{925,0},2000));
 const auto reset=proposed->propose_reset_slots(owner,{{293,0}},2500);check(bool(reset));
 check(reset->disabled_until(owner,{293,0},2500)==0&&reset->disabled_until(owner,{74,0},2500)==12000);
 check(!reset->propose(owner,*bar,1,3000)); // Reset preserves replay watermark.
 auto corrupt=*bar;corrupt.bar[5]=corrupt.bar[3];check(!original.propose(owner,corrupt,1,2000));
 auto retired=*proposed;retired.retire();check(!retired.disabled_until(owner,{925,0},2000));
}
