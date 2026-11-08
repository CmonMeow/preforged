#include "gw_warriors_endurance.h"
#include "gw_overcast_settlement.h"
#include <source_location>
#include <iostream>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Endurance gain line "+std::to_string(at.line()));}
int main()try{
 const skills::effects::Scope owner{1,2,3};overcast::CeilingCurve clear(owner,1000,33);
 for(unsigned id:{374u,3002u})for(unsigned rank=0;rank<=20;++rank){const auto p=skills::effects::warriors_endurance(id,rank);check(p&&p->energy==3&&p->ceiling==10+rank&&p->duration==1000ull*(5+2*rank)&&p->stance==(id==3002));}
 for(unsigned current:{0u,19u,21u,22u,24u})for(unsigned fraction:{0u,1u,1500u,2999u})for(unsigned tail:{0u,99u}){
 vitals::Resources r{100,100,current,50};energy::Regeneration regen;regen.restore_fraction(fraction);
 const auto before=std::int64_t(current)*overcast::energy_unit+fraction*100+tail;
 const auto p=overcast::prepare_gain(clear,owner,r,regen,0,3,1000,0,tail,22);
 const auto expected=std::min<std::int64_t>(3*overcast::energy_unit,std::max<std::int64_t>(0,22*overcast::energy_unit-before));
 check(p&&p->gained_units==expected&&overcast::precise_current(p->state)==before+expected);
 check(r.energy==current&&regen.remainder()==fraction&&clear.cursor()==1000);
 }
 {vitals::Resources r{100,100,0,50};energy::Regeneration regen;regen.restore_fraction(2250);const auto p=overcast::prepare_gain(clear,owner,r,regen,8,3,1000,0,0,22);check(p&&p->state.debt==5&&p->state.resources.energy==0&&p->state.regeneration.remainder()==2250&&p->debt_repaid_units==3*overcast::energy_unit);}
 {auto curve=clear;check(curve.apply(owner,40,1,1000));vitals::Resources r{100,100,9,50};energy::Regeneration regen;regen.restore_fraction(1500);const auto p=overcast::prepare_gain(curve,owner,r,regen,0,3,1000,0,0,22);check(p&&overcast::precise_current(p->state)==10*overcast::energy_unit&&p->gained_units==overcast::energy_unit/2);}
 for(int pips:{1,4}){vitals::Resources r{100,100,21,50};energy::Regeneration regen;const auto p=overcast::prepare_gain(clear,owner,r,regen,0,3,4000,pips,0,22);check(p&&p->state.resources.energy==unsigned(21+pips)&&!p->gained_units);check(!overcast::prepare_gain(clear,owner,r,regen,0,3,1000,0,0,1000001));}
 std::cout<<"PASS Endurance native plans and precise capped gain,above-cap preservation,fractions/debt/Overcast/regeneration settlement;World/Runtime unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
