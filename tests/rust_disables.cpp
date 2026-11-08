#include "gw_rust_disables.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::skills;using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Rust disable line "+std::to_string(at.line()));
}
int main()try{
 const Scope owner{1,2,3};RustDisables state(owner);const auto plan=*rust(15);
 auto applied=state.propose(owner,plan,{{1,0},50,500},true,100);check(bool(applied));
 check(state.disabled_until(owner,{1,0},100)==0);state=*applied;
 check(state.disabled_until(owner,{1,0},100)==10100);
 check(state.disabled_until(owner,{1,1},100)==0&&state.disabled_until(owner,{63,0},100)==0);
 check(state.disabled_until(owner,{1,0},10100)==0);
 check(!state.disabled_until({1,2,4},{1,0},100)&&!state.disabled_until(owner,{1,0},99));
 check(!state.propose(owner,plan,{{23,0},100,500},true,200));
 check(!state.propose(owner,plan,{{1,0},100,500},false,200));
 check(!state.propose(owner,plan,{{1,0},100,500},true,500));
 auto reset=state.propose_reset_slots(owner,{{1,1}},200);check(reset&&reset->disabled_until(owner,{1,0},200)==10100);
 reset=state.propose_reset_slots(owner,{{1,0}},200);check(reset&&reset->disabled_until(owner,{1,0},200)==0);
 auto shorter=reset->propose(owner,*rust(0),{{1,0},200,600},true,300);
 check(shorter&&shorter->disabled_until(owner,{1,0},300)==1300); // Reset must not resurrect the old longer layer.
 auto renewal=state.propose(owner,*rust(0),{{1,0},100,600},true,300);
 check(renewal&&renewal->disabled_until(owner,{1,0},300)==10100);
 check(!state.propose_reset_slots(owner,{{1,0},{1,0}},200));
 auto stale=owner;++stale.generation;check(!state.propose(stale,plan,{{1,0},100,500},true,200));
 state.retire();check(!state.disabled_until(owner,{1,0},200));
 std::cout<<"PASS Rust single-slot disable ownership,expiry,renewal,reset layers and stale-lifetime rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
