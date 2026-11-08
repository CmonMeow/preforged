#include "gw_mark_of_pain.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Mark of Pain qualification regression");}
int main()try{
    check(!mark_of_pain(21));
    for(unsigned rank=0;rank<=20;++rank){const auto plan=mark_of_pain(rank);check(plan&&plan->damage==10+2*rank&&plan->duration==30000&&plan->radius==166.);}
    Entry e{7,42,150,12,Category::hex,100,30100};
    for(unsigned raw=0;raw<14;++raw)check(bool(mark_of_pain_physical(e,raw,1,100))==(raw<=2));
    for(double damage:{0.,-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})check(!mark_of_pain_physical(e,0,damage,100));
    check(!mark_of_pain_physical(e,{},1,100)&&!mark_of_pain_physical(e,0,1,99)&&!mark_of_pain_physical(e,0,1,30100));
    check(mark_of_pain_physical(e,2,1,30099)->damage==34);
    for(unsigned invalid=0;invalid<6;++invalid){auto bad=e;
        if(invalid==0)bad.instance=0;if(invalid==1)bad.cause=0;if(invalid==2)bad.skill=101;
        if(invalid==3)bad.kind=Category::enchantment;if(invalid==4)bad.expires.reset();if(invalid==5)bad.rank=21;
        check(!mark_of_pain_physical(bad,0,1,100));
    }
    std::cout<<"PASS Mark of Pain rank0..20,physical kind and positive finite damage qualification,expiry and malformed effect rejection;world/casting/rendering pending\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
