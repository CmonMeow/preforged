#include "gw_rust.h"
#include "gw_activation_modifiers.h"
#include "gw_cast_timing.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Rust policy line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills::effects;
 const combat::Actor base{2,1,combat::Health{1000,1000},{},0,true,{},{},60};const combat::conditions::State clean;
 for(unsigned rank=0;rank<=20;++rank)for(bool overcast:{false,true})for(unsigned active:{1u,63u,23u}){
  const auto plan=rust(rank);check(bool(plan));
  const auto r=rust_target(*plan,base,clean,20,overcast,RustActivity{{active,2},50,500},100);
  check(r.accepted&&r.hit.applied==10+4*rank&&r.hex_ms==(5+rank)*1000);
  check(r.active_signet.has_value()==(overcast&&active!=23));
  if(r.active_signet)check(r.active_signet->skill==active&&r.active_signet->copy==2&&r.disable_ms==1000ull*interpolate(1,10,rank));
 }
 const auto plan=*rust(15);
 for(auto now:{49ull,50ull,499ull,500ull}){
  const auto r=rust_target(plan,base,clean,20,true,RustActivity{{1,0},50,500},now);
  check(r.accepted&&r.active_signet.has_value()==(now>=50&&now<500));
 }
 auto dead=base;dead.health->current=1;auto r=rust_target(plan,dead,clean,20,true,RustActivity{{1,0},50,500},100);
 check(r.accepted&&r.hit.killed&&!r.hex_ms&&!r.active_signet);
 auto actor=base;actor.incoming_reduction=5;r=rust_target(plan,actor,clean,20,false,{},100);check(r.accepted&&r.hit.applied==65);
 actor=base;actor.cold_armor.reset();check(!rust_target(plan,actor,clean,20,false,{},100).accepted);
 auto forged=plan;forged.radius=322;check(!rust_target(forged,base,clean,20,false,{},100).accepted);
 check(!rust(21)&&!rust_target(plan,base,clean,20,true,RustActivity{{0,0},50,500},100).accepted);
 std::vector<Entry> entries{{1,1,204,15,Category::hex,100,1000}};
 const auto& signet=*skills::find_skill(1);const auto& spell=*skills::find_skill(23);const auto& attack=*skills::find_skill(359);
 check(activation_multiplier(signet,entries,clean,99)==1&&activation_multiplier(signet,entries,clean,100)==2&&activation_multiplier(signet,entries,clean,1000)==1);
 check(activation_multiplier(spell,entries,clean,100)==1&&activation_multiplier(attack,entries,clean,100)==1);
 entries.push_back({2,1,53,15,Category::hex,100,1000});
 check(activation_multiplier(signet,entries,clean,100)==2.5);
 check(activation_multiplier(spell,entries,clean,100)==2);
 entries.erase(entries.begin());check(activation_multiplier(signet,entries,clean,100)==2);
 check(skills::timing::activation(signet,5,10,2.)==std::uint64_t(std::llround(signet.activation*.7*2*1000)));
 std::cout<<"PASS Rust native ranks,cold recipient proposal,active-signet-only disable,signet activation modifier,stack cap and expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
