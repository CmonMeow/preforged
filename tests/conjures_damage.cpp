#include "gw_conjures.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool x){if(!x)throw std::runtime_error("Conjure damage regression");}
int main(){using namespace skills::effects;using combat::Kind;
 const Scope owner{1,1,1};
 for(unsigned skill:{182u,207u,221u})for(unsigned rank:{0u,12u,15u,20u}){
  Registry registry(owner);check(registry.apply(owner,{1,1,skill,rank,Category::enchantment,100,60100},Replace::same_skill).accepted);
  const auto plan=*conjure(skill,rank);
  for(unsigned level:{1u,20u,80u,255u})for(int armor:{-100,60,160,500}){
   combat::Actor target{2,7,combat::Health{500,500},armor,0,true};target.fire_armor=armor;target.cold_armor=armor;target.lightning_armor=armor;
   const auto proposal=prepare_conjure_hit(registry,owner,plan.element,true,target,level,true,100);
   check(proposal&&proposal->hit.kind==plan.element&&proposal->hit.skill==skill&&proposal->hit.damage==5+rank&&proposal->actor.health->current==500-5-rank);
   check(target.health->current==500&&!combat::damage_packets_2022(1,proposal->hit).empty());
  }
  combat::Actor unknown{2,7,combat::Health{500,500},{},0,true};
  check(prepare_conjure_hit(registry,owner,plan.element,true,unknown,20,true,100).has_value());
  unknown.elemental_protection=plan.element;unknown.elemental_multiplier=.5;unknown.incoming_damage_multiplier=2;unknown.incoming_reduction=3;
  const auto protected_hit=prepare_conjure_hit(registry,owner,plan.element,true,unknown,20,true,100);
  check(protected_hit&&protected_hit->hit.damage==plan.damage-3);
  check(!prepare_conjure_hit(registry,owner,plan.element,false,unknown,20,true,100));
  check(!prepare_conjure_hit(registry,owner,plan.element,true,unknown,20,false,100));
  check(!prepare_conjure_hit(registry,owner,plan.element,true,unknown,0,true,100));
  check(!prepare_conjure_hit(registry,owner,plan.element,true,unknown,20,true,60100));
  unknown.health=combat::Health{1,500};const auto lethal=prepare_conjure_hit(registry,owner,plan.element,true,unknown,20,true,100);
  check(lethal&&lethal->hit.killed&&lethal->hit.applied==1&&lethal->actor.health->current==0);
  unknown.health=combat::Health{0,500};check(!prepare_conjure_hit(registry,owner,plan.element,true,unknown,20,true,100));
 }
 check(!combat::valid(combat::Damage{Kind::fire,17,20,182,10,true}));
 // Existing armor-respecting and untyped armor-ignoring paths retain behavior.
 check(combat::amount({Kind::fire,100,20,182},100)==50);
 check(combat::amount({Kind::armor_ignoring,17,20,182},{})==17);
 std::cout<<"PASS Conjure typed armor bypass,all elements/ranks/levels,unknown armor,elemental defenses,flat reduction,lethal clamp and immutable proposals;World pending\n";
}
