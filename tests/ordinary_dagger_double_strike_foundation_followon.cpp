// NEW unregistered foundation; compile against detached candidate headers first.
#include "gw_ordinary_dagger_chance_followon.h"
#include "gw_attack_outcome_delivery.h"
#include <stdexcept>
#include <limits>
using namespace fw::gw;using namespace combat::weapon;
static void check(bool value){if(!value)throw std::runtime_error("ordinary dagger canonical double-strike foundation");}
int main(){
 using P=OrdinaryDaggerChancePolicyFollowon;P unknown;
 check(!select_ordinary_dagger_contacts_followon(unknown,32,12,false,0));
 const P legacy{P::Basis::legacy_rank_only_declared,"explicit legacy rank-only comparison"};
 const P innate{P::Basis::explicit_inherent_two,"local Dagger explicit innate2% + rank2%, reviewed declaration"};
 for(unsigned rank=0;rank<=20;++rank){
  const auto old=select_ordinary_dagger_contacts_followon(legacy,32,rank,false,.99);
  const auto current=select_ordinary_dagger_contacts_followon(innate,32,rank,false,.99);
  check(old&&current&&old->percent==rank*2&&current->percent==rank*2+2&&current->contacts==1);
 }
 check(select_ordinary_dagger_contacts_followon(innate,32,0,false,0)->contacts==2);
 check(select_ordinary_dagger_contacts_followon(legacy,32,0,false,0)->contacts==1);
 check(select_ordinary_dagger_contacts_followon(innate,32,12,false,.26)->contacts==1);
 check(select_ordinary_dagger_contacts_followon(innate,32,12,false,.259)->contacts==2);
 check(select_ordinary_dagger_contacts_followon(innate,32,20,true,.919)->contacts==2);
 check(!select_ordinary_dagger_contacts_followon(innate,5,12,false,0)); // Arrows unaffected.
 check(!select_ordinary_dagger_contacts_followon(innate,32,21,false,0));
 check(!select_ordinary_dagger_contacts_followon(innate,32,12,false,1));
 check(!select_ordinary_dagger_contacts_followon(innate,32,12,false,std::numeric_limits<double>::quiet_NaN()));
 const AttackScope source{1,1,7},target{1,2,4};AttackOutcomeDelivery ledger(source);
 const auto reservation=ledger.reserve_ordinary_action(source,1,100,9);check(bool(reservation));
 const auto made=ledger.make_action(source,*reservation);check(bool(made));
 check(!ledger.declare_ordinary_contacts(source,*made,2,10));
 check(ledger.declare_ordinary_contacts(source,*made,2,9));check(!ledger.declare_ordinary_contacts(source,*made,1,9));
 auto once=ledger.observe_action_once(source,*made,2);check(once&&*once);
 once=ledger.observe_action_once(source,*made,2);check(once&&!*once); // Vigor/fee/source action once.
 check(!ledger.release_action_contact(source,*made,target,2,100,1,9));
 check(ledger.release_action_contact(source,*made,target,2,100,0,9));
 check(ledger.action_contact_ordinal(source,*made,target,2)==std::optional(0u));
 check(!ledger.release_action_contact(source,*made,target,3,100,0,9));
 auto contact_seen=ledger.observe_release_once(source,target,2,100,1);check(contact_seen&&*contact_seen);
 contact_seen=ledger.observe_release_once(source,target,2,100,1);check(contact_seen&&!*contact_seen);
 const auto first=ledger.finish(source,target,2,AttackOutcome::miss,100);check(bool(first));
 // No guessed delay: second contact timestamp explicitly supplied by producer.
 check(!ledger.release_action_contact(source,*made,target,3,101,1,10));
 check(ledger.release_action_contact(source,*made,target,3,101,1,9));
 check(ledger.action_contact_ordinal(source,*made,target,3)==std::optional(1u));
 contact_seen=ledger.observe_release_once(source,target,3,101,1);check(contact_seen&&*contact_seen);
 const auto second=ledger.finish(source,target,3,AttackOutcome::block,101);check(bool(second));
 check(!ledger.release_action_contact(source,*made,target,4,101,2,9));
 check(ledger.finish_action(source,*made,101));
 check(!ledger.release_action_contact(source,*made,target,4,101,1,9));
 check(ledger.acknowledge(source,*first)&&ledger.acknowledge(source,*second));
 // Actual early stop: close source action after first contact; no fabricated second.
 auto held=ledger.reserve_ordinary_action(source,4,200,11);check(bool(held));auto stopped=ledger.make_action(source,*held);check(bool(stopped));
 check(ledger.declare_ordinary_contacts(source,*stopped,2,11));
 check(ledger.release_action_contact(source,*stopped,target,5,200,0,11));
 const auto cancelled=ledger.finish(source,target,5,AttackOutcome::target_retired,200);check(bool(cancelled));
 check(ledger.finish_action(source,*stopped,200));check(!ledger.release_action_contact(source,*stopped,target,6,200,1,11));
 check(ledger.acknowledge(source,*cancelled));
 // Existing projectile API keeps independent delivery and buffering semantics.
 check(ledger.release(source,target,6,201,AttackDelivery::ranged));const auto arrow=ledger.finish(source,target,6,AttackOutcome::miss,205);check(bool(arrow));check(ledger.acknowledge(source,*arrow));
 held=ledger.reserve_ordinary_action(source,7,206,12);check(bool(held));const auto retired=ledger.make_action(source,*held);check(bool(retired));check(ledger.declare_ordinary_contacts(source,*retired,2,12));
 check(ledger.retire(source));check(!ledger.release_action_contact(source,*retired,target,8,206,0,12));
}
