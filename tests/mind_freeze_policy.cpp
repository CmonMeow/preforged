#include "gw_mind_freeze.h"
#include "gw_movement_effects.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mind Freeze policy line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills::effects;
 const combat::Actor base{2,1,combat::Health{1000,1000},{},0,true,{},{},60};
 const combat::conditions::State clean;
 for(unsigned skill:{209u,2803u})for(unsigned rank=0;rank<=20;++rank)for(std::int64_t energy:{299999,300000,300001}){
  const auto p=mind_freeze(skill,rank);check(bool(p));
  check(p->damage==damage_scale(10,skill==209?60:40,rank)&&p->duration==1000ull*interpolate(1,5,rank));
  const auto r=mind_freeze_target(*p,base,clean,20,energy,300000,100);
  check(r.accepted&&r.energy_advantage==(energy>300000)&&r.hits.size()==(energy>300000?2:1));
  check(std::abs(r.actor.health->current-(1000-p->damage*r.hits.size()))<1e-9);
  check(r.snare_ms==(energy>300000?p->duration:0));
  for(const auto& hit:r.hits)check(hit.skill==skill&&hit.kind==combat::Kind::cold);
 }
 const auto plan=*mind_freeze(209,15);
 for(unsigned hp:{1u,60u,61u,120u,121u}){
  auto actor=base;actor.health->current=hp;const auto r=mind_freeze_target(plan,actor,clean,20,2,1,100);
  check(r.accepted&&r.hits.size()==(hp<=60?1:2)&&r.hits.back().killed==(hp<=120));
  check(r.snare_ms==(hp>120?5000:0));
 }
 auto actor=base;actor.incoming_reduction=5;
 auto r=mind_freeze_target(plan,actor,clean,20,2,1,100);check(r.accepted&&r.hits.size()==2&&r.actor.health->current==890);
 auto cracked=clean;check(cracked.apply(combat::conditions::Kind::cracked_armor,1,100,5000));actor=base;actor.cold_armor=80;
 r=mind_freeze_target(plan,actor,cracked,20,2,1,100);check(r.accepted&&r.actor.health->current==880);
 check(!mind_freeze(209,21)&&!mind_freeze(226,12));
 auto forged=plan;forged.multiplier=.5f;check(!mind_freeze_target(forged,base,clean,20,2,1,100).accepted);
 check(!mind_freeze_target(plan,base,clean,20,{},1,100).accepted);
 check(!mind_freeze_target(plan,base,clean,20,2,{},100).accepted);
 check(!mind_freeze_target(plan,base,clean,20,300000000001ll,1,100).accepted);
 check(!mind_freeze_target(plan,base,clean,20,2,1,UINT64_MAX).accepted);
 check(mind_freeze_target(plan,base,clean,20,1,1,UINT64_MAX).accepted);
 check(!mind_freeze_target(plan,base,clean,20,-1,0,100).energy_advantage);
 check(mind_freeze_target(plan,base,clean,20,0,-1,100).energy_advantage);
 actor=base;actor.cold_armor.reset();check(!mind_freeze_target(plan,actor,clean,20,2,1,100).accepted);
 for(unsigned skill:{209u,2803u}){
  const auto hex=mind_freeze_hex(skill,15);check(hex&&valid_movement_hex(*hex));
  MovementEffects movement;movement.apply({1,1,skill,15,Category::hex,100,5100});
  check(movement.at(99)==1&&std::abs(movement.at(100)-.1f)<1e-6);
  movement.apply({2,1,210,15,Category::hex,100,6000});
  check(std::abs(movement.at(100)-.1f)<1e-6); // Combined snares cannot exceed the strongest exception.
  movement.apply({3,1,1043,0,Category::stance,100,3100});
  check(std::abs(movement.at(100)-.1f)<1e-6);
  movement.remove(1,1000);check(std::abs(movement.at(1000)-.51f)<1e-6);
  check(std::abs(movement.at(5100)-.34f)<1e-6&&movement.at(6000)==1);
  MovementEffects haste;haste.apply({1,1,skill,15,Category::hex,100,5100});haste.apply({2,1,1043,0,Category::stance,100,3100});
  check(std::abs(haste.at(100)-.15f)<1e-6);
 }
 std::cout<<"PASS Mind Freeze PvE/PvP native plans,strict fractional Energy threshold,separate hits,armor,protection and nonlethal snare\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
