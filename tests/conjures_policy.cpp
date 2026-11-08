#include "gw_conjures.h"
#include <iostream>
#include <stdexcept>
void check(bool x){if(!x)throw std::runtime_error("Conjure selection regression");}
int main(){using namespace fw::gw;using namespace skills::effects;
 const Scope owner{1,42,3};
 for(unsigned rank=0;rank<=20;++rank){Registry registry(owner);
  for(unsigned skill:{182u,207u,221u}){const auto p=conjure(skill,rank);check(p&&p->damage==5+rank&&p->duration==60000);
   check(registry.apply(owner,{skill,42,skill,rank,Category::enchantment,100,60100},Replace::same_skill).accepted);}
  for(auto element:{combat::Kind::fire,combat::Kind::cold,combat::Kind::lightning}){
   const auto result=conjure_bonus(registry,owner,element,true,100);check(result&&result->plan.element==element&&result->plan.damage==5+rank);
   check(!conjure_bonus(registry,owner,element,false,100)&&!conjure_bonus(registry,owner,element,true,99)&&!conjure_bonus(registry,owner,element,true,60100));
   // A multi-hit attack must receive a fresh bonus per landed hit;selection
   // cannot consume the enchantment or replay an earlier weapon element.
   check(conjure_bonus(registry,owner,element,true,101)->cause.instance==result->cause.instance);
  }
  check(registry.count(owner,Category::enchantment,200)==3);
  check(!conjure_bonus(registry,owner,{},true,100)&&!conjure_bonus(registry,owner,combat::Kind::earth,true,100)&&!conjure_bonus(registry,owner,combat::Kind::armor_ignoring,true,100));
  auto stale=owner;++stale.generation;check(!conjure_bonus(registry,stale,combat::Kind::fire,true,100));
  check(registry.remove(owner,182).has_value()&&!conjure_bonus(registry,owner,combat::Kind::fire,true,100));
 }
 check(!conjure(182,21)&&!conjure(183,12));
 std::cout<<"PASS three Conjures ranks0..20,element switching,per-hit nonconsumption,expiration,removal and full owner lifetime;World/casting pending\n";
}
