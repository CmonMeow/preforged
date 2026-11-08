#include "gw_attack_interrupt_disable.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace combat::weapon;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("attack disable line "+std::to_string(at.line()));
}
int main()try{
 const skills::effects::Scope owner{1,2,3};
 for(unsigned skill:{340u,399u}){
  const auto plan=*interrupt_disable_plan(skill);AttackInterruptDisables state(owner);
  InterruptedActionReceipt receipt{owner,{281,1},1,100,1000,500,8500};
  const auto first=state.propose(owner,plan,receipt,500);check(first.has_value());
  check(state.disabled_until(owner,{281,1},500)==0); // Proposal is immutable.
  state=*first;check(state.disabled_until(owner,{281,1},500)==28500);
  check(state.disabled_until(owner,{281,0},500)==0&&state.disabled_until(owner,{282,1},500)==0);
  check(!state.propose(owner,plan,receipt,500)); // Duplicate action cannot extend disable.
  receipt.action=2;receipt.recharge_until=500;
  auto shorter=state.propose(owner,plan,receipt,500);check(shorter&&shorter->disabled_until(owner,{281,1},500)==28500);
  receipt.action=3;receipt.recharge_until=10000;
  auto longer=state.propose(owner,plan,receipt,500);check(longer&&longer->disabled_until(owner,{281,1},500)==30000);
  check(longer->disabled_until(owner,{281,1},30000)==0);
  const auto reset=state.propose_reset_slots(owner,{{281,1}},600);check(reset&&reset->disabled_until(owner,{281,1},600)==0);
  check(!reset->propose(owner,plan,receipt,500));
  auto replay=receipt;replay.action=1;replay.interrupted_at=600;check(!reset->propose(owner,plan,replay,600));
  for(unsigned mode=0;mode<9;++mode){auto invalid=receipt;
   if(mode==0)invalid.target.generation++;if(mode==1)invalid.slot.skill=0;if(mode==2)invalid.slot.copy=65536;
   if(mode==3)invalid.action=0;if(mode==4)invalid.ends=500;if(mode==5)invalid.began=501;
   if(mode==6)invalid.interrupted_at=501;if(mode==7)invalid.recharge_until=499;if(mode==8)invalid.recharge_until=UINT64_MAX;
   check(!AttackInterruptDisables(owner).propose(owner,plan,invalid,500));
  }
  auto forged=plan;forged.extra=19999;check(!state.propose(owner,forged,receipt,500));
  auto retired=state;retired.retire();check(!retired.disabled_until(owner,{281,1},500));
  // Long sessions do not exhaust a receipt set or accumulate expired rules.
  AttackInterruptDisables sustained(owner);
  for(std::uint64_t n=1;n<=5000;++n){const auto at=n*30000;
   const InterruptedActionReceipt next{owner,{281,1},n,at,at+100,at,at};
   const auto proposed=sustained.propose(owner,plan,next,at);check(proposed.has_value());sustained=*proposed;
  }
  check(sustained.disabled_until(owner,{281,1},150000000)==150020000);
 }
 check(!interrupt_disable_plan(426));
 std::cout<<"PASS exact interrupted slot/copy,recharge plus20s,immutable proposals,longer preservation,expiry/reset,replay/lifetime/clock/overflow guards;production integration pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
