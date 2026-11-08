#include "gw_attack_outcomes.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::combat::weapon;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Attack outcome regression line "+std::to_string(at.line()));}
int main()try{
 const AttackScope a{1,1,10},b{1,2,20},c{1,3,30};AttackOutcomes ledger(a);
 for(unsigned type:{2u,15u,27u,32u,35u})check(weapon_delivery(type,false)==AttackDelivery::melee&&weapon_delivery(type,true)==AttackDelivery::unknown);
 for(unsigned type:{5u,36u})check(weapon_delivery(type,false)==AttackDelivery::ranged);
 for(unsigned type:{22u,26u})check(weapon_delivery(type,true)==AttackDelivery::ranged&&weapon_delivery(type,false)==AttackDelivery::unknown);
 check(weapon_delivery(0,false)==AttackDelivery::unknown&&weapon_delivery(255,true)==AttackDelivery::unknown);
 {
  AttackOutcomes deliveries(a);
  check(!deliveries.release(a,b,1,100,static_cast<AttackDelivery>(99))&&deliveries.size()==0);
  check(deliveries.release(a,b,1,100,AttackDelivery::ranged));
  check(deliveries.release(a,c,2,110,AttackDelivery::melee));
  const auto melee=deliveries.qualify_hit(a,c,2,120);
  check(melee&&melee->delivery==AttackDelivery::melee&&!deliveries.qualify_hit(a,c,2,120));
  const auto done=deliveries.finish(a,c,2,AttackOutcome::hit,120);
  check(done&&done->delivery==AttackDelivery::melee&&done->before_damage==120);
  const auto ranged=deliveries.qualify_hit(a,b,1,130);
  check(ranged&&ranged->delivery==AttackDelivery::ranged);
  const auto delayed=deliveries.finish(a,b,1,AttackOutcome::hit,130);
  check(delayed&&delayed->delivery==AttackDelivery::ranged&&!deliveries.finish(a,b,1,AttackOutcome::hit,130));
  check(deliveries.release(a,b,3,140));
  const auto unknown=deliveries.finish(a,b,3,AttackOutcome::block,140);
  check(unknown&&unknown->delivery==AttackDelivery::unknown);
 }
 check(ledger.release(a,b,1,100)&&ledger.release(a,c,2,200));
 check(!ledger.finish(a,b,1,AttackOutcome::miss,99));
 check(!ledger.finish({1,1,11},b,1,AttackOutcome::miss,300));
 check(!ledger.finish(a,{1,2,21},1,AttackOutcome::miss,300));
 check(!ledger.finish(a,b,1,static_cast<AttackOutcome>(999),300)&&ledger.size()==2);
 const auto fast=ledger.finish(a,c,2,AttackOutcome::dodge,250);check(fast&&fast->failed_hit()&&fast->event==2);
 const auto slow=ledger.finish(a,b,1,AttackOutcome::miss,1000);check(slow&&slow->failed_hit()&&slow->released==100);
 check(!ledger.finish(a,b,1,AttackOutcome::miss,1000)&&!ledger.release(a,b,1,1001));
 unsigned triggered=0;std::uint64_t id=2;
 for(auto outcome:{AttackOutcome::hit,AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::target_retired,AttackOutcome::cancelled}){
  const auto at=2000+300*id;check(ledger.release(a,b,++id,at));const auto result=ledger.finish(a,b,id,outcome,at+100);check(result.has_value());
  triggered+=result->failed_hit();check(!ledger.finish(a,b,id,outcome,at+100));
 }
 check(triggered==4&&ledger.size()==0);
 check(!ledger.release(a,a,++id,6000)&&!ledger.release(a,{2,2,20},++id,6000));
 check(ledger.release(a,b,++id,6000));ledger.discard_target({1,2,21});check(ledger.size()==1);
 check(!ledger.finish(a,b,id,AttackOutcome::miss,5999)&&ledger.size()==1);
 ledger.discard_target(b);check(!ledger.finish(a,b,id,AttackOutcome::obstruction,6100));
 check(ledger.release(a,c,++id,6200));check(!ledger.retire({1,1,11})&&ledger.size()==1);
 check(ledger.retire(a)&&ledger.size()==0&&!ledger.release(a,b,++id,6300));
 AttackOutcomes capacity(a);for(unsigned event=1;event<=4096;++event)check(capacity.release(a,b,event,event));
 check(!capacity.release(a,c,4097,4097)&&capacity.size()==4096);
 check(capacity.finish(a,b,1,AttackOutcome::hit,4097).has_value()&&capacity.release(a,c,4097,4097));
 std::cout<<"PASS released attack outcomes,out-of-order completion,exactly-once consumption,lifetimes and bounded capacity;world wiring pending\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
