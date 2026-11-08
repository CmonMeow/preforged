#include "gw_overcast_settlement.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Overcast settlement line "+std::to_string(at.line()));
}
int main()try{
 const skills::effects::Scope owner{1,42,7};
 for(unsigned burden:{0u,10u,60u})for(int pips:{-2,-1,0,1,4,10}){
  overcast::CeilingCurve curve(owner,1000,33);if(burden)check(curve.apply(owner,burden,1,1000));
  vitals::Resources initial{100,100,0,50};std::uint32_t debt=burden==60?30:0;energy::Regeneration regen;
  const auto sparse=overcast::settle(curve,owner,initial,regen,debt,25000,pips);check(bool(sparse));
  auto current=initial;auto clock=curve;auto regeneration=regen;auto owing=debt;unsigned tail=0;
  for(unsigned i=1;i<=240;++i){const auto next=overcast::settle(clock,owner,current,regeneration,owing,1000+i*100,pips,tail);check(bool(next));clock=next->curve;current=next->resources;regeneration=next->regeneration;owing=next->debt;tail=next->fractional_tail;}
  check(current.energy==sparse->resources.energy&&owing==sparse->debt&&clock.burden_units()==sparse->curve.burden_units()&&regeneration.remainder()==sparse->regeneration.remainder()&&tail==sparse->fractional_tail);
  check(curve.cursor()==1000&&initial.energy==0&&regen.remainder()==0);
 }
 {
  overcast::CeilingCurve signed_curve(owner,1000,33);energy::Regeneration signed_regen;
  vitals::Resources charged{100,100,20,20};
  const auto loss=overcast::settle(signed_curve,owner,charged,signed_regen,0,4000,-2);
  check(loss&&overcast::precise_current(*loss)==18*overcast::energy_unit&&loss->lost_units==2*overcast::energy_unit);
  const auto zero=overcast::settle(signed_curve,owner,charged,signed_regen,0,41000,-10);
  check(zero&&zero->first_zero&&overcast::precise_current(*zero)==0&&!zero->debt);
 }
 overcast::CeilingCurve curve(owner,1000,33);check(curve.apply(owner,10,1,1000));energy::Regeneration regen;vitals::Resources full{100,100,40,50};
 const auto opened=overcast::settle(curve,owner,full,regen,0,4031,4);check(opened&&opened->resources.energy==41&&opened->curve.integer_ceiling(50)==41&&opened->fractional_tail==69);
 const auto gained=overcast::settle(opened->curve,owner,opened->resources,opened->regeneration,0,4781,4,opened->fractional_tail);check(gained&&gained->resources.energy==41);
 auto corpse=full;corpse.health=0;const auto dead=overcast::settle(curve,owner,corpse,regen,0,25000,4);check(dead&&dead->resources.energy==40&&dead->curve.burden_units()<curve.burden_units()&&dead->gained_units==0);
 overcast::CeilingCurve clear(owner,1000,33);auto capped=full;capped.energy=50;
 const auto saturated=overcast::settle(clear,owner,capped,regen,0,25000,4);
 check(saturated&&saturated->resources.energy==50&&saturated->regeneration.remainder()==0&&saturated->fractional_tail==0&&saturated->gained_units==0);
 auto stale=owner;++stale.generation;check(!overcast::settle(curve,stale,full,regen,0,2000,4));
 check(!overcast::settle(curve,owner,full,regen,0,999,4));
 const auto long_span=overcast::settle(curve,owner,full,regen,0,UINT64_MAX,4);
 check(long_span&&long_span->resources.energy==50&&!long_span->curve.active()&&long_span->regeneration.remainder()==0&&long_span->fractional_tail==0);
 const auto no_regen=overcast::settle(curve,owner,full,regen,0,UINT64_MAX,0);
 check(no_regen&&no_regen->resources.energy==40&&!no_regen->curve.active());
 const auto dead_long=overcast::settle(curve,owner,corpse,regen,0,UINT64_MAX,4);
 check(dead_long&&dead_long->resources.energy==40&&!dead_long->curve.active());
 auto sparse_long=overcast::settle(curve,owner,full,regen,0,121000,4);check(bool(sparse_long));
 auto long_clock=curve;auto long_current=full;auto long_regen=regen;unsigned long_tail=0;std::uint32_t long_debt=0;
 for(unsigned i=1;i<=1200;++i){const auto step=overcast::settle(long_clock,owner,long_current,long_regen,long_debt,1000+i*100,4,long_tail);check(bool(step));long_clock=step->curve;long_current=step->resources;long_regen=step->regeneration;long_debt=step->debt;long_tail=step->fractional_tail;}
 check(long_current.energy==sparse_long->resources.energy&&long_debt==sparse_long->debt&&long_regen.remainder()==sparse_long->regeneration.remainder()&&long_tail==sparse_long->fractional_tail);
 full.energy=41;check(!overcast::settle(curve,owner,full,regen,0,2000,4));
 auto low=full;low.energy=20;
 const auto reward=overcast::prepare_gain(curve,owner,low,regen,0,100,1500,0);
 check(reward&&overcast::precise_current(reward->state)==40*overcast::energy_unit+49500&&reward->gained_units==20*overcast::energy_unit+49500&&reward->debt_repaid_units==0);
 overcast::CeilingCurve negative(owner,1000,33);check(negative.apply(owner,30,1,1000));low.max_energy=10;low.energy=0;
 const auto paid=overcast::prepare_gain(negative,owner,low,regen,30,100,1000,0);
 check(paid&&paid->state.debt==20&&paid->gained_units==10*overcast::energy_unit&&paid->debt_repaid_units==paid->gained_units&&paid->positive_gain_units==0);
 const auto fraction=overcast::prepare_gain(paid->state.curve,owner,paid->state.resources,paid->state.regeneration,paid->state.debt,1,1500,0,paid->state.fractional_tail);
 check(fraction&&overcast::precise_current(fraction->state)==-20*overcast::energy_unit+49500&&fraction->gained_units==49500&&fraction->state.debt==20);
 const auto first=overcast::prepare_gain(curve,owner,capped,regen,0,1,1000,0);check(!first); // Current exceeds original reduced ceiling.
 low.max_energy=50;low.energy=10;
 const auto one=overcast::prepare_gain(curve,owner,low,regen,0,1,1500,0);
 const auto two=overcast::prepare_gain(one->state.curve,owner,one->state.resources,one->state.regeneration,one->state.debt,2,1500,0,one->state.fractional_tail);
 const auto three=overcast::prepare_gain(curve,owner,low,regen,0,3,1500,0);
 check(two&&three&&overcast::precise_current(two->state)==overcast::precise_current(three->state));
 low.energy=0;energy::Regeneration partial;partial.restore_fraction(495);
 const auto drained=overcast::prepare_loss(curve,owner,low,partial,0,1,1000,0,overcast::LossRule::drain_to_zero);
 check(drained&&drained->lost_units==49500&&overcast::precise_current(drained->state)==0);
 check(!overcast::prepare_loss(curve,owner,low,partial,0,1,1000,0,overcast::LossRule::full_cost));
 low.max_energy=10;
 const auto no_drain=overcast::prepare_loss(negative,owner,low,partial,30,10,1000,0,overcast::LossRule::drain_to_zero);
 check(no_drain&&no_drain->lost_units==0&&overcast::precise_current(no_drain->state)==-30*overcast::energy_unit+49500);
 check(!overcast::prepare_loss(negative,owner,low,partial,30,1,1000,0,overcast::LossRule::full_cost));
 const auto zero_cost=overcast::prepare_loss(negative,owner,low,partial,30,0,1000,0,overcast::LossRule::full_cost);
 check(zero_cost&&zero_cost->lost_units==0&&overcast::precise_current(zero_cost->state)==overcast::precise_current(no_drain->state));
 {
  const skills::effects::Scope dead_owner{7,9,11};overcast::CeilingCurve dead_curve(dead_owner,1000,33);
  check(dead_curve.apply(dead_owner,10,1,1000));vitals::Resources dead{0,100,19,30};energy::Regeneration fraction;fraction.restore_fraction(123);
  const auto revived=overcast::prepare_resurrection(dead_curve,dead_owner,dead,fraction,0,30,1501,45);
  check(revived&&revived->curve.burden_units()==983467&&overcast::precise_current(*revived)==20*overcast::energy_unit+49599);
  const auto empty=overcast::prepare_resurrection(dead_curve,dead_owner,dead,fraction,0,0,1501,45);
  check(empty&&overcast::precise_current(*empty)==0&&empty->regeneration.remainder()==0&&empty->fractional_tail==0);
  auto stale=dead_owner;++stale.generation;
  check(!overcast::prepare_resurrection(dead_curve,stale,dead,fraction,0,30,1501,45));
  check(!overcast::prepare_resurrection(dead_curve,dead_owner,dead,fraction,0,31,1501,45));
  dead.health=1;check(!overcast::prepare_resurrection(dead_curve,dead_owner,dead,fraction,0,30,1501,45));
  check(dead_curve.cursor()==1000&&dead_curve.burden_units()==1000000&&fraction.remainder()==123);
 }
 std::cout<<"PASS changing-ceiling Energy settlement, sparse/dense equivalence, no banked full-cap regeneration, corpse recovery and immutable failure\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
