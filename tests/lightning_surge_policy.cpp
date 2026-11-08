#include "gw_lightning_surge.h"
#include "gw_lightning_strike.h"
#include <iostream>
#include <source_location>
#include <string>
#include <type_traits>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Lightning Surge policy line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills::effects;using combat::conditions::Kind;
 static_assert(!std::is_same_v<LightningSurgeTimers,LightningStrikeTimers>);
 static_assert(!std::is_same_v<LightningSurgeTimers::Link,LightningStrikeTimers::Link>);
 const combat::Actor base{2,1,combat::Health{1000,1000},{},0,true,80};
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=lightning_surge(rank);check(plan&&plan->duration==3000&&plan->penetration==25&&plan->knockdown_ms==2000);
  const auto r=lightning_surge_impact(*plan,base,{},20,1,4000);
  check(r.accepted&&r.knockdown_eligible&&r.hit.applied==damage_scale(14,100,rank));
  check(r.conditions.has(Kind::cracked_armor,4000+plan->condition_ms-1)&&!r.conditions.has(Kind::cracked_armor,4000+plan->condition_ms));
 }
 const auto plan=*lightning_surge(12);check(std::lround(plan.damage)==83&&plan.condition_ms==17000&&!lightning_surge(21));
 for(unsigned hp:{1u,82u,83u,84u}){
  auto actor=base;actor.health->current=hp;const auto r=lightning_surge_impact(plan,actor,{},20,1,4000);
  check(r.accepted&&r.hit.killed==(hp<=plan.damage)&&r.knockdown_eligible==(hp>plan.damage)&&r.conditions.has(Kind::cracked_armor,4000)==(hp>plan.damage));
 }
 {
  const auto r=lightning_surge_impact(plan,base,{},20,1,4000,false);
  check(r.accepted&&r.hit.applied==plan.damage&&r.knockdown_eligible&&!r.conditions.count(4000));
  auto conditions=combat::conditions::State{};check(conditions.apply(Kind::cracked_armor,9,1000,2999));
  const auto expired=lightning_surge_impact(plan,base,conditions,20,1,4000);check(expired.accepted&&expired.hit.applied==plan.damage);
  check(conditions.apply(Kind::cracked_armor,9,1000,5000));
  const auto active=lightning_surge_impact(plan,base,conditions,20,1,4000);
  check(active.accepted&&std::abs(active.hit.applied-plan.damage*std::exp2(15./40.))<1e-7);
 }
 using T=LightningSurgeTimers;const Scope a{1,1,1},b{1,2,1},c{1,3,1};const T::Link first{a,b,1,12,20,1000,4000};
 {
  T t(1);check(t.add(first)&&t.advance(3999).empty());
  check(!t.cancel(b,1,4000));check(t.advance(10000)==std::vector<T::Link>{first}&&t.advance(10000).empty());
 }
 {
  T t(1);check(t.add(first)&&t.cancel(b,1,3999)&&t.advance(9000).empty());check(!t.add(first));
 }
 {
  T t(1);check(t.add(first));auto renewed=first;renewed.caster=c;renewed.effect=2;renewed.applied=2000;renewed.expires=5000;
  check(t.replace(b,1,renewed,2000)&&t.advance(4000).empty()&&t.advance(5000)==std::vector<T::Link>{renewed});
 }
 for(unsigned mode=0;mode<4;++mode){
  T t(1);auto bad=first;if(mode==0)bad.target.generation=0;if(mode==1)bad.caster.instance=2;if(mode==2)bad.rank=21;if(mode==3)bad.expires=3999;
  check(!t.add(bad)&&t.add(first));t.discard(mode%2?a:b);check(t.advance(9000).empty());
 }
 auto unknown=base;unknown.lightning_armor.reset();check(!lightning_surge_impact(plan,unknown,{},20,1,4000).accepted);
 auto forged=plan;forged.duration=2000;check(!lightning_surge_impact(forged,base,{},20,1,4000).accepted);
 check(!lightning_surge_impact(plan,base,{},0,1,4000).accepted);
 std::cout<<"PASS prepared Lightning Surge expiry/armor/condition policy and distinct scoped natural-hex timer lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
