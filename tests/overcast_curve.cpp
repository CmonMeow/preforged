#include "gw_overcast_curve.h"
#include <iostream>
#include <string>
#include <cmath>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Overcast curve line "+std::to_string(at.line()));
}
int main()try{
 const skills::effects::Scope owner{1,42,7};overcast::CeilingCurve curve(owner,1000,33);
 check(curve.apply(owner,10,1,1000)&&curve.ceiling(50)==40&&curve.integer_ceiling(50)==40);
 check(curve.next_integer_change()==4031);
 check(curve.advance(owner,1500)&&std::abs(curve.ceiling(50)-40.165)<1e-9&&curve.integer_ceiling(50)==40);
 check(curve.advance(owner,2500)&&std::abs(curve.ceiling(50)-40.495)<1e-9);
 check(curve.next_integer_change()==4031);auto boundary=curve;
 check(boundary.advance(owner,4030)&&boundary.integer_ceiling(50)==40);
 check(boundary.advance(owner,4031)&&boundary.integer_ceiling(50)==41&&boundary.next_integer_change()==7061);
 // Fixed frozen157 observations:6.422s later, another10-Energy reduction.
 overcast::CeilingCurve observed(owner,18301671,33);check(observed.apply(owner,10,1,18301671)&&observed.apply(owner,10,2,18308093));
 check(std::abs(observed.ceiling(50)-.642484188079834*50)<.01);
 auto dense=curve,sparse=curve;for(unsigned i=1;i<=1000;++i)check(dense.advance(owner,2500+i));
 check(sparse.advance(owner,3500)&&dense.burden_units()==sparse.burden_units());
 const auto prior=curve.burden_units();auto stale=owner;++stale.generation;
 check(!curve.apply(stale,10,2,2500)&&!curve.apply(owner,10,1,2500)&&!curve.advance(owner,2499)&&curve.burden_units()==prior);
 check(curve.apply(owner,50,2,2500)&&curve.ceiling(20)<0&&curve.integer_ceiling(20)<0);
 // Death is not a clock pause. A new map/lifetime starts a fresh curve.
 check(curve.advance(owner,UINT64_MAX)&&!curve.active());
 check(!curve.next_integer_change());
 overcast::CeilingCurve end(owner,UINT64_MAX-1,33);check(end.apply(owner,1,1,UINT64_MAX-1)&&!end.next_integer_change());
 overcast::CeilingCurve zoned({2,42,8},100,33);check(!zoned.active()&&!zoned.advance(owner,200));
 overcast::CeilingCurve huge(owner,0,33);check(huge.apply(owner,UINT64_MAX/100000,1,0));
 const auto before=huge.burden_units();check(!huge.apply(owner,1,2,0)&&huge.burden_units()==before);
 check(!huge.apply(owner,1,2,1)&&huge.burden_units()==before&&huge.cursor()==0);
 check(huge.retire(owner)&&!huge.apply(owner,1,2,1)&&!huge.advance(owner,1));
 std::cout<<"PASS owned continuous Overcast ceiling, native/frozen values, cadence, signed limit and replay/scope/overflow guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
