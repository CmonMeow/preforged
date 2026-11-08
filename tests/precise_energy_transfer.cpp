#include "gw_overcast_settlement.h"
#include <iostream>
#include <source_location>
#include <string>
#include <array>
using namespace fw::gw;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Precise transfer line "+std::to_string(at.line()));}
int main()try{
 const skills::effects::Scope source{1,1,1},target{1,2,1};
 overcast::CeilingCurve source_curve(source,1000,33),target_curve(target,1000,33);
 energy::Regeneration target_regen;target_regen.restore_fraction(1500);
 const vitals::Resources foe{500,1000,2,30};
 const auto loss=overcast::prepare_loss(target_curve,target,foe,target_regen,0,4,1000,0,overcast::LossRule::drain_to_zero);
 check(loss&&loss->lost_units==750000&&overcast::precise_current(loss->state)==0);
 check(!overcast::prepare_loss(target_curve,target,foe,target_regen,0,4,1000,0,overcast::LossRule::full_cost));
 energy::Regeneration caster_regen;caster_regen.restore_fraction(750);
 const vitals::Resources caster{500,1000,7,30};
 // 2.5 Energy actually lost gives exactly10,not8 or12,at multiplier4.
 const auto gain=overcast::prepare_gain_units(source_curve,source,caster,caster_regen,0,loss->lost_units*4,1000,0);
 check(gain&&gain->gained_units==3000000&&overcast::precise_current(gain->state)==5175000);
 check(gain->state.resources.energy==17&&gain->state.regeneration.remainder()==750&&gain->state.fractional_tail==0);
 // Fractional negative Energy is repaid before positive gain; retain the
 // original one-unit tail rather than rounding the debt or reward.
 const auto debt=overcast::prepare_gain_units(source_curve,source,{500,1000,0,30},caster_regen,4,3000000,1000,0,1);
 check(debt&&debt->debt_repaid_units==1124999&&debt->positive_gain_units==1875001&&overcast::precise_current(debt->state)==1875001);
 check(debt->state.resources.energy==6&&debt->state.regeneration.remainder()==750&&debt->state.fractional_tail==1);
 const auto limited=overcast::prepare_gain_units(source_curve,source,caster,caster_regen,0,3000000,1000,0,0,10);
 check(limited&&limited->gained_units==825000&&overcast::precise_current(limited->state)==3000000);
 // Ordinary regeneration settles once before the resource event.
 const auto regen=overcast::prepare_gain_units(source_curve,source,caster,caster_regen,0,3000000,1003,4);
 check(regen&&overcast::precise_current(regen->state)==5176200&&regen->gained_units==3000000);
 auto burden=source_curve;check(burden.apply(source,10,1,1000));
 const auto ceiling=overcast::prepare_gain_units(burden,source,{500,1000,20,30},{},0,3000000,1001,0);
 check(ceiling&&ceiling->gained_units==99&&overcast::precise_current(ceiling->state)==6000099);
 const auto huge=overcast::prepare_gain_units(source_curve,source,caster,caster_regen,0,UINT64_MAX,1000,0);
 check(huge&&huge->gained_units==6825000&&overcast::precise_current(huge->state)==9000000);
 // Whole-point callers retain exactly the same proposal/projection path.
 struct WholeCase{unsigned amount;std::uint64_t gain,current;};
 for(const auto entry:std::array<WholeCase,5>{{{0,0,2175000},{1,300000,2475000},{4,1200000,3375000},{100,6825000,9000000},{UINT32_MAX,6825000,9000000}}}){
  const auto whole=overcast::prepare_gain(source_curve,source,caster,caster_regen,0,entry.amount,1000,0);
  check(whole&&whole->gained_units==entry.gain&&overcast::precise_current(whole->state)==entry.current);
 }
 overcast::Settlement small{source_curve,{500,1000,0,30},{},0,0};
 for(unsigned i=0;i<1000;++i){const auto next=overcast::prepare_gain_units(small.curve,source,small.resources,small.regeneration,small.debt,1,1000,0,small.fractional_tail);check(bool(next));small=next->state;}
 check(overcast::precise_current(small)==1000&&small.regeneration.remainder()==10&&small.fractional_tail==0);
 auto stale=source;++stale.generation;check(!overcast::prepare_gain_units(source_curve,stale,caster,caster_regen,0,1,1000,0));
 check(!overcast::prepare_gain_units(source_curve,source,caster,caster_regen,0,1,999,0));
 check(!overcast::prepare_gain_units(source_curve,source,{0,1000,7,30},caster_regen,0,1,1000,0));
 check(!overcast::prepare_gain_units(source_curve,source,caster,caster_regen,0,1,1000,0,100));
 std::cout<<"PASS precise Energy transfer2.5x4,debt/tails,caps/Overcast/regeneration,oversize safety and split rewards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
