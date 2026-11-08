#include "gw_overcast.h"
#include <iostream>

int main()try{
    using fw::gw::overcast::Pool;
    auto check=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
    Pool shock(30,30);check(shock.start(5,10),"Shock activation rejected");
    check(shock.current()==20&&shock.maximum()==20,"wiki Shock example/order");
    // Cancellation/interrupt have no resource rollback API.
    check(shock.points()==10&&shock.active(),"activation cost missing");
    Pool low(30,12);low.apply(10);check(low.current()==12,"Overcast subtracts current unconditionally");
    low.recover(1);check(low.current()==12&&low.maximum()==21,"recovery grants current energy");
    low.apply(30);check(low.maximum()==-9&&low.current()==-9,"negative maximum was capped");
    check(low.visible_current()==0&&!low.can_pay(1),"negative current pays positive cost");
    check(low.pay(0)&&low.current()==-9,"zero payment changed current");
    low.recover(10);check(low.maximum()==1&&low.current()==-9,"recovery erased negative current");
    low.gain(5);check(low.current()==-4,"gain did not repay debt");
    low.gain(100);check(low.current()==1,"gain exceeded Overcast ceiling");
    Pool gear(30,5);gear.apply(10);gear.resize_nominal(10);
    check(gear.current()==-15&&gear.maximum()==0&&gear.points()==10,"equipment delta conflated with Overcast");
    gear.resize_nominal(30);check(gear.current()==5&&gear.maximum()==20,"equipment restore lost debt");
    check(!gear.start(6,10)&&gear.current()==5&&gear.points()==10,"failed activation changed costs");
    check(gear.recover(UINT64_MAX)==10&&!gear.active(),"unbounded recovery overflow");
    check(gear.current()==5&&gear.maximum()==30,"recovery refilled energy");
    gear.apply(10);gear.zone();check(!gear.active()&&gear.maximum()==30&&gear.current()==5,"zone reset");
    Pool debt(30,-20);check(!debt.active()&&debt.visible_current()==0,"negative energy implies Overcast");
    check(debt.start(0,0)&&debt.current()==-20&&!debt.active(),"negative energy blocked cost-free activation");
    check(!debt.start(1,0)&&debt.current()==-20,"negative energy paid positive activation cost");
    bool rejected=false;try{debt.apply(-1);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected&&!debt.active()&&debt.current()==-20,"invalid input mutates pool");
    check(Pool::recovery_interval_ms==3000,"recovery interval");
    using fw::gw::overcast::RecoveryClock;
    RecoveryClock fine(700),batch(700);std::uint64_t events=0;
    for(unsigned i=0;i<10000;++i)events+=fine.advance(1);
    check(events==batch.advance(10000)&&fine.remainder()==batch.remainder(),"tick cadence changed recovery events");
    check(events==3&&fine.remainder()==1700,"recovery lost fractional period");
    RecoveryClock fresh(0),offset(2999);
    check(fresh.advance(1)==0&&offset.advance(1)==1,"caller-selected phase ignored");
    offset.set_phase(100);check(offset.advance(2900)==1&&offset.remainder()==0,"explicit phase reset");
    RecoveryClock huge(2999);check(huge.advance(UINT64_MAX)==UINT64_MAX/3000+1,"elapsed overflow");
    Pool negative_ceiling(10,-30);negative_ceiling.apply(30);
    negative_ceiling.gain(100);check(negative_ceiling.current()==-20,"gain exceeded negative ceiling");
    auto before=negative_ceiling.current();bool overflow_rejected=false;
    try{negative_ceiling.apply(INT64_MAX);}catch(const std::overflow_error&){overflow_rejected=true;}
    check(overflow_rejected&&negative_ceiling.current()==before&&negative_ceiling.points()==30,"overflow partially committed");
    Pool corpse(30,10);corpse.apply(10);RecoveryClock death_phase(0);
    corpse.recover(death_phase.advance(30000));check(!corpse.active()&&corpse.current()==10,"dead recovery requires health or refills current");
    std::cout<<"Native signed Overcast resource tests passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
