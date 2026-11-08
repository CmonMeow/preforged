#include "gw_overcast_activation.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Overcast activation line "+std::to_string(at.line()));
}
int main()try{
 const skills::effects::Scope owner{1,42,7};overcast::CeilingCurve curve(owner,1000,33);energy::Regeneration regen;
 vitals::Resources full{100,100,30,30};
 const auto shock=overcast::prepare_activation(curve,owner,full,regen,0,5,10,1,1000,0);
 check(shock&&shock->state.resources.energy==20&&shock->state.resources.max_energy==30&&shock->ordinary_paid==5&&shock->ceiling_loss_units==5*overcast::energy_unit);
 check(full.energy==30&&!curve.active()&&regen.remainder()==0);
 auto low=full;low.energy=12;const auto below=overcast::prepare_activation(curve,owner,low,regen,0,5,10,1,1000,0);
 check(below&&below->state.resources.energy==7&&below->ceiling_loss_units==0);
 auto small=full;small.max_energy=small.energy=10;
 const auto negative=overcast::prepare_activation(curve,owner,small,regen,0,5,30,1,1000,0);
 check(negative&&negative->state.debt==20&&negative->state.resources.energy==0&&negative->state.curve.integer_ceiling(10)==-20);
 check(!overcast::prepare_activation(negative->state.curve,owner,negative->state.resources,negative->state.regeneration,negative->state.debt,1,3,2,1000,0));
 const auto zero=overcast::prepare_activation(negative->state.curve,owner,negative->state.resources,negative->state.regeneration,negative->state.debt,0,3,2,1000,0);
 check(zero&&zero->state.debt==23); // Arithmetic only; skill eligibility remains separate.
 check(!overcast::prepare_activation(shock->state.curve,owner,shock->state.resources,shock->state.regeneration,0,5,10,1,1000,0));
 auto stale=owner;++stale.generation;check(!overcast::prepare_activation(curve,stale,full,regen,0,5,10,1,1000,0));
 auto dead=full;dead.health=0;check(!overcast::prepare_activation(curve,owner,dead,regen,0,5,10,1,1000,0));
 check(!overcast::prepare_activation(curve,owner,full,regen,0,31,10,1,1000,0));
 check(!overcast::prepare_activation(curve,owner,full,regen,0,5,UINT64_MAX,1,1000,0));
 // Preserve fractional current and ceiling when applying another cost.
 overcast::CeilingCurve fractional(owner,1000,33);check(fractional.apply(owner,10,1,1000));auto r=full;r.max_energy=50;r.energy=40;
 const auto next=overcast::prepare_activation(fractional,owner,r,regen,0,5,5,2,1500,4);
 check(next&&overcast::precise_current(next->state)==35*overcast::energy_unit+49500&&next->ceiling_loss_units==0);
 energy::Regeneration almost;almost.restore_fraction(2999);r.energy=4;
 check(!overcast::prepare_activation(fractional,owner,r,almost,0,5,3,2,1000,0,99));
 check(almost.remainder()==2999&&fractional.cursor()==1000&&r.energy==4);
 r.energy=5;const auto precise=overcast::prepare_activation(fractional,owner,r,regen,0,5,3,2,1000,0,1);
 check(precise&&precise->state.resources.energy==0&&precise->state.debt==0&&precise->state.fractional_tail==1&&overcast::precise_current(precise->state)==1);
 {
  // Regeneration can cross zero and then recover before a spell activation.
  // Paying a new cast must wait for that reaction, before adding any burden.
  overcast::CeilingCurve changing(owner,1000,33);
  check(changing.apply_energy_modifier(owner,10,125,-5,6000,1000));
  auto one=full;one.energy=1;
  for(unsigned at:{4000u,10000u})
   check(!overcast::prepare_activation(changing,owner,one,regen,0,0,10,77,at,4));
  const auto boundary=overcast::settle_until_zero(changing,owner,one,regen,0,10000,4);
  check(boundary&&boundary->first_zero==4000);
  auto handled=boundary->curve;check(handled.remove_energy_modifier(owner,10,4000));
  const auto retry=overcast::prepare_activation(handled,owner,boundary->resources,boundary->regeneration,
      boundary->debt,5,10,77,10000,4,boundary->fractional_tail);
  check(retry&&retry->ordinary_paid==5&&retry->state.resources.energy==3&&
      retry->state.curve.burden_units()==1000000);
  // A rejected attempt did not burn the activation identity or charge burden.
  check(!changing.active()&&changing.cursor()==1000&&one.energy==1);
 }
 std::cout<<"PASS atomic Overcast activation payment/clamp, signed debt, fractional current and failed-cost replay/scope/overflow rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
