#include "gw_overcast_settlement.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy degeneration settlement line "+std::to_string(at.line()));
}
int main()try{
 const skills::effects::Scope owner{1,42,7};overcast::CeilingCurve curve(owner,1000,33);
 check(curve.apply_energy_modifier(owner,1,125,-1,6000,1000));
 check(!curve.apply_energy_modifier(owner,2,140,-11,6000,1000));
 check(!curve.apply_energy_modifier(owner,2,140,UINT32_MAX,6000,1000));
 energy::Regeneration regen;vitals::Resources resources{100,100,10,50};
 const auto loss=overcast::settle(curve,owner,resources,regen,0,4000,0);
 check(loss&&loss->resources.energy==9&&loss->lost_units==overcast::energy_unit&&!loss->first_zero&&!loss->gained_units);
 const auto expiry=overcast::settle(curve,owner,resources,regen,0,10000,0);
 check(expiry&&expiry->resources.energy==8&&expiry->lost_units==2*overcast::energy_unit&&!expiry->first_zero);
 const auto positive=overcast::settle(curve,owner,resources,regen,0,4000,4);
 check(positive&&positive->resources.energy==13&&!positive->lost_units);
 auto mixed=curve;check(mixed.apply_energy_modifier(owner,2,119,3,6000,1000));
 check(mixed.effective_energy_pips(0,1000)==2);
 const auto mixed_gain=overcast::settle(mixed,owner,resources,regen,0,4000,0);
 check(mixed_gain&&mixed_gain->resources.energy==12);
 {overcast::CeilingCurve reversal(owner,1000,33);check(reversal.apply_energy_modifier(owner,1,125,-5,6000,1000));
  auto one=resources;one.energy=1;const auto proposed=overcast::settle(reversal,owner,one,regen,0,10000,4);
  check(proposed&&proposed->first_zero==4000&&proposed->resources.energy==4&&proposed->lost_units==overcast::energy_unit&&proposed->gained_units==4*overcast::energy_unit);
  // A zero-terminating hex must discard/recompute this proposal from4000,
  // not accept the later expiry-based gain as if the hex stayed installed.
  const auto stopped=overcast::settle_until_zero(reversal,owner,one,regen,0,10000,4);
  check(stopped&&stopped->curve.cursor()==4000&&stopped->first_zero==4000&&
      overcast::precise_current(*stopped)==0&&stopped->gained_units==0&&stopped->lost_units==overcast::energy_unit);
  // Neither a same-time mutation nor a later recovery may hide the pending
  // zero event. Failed proposals retain the caller's original authorities.
  for(unsigned at:{4000u,10000u}){
   check(!overcast::prepare_gain(reversal,owner,one,regen,0,10,at,4));
   check(!overcast::prepare_loss(reversal,owner,one,regen,0,0,at,4,overcast::LossRule::full_cost));
   check(!overcast::prepare_loss(reversal,owner,one,regen,0,10,at,4,overcast::LossRule::drain_to_zero));
   check(!overcast::prepare_resize(reversal,owner,one,regen,0,60,at,4));
  }
  check(overcast::prepare_gain(reversal,owner,one,regen,0,1,3999,4).has_value());
  auto ended=stopped->curve;check(ended.remove_energy_modifier(owner,1,4000));
  const auto resumed=overcast::settle_until_zero(ended,owner,stopped->resources,stopped->regeneration,
      stopped->debt,10000,4,stopped->fractional_tail);
  check(resumed&&resumed->curve.cursor()==10000&&!resumed->first_zero&&resumed->resources.energy==8);
  // Once the boundary is handled and its modifier removed, retry succeeds
  // without discarding the recovery earned after4000.
  const auto rewarded=overcast::prepare_gain(ended,owner,stopped->resources,stopped->regeneration,
      stopped->debt,10,10000,4,stopped->fractional_tail);
  check(rewarded&&rewarded->state.resources.energy==18);
  const auto spent=overcast::prepare_loss(ended,owner,stopped->resources,stopped->regeneration,
      stopped->debt,3,10000,4,overcast::LossRule::full_cost,stopped->fractional_tail);
  check(spent&&spent->state.resources.energy==5&&spent->lost_units==3*overcast::energy_unit);
  const auto drained=overcast::prepare_loss(ended,owner,stopped->resources,stopped->regeneration,
      stopped->debt,20,10000,4,overcast::LossRule::drain_to_zero,stopped->fractional_tail);
  check(drained&&drained->state.resources.energy==0&&drained->lost_units==8*overcast::energy_unit);
  const auto resized=overcast::prepare_resize(ended,owner,stopped->resources,stopped->regeneration,
      stopped->debt,60,10000,4,stopped->fractional_tail);
  check(resized&&resized->state.resources.energy==18&&resized->state.resources.max_energy==60);
  check(one.energy==1&&reversal.cursor()==1000);
  check(reversal.cursor()==1000&&reversal.has_energy_modifier(owner,1));
 }
 auto caps=curve;check(caps.apply_energy_modifier(owner,2,119,10,6000,1000)&&caps.apply_energy_modifier(owner,3,157,10,6000,1000));
 check(caps.effective_energy_pips(0,1000)==10); // Sum before applying the cap.
 auto negative=curve;check(negative.apply_energy_modifier(owner,2,140,-10,6000,1000));
 check(negative.effective_energy_pips(0,1000)==-10);
 auto partial=regen;partial.restore_fraction(1);resources.energy=1;
 const auto before=overcast::settle(curve,owner,resources,partial,0,4001,0,1);
 check(before&&overcast::precise_current(*before)==1&&!before->first_zero);
 const auto zero=overcast::settle(curve,owner,resources,partial,0,4002,0,1);
 check(zero&&overcast::precise_current(*zero)==0&&zero->first_zero==4002&&zero->lost_units==300101);
 check(zero->first_zero_before_units==1);
 const auto stopped_fraction=overcast::settle_until_zero(curve,owner,resources,partial,0,10000,0,1);
 check(stopped_fraction&&stopped_fraction->curve.cursor()==4002&&stopped_fraction->lost_units==300101&&
     stopped_fraction->fractional_tail==0&&stopped_fraction->regeneration.remainder()==0);
 check(stopped_fraction->first_zero_before_units==1);
 {
  // The whole interval begins at zero, gains Energy, then degenerates after
  // the positive modifier expires. Preserve the actual pre-crossing value.
  overcast::CeilingCurve changing(owner,1000,33);
  check(changing.apply_energy_modifier(owner,10,125,-1,20000,1000));
  check(changing.apply_energy_modifier(owner,11,119,4,3000,1000));
  auto empty=resources;empty.energy=0;
  const auto crossed=overcast::settle_until_zero(changing,owner,empty,regen,0,15000,0);
  check(crossed&&crossed->first_zero==13000&&crossed->curve.cursor()==13000&&
      crossed->first_zero_before_units==100&&overcast::precise_current(*crossed)==0);
  auto c=changing;auto r=empty;auto f=regen;std::optional<std::uint64_t> stamp,prezero;
  for(unsigned at=1100;at<=15000;at+=100){
   const auto step=overcast::settle_until_zero(c,owner,r,f,0,at,0);check(bool(step));
   if(step->first_zero){stamp=step->first_zero;prezero=step->first_zero_before_units;break;}
   c=step->curve;r=step->resources;f=step->regeneration;
  }
  check(stamp==crossed->first_zero&&prezero==crossed->first_zero_before_units);
 }
 const auto late=overcast::settle(curve,owner,resources,partial,0,10000,0,1);
 check(late&&late->first_zero==4002&&late->lost_units==300101&&overcast::precise_current(*late)==0);
 // Sparse versus dense proposals must retain fractional tails and the same
 // first crossing. The owned hex must handle this crossing before full commit.
 auto clock=curve;auto current=resources;auto fraction=partial;unsigned tail=1;std::uint32_t debt=0;
 std::optional<std::uint64_t> first;std::uint64_t lost=0;
 for(unsigned i=1;i<=90;++i){const auto next=overcast::settle(clock,owner,current,fraction,debt,1000+i*100,0,tail);check(bool(next));
  if(next->first_zero&&!first)first=next->first_zero;lost+=next->lost_units;
  clock=next->curve;current=next->resources;fraction=next->regeneration;debt=next->debt;tail=next->fractional_tail;}
 check(first==late->first_zero&&lost==late->lost_units&&current.energy==late->resources.energy&&fraction.remainder()==late->regeneration.remainder()&&tail==late->fractional_tail);
 resources.energy=0;partial.restore_fraction(1500);
 const auto owing=overcast::settle(curve,owner,resources,partial,3,10000,0,1);
 check(owing&&overcast::precise_current(*owing)==-3*overcast::energy_unit+150001&&!owing->lost_units&&!owing->first_zero);
 resources.energy=1;resources.health=0;
 const auto corpse=overcast::settle(curve,owner,resources,regen,0,10000,0);
 check(corpse&&corpse->resources.energy==1&&!corpse->lost_units&&!corpse->first_zero);
 resources.health=100;
 const auto huge=overcast::settle_constant(curve,owner,resources,regen,0,UINT64_MAX,-10);
 check(huge&&huge->first_zero==1300&&huge->lost_units==overcast::energy_unit&&overcast::precise_current(*huge)==0);
 auto stale=owner;++stale.generation;check(!overcast::settle(curve,stale,resources,regen,0,4000,0));
 check(curve.cursor()==1000&&regen.remainder()==0); // Proposals never mutate their inputs.
 std::cout<<"PASS signed Energy modifier stacking/caps,expiry,fractional zero timestamp,sparse/dense/debt/death/overflow and immutable proposals;owned hex zero-event commit pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
