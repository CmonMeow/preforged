#include "gw_overcast_settlement.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Overcast resize line "+std::to_string(at.line()));
}
int main()try{
 const skills::effects::Scope owner{1,42,7};overcast::CeilingCurve curve(owner,1000,33);energy::Regeneration regen;
 check(curve.apply(owner,10,1,1000));vitals::Resources r{100,100,20,50};
 const auto down=overcast::prepare_resize(curve,owner,r,regen,0,30,1000,0);
 check(down&&down->state.resources.energy==0&&down->state.debt==0&&down->state.curve.burden_units()==curve.burden_units()&&down->state.curve.integer_ceiling(30)==20);
 const auto up=overcast::prepare_resize(down->state.curve,owner,down->state.resources,down->state.regeneration,down->state.debt,50,1000,0,down->state.fractional_tail);
 check(up&&up->state.resources.energy==20&&up->state.resources.max_energy==50);
 r.energy=40;const auto fractional=overcast::prepare_resize(curve,owner,r,regen,0,30,1500,4);
 check(fractional&&overcast::precise_current(fractional->state)==20*overcast::energy_unit+49500&&fractional->state.curve.burden_units()==983500);
 overcast::CeilingCurve negative(owner,1000,33);check(negative.apply(owner,60,1,1000));r.energy=0;
 const auto debt=overcast::prepare_resize(negative,owner,r,regen,30,30,1000,0);
 check(debt&&debt->state.debt==50&&debt->state.curve.integer_ceiling(30)==-30);
 for(unsigned nominal:{10u,30u,50u})for(int current=-30;current<=int(nominal);++current)for(unsigned next:{10u,30u,50u}){
  overcast::CeilingCurve clear(owner,1000,33);vitals::Resources original{100,100,unsigned(std::max(0,current)),nominal};auto expected=original;std::uint32_t owing=unsigned(std::max(0,-current)),expected_debt=owing;
  energy::resize(expected,next,expected_debt);const auto proposal=overcast::prepare_resize(clear,owner,original,regen,owing,next,1000,0);
  check(proposal&&proposal->state.resources.energy==expected.energy&&proposal->state.debt==expected_debt&&proposal->state.resources.max_energy==next);
 }
 check(!overcast::prepare_resize(curve,owner,r,regen,0,0,1000,0)&&!overcast::prepare_resize(curve,owner,r,regen,0,1000001,1000,0));
 auto stale=owner;++stale.generation;check(!overcast::prepare_resize(curve,stale,r,regen,0,30,1000,0));
 check(curve.cursor()==1000&&r.max_energy==50);
 std::cout<<"PASS fractional nominal-capacity changes, preserved Overcast burden, signed debt and legacy resize equivalence\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
