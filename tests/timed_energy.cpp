#include "gw_overcast_settlement.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Timed Energy line "+std::to_string(at.line()));
}
int main()try{
 const skills::effects::Scope owner{1,1,1};energy::Regeneration regen;vitals::Resources r{100,100,0,100};
 overcast::CeilingCurve curve(owner,1000,33);
 check(curve.apply_energy_modifier(owner,1,119,5,10000,1000));
 check(!curve.apply_energy_modifier(owner,0,119,5,1000,1000));
 check(!curve.apply_energy_modifier(owner,2,0,5,1000,1000));
 check(!curve.apply_energy_modifier(owner,2,119,11,1000,1000));
 overcast::CeilingCurve overflow(owner,UINT64_MAX,33);check(!overflow.apply_energy_modifier(owner,1,119,5,1,UINT64_MAX));
 check(curve.effective_energy_pips(2,10999)==7&&curve.effective_energy_pips(2,11000)==2);
 check(!curve.apply_energy_modifier({1,1,2},2,119,5,1000,1000));
 check(!curve.apply_energy_modifier(owner,2,119,5,1000,1001));
 check(!curve.apply_energy_modifier(owner,1,118,5,1000,1000));
 auto whole=overcast::settle(curve,owner,r,regen,0,16000,2);check(whole&&overcast::precise_current(*whole)==8000000);
 check(curve.cursor()==1000&&r.energy==0&&regen.remainder()==0);
 auto fine=overcast::settle(curve,owner,r,regen,0,1000,2);check(bool(fine));
 for(std::uint64_t at=1037;at<16000;at+=37)fine=overcast::settle(fine->curve,owner,fine->resources,fine->regeneration,fine->debt,at,2,fine->fractional_tail);
 fine=overcast::settle(fine->curve,owner,fine->resources,fine->regeneration,fine->debt,16000,2,fine->fractional_tail);
 check(fine&&overcast::precise_current(*fine)==overcast::precise_current(*whole));
 const auto loss=overcast::prepare_loss(curve,owner,r,regen,0,20,16000,2,overcast::LossRule::full_cost);
 check(loss&&overcast::precise_current(loss->state)==2000000);
 const auto gain=overcast::prepare_gain(curve,owner,r,regen,0,10,16000,2);
 check(gain&&overcast::precise_current(gain->state)==11000000);
 const auto resized=overcast::prepare_resize(curve,owner,r,regen,0,80,16000,2);
 check(resized&&overcast::precise_current(resized->state)==2000000);
 const auto debt=overcast::settle(curve,owner,r,regen,10,16000,2);check(debt&&overcast::precise_current(*debt)==5000000&&debt->debt==0);
 auto changed=overcast::settle(curve,owner,r,regen,0,5000,2);check(bool(changed));
 check(!changed->curve.remove_energy_modifier(owner,1,4999));
 auto renewed=*changed;check(renewed.curve.apply_energy_modifier(owner,2,119,3,6000,5000));
 const auto replaced=overcast::settle(renewed.curve,owner,renewed.resources,renewed.regeneration,renewed.debt,16000,2,renewed.fractional_tail);
 check(replaced&&overcast::precise_current(*replaced)==6800000);
 check(changed->curve.remove_energy_modifier(owner,1,5000));
 const auto removed=overcast::settle(changed->curve,owner,changed->resources,changed->regeneration,changed->debt,16000,2,changed->fractional_tail);
 check(removed&&overcast::precise_current(*removed)==5000000);
 auto stacked=curve;check(stacked.apply_energy_modifier(owner,2,118,6,10000,1000));
 const auto cap=overcast::settle(stacked,owner,r,regen,0,16000,2);check(cap&&overcast::precise_current(*cap)==11000000);
 overcast::CeilingCurve burden(owner,1000,33);check(burden.apply(owner,10,1,1000)&&burden.apply_energy_modifier(owner,1,119,5,10000,1000));
 r.max_energy=20;const auto bounded=overcast::settle(burden,owner,r,regen,0,16000,2);
 check(bounded&&overcast::precise_current(*bounded)==4485000&&bounded->curve.burden_units()==505000);
 r.health=0;const auto dead=overcast::settle(burden,owner,r,regen,0,16000,2);check(dead&&overcast::precise_current(*dead)==0&&dead->curve.burden_units()==505000);
 check(curve.retire(owner)&&!curve.apply_energy_modifier(owner,3,119,5,1000,1000)&&curve.effective_energy_pips(2,1000)==2);
 std::cout<<"PASS timed positive Energy modifiers,exact expiry,whole/fine ticks,debt,fractions,Overcast,all resource proposals,replacement/removal,cap and lifetimes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
