#include "gw_maintained_range.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool value){if(!value)throw std::runtime_error("maintained range regression");}
int main()try{
    using P=MaintainedRangePoint;
    const std::array<P,2> still{{{0,0,0},{1000,0,0}}};
    unsigned cases=0;
    for(int x:{-6000,-5000,-100,0,100,5000,6000})
    for(int y:{-5100,0,5100})for(int tip:{-7000,0,7000}){
        const std::array<P,3> path{{{0,double(x),double(y)},{400,double(tip),double(y)},{1000,double(x),double(y)}}};
        std::optional<std::uint64_t> expected;
        for(unsigned t=0;t<=1000;++t){
            const double px=t<=400?x+(tip-x)*double(t)/400:tip+(x-tip)*double(t-400)/600;
            if(std::hypot(px,double(y))>5000){expected=t;break;}
        }
        const auto result=maintained_range_exit(still,path,0,1000);check(result&&result->first_outside==expected);++cases;
    }
    const std::array<P,3> excursion{{{0,4900,0},{20,5100,0},{40,4900,0}}};
    check(maintained_range_exit(still,excursion,0,40)->first_outside==11);
    check(maintained_range_exit(still,excursion,11,12)->first_outside==11);
    const std::array<P,2> edge{{{0,5000,0},{1000,5000,0}}};
    check(!maintained_range_exit(still,edge,0,1000)->first_outside);
    const std::array<P,3> follower{{{0,0,0},{17,170,0},{40,400,0}}};
    const std::array<P,2> leader{{{0,5000,0},{40,5400,0}}};
    check(!maintained_range_exit(follower,leader,0,40)->first_outside);
    auto bad=edge;bad[1].at=0;check(!maintained_range_exit(still,bad,0,1000));
    bad=edge;bad[0].x=std::numeric_limits<double>::infinity();check(!maintained_range_exit(still,bad,0,1000));
    check(!maintained_range_exit(still,edge,0,1001));check(!maintained_range_exit(still,edge,10,9));check(!maintained_range_exit(still,edge,0,1000,0));
    std::cout<<"PASS "<<cases<<" millisecond-oracle trajectories,exit/reentry,exact edge,moving actors and input guards;World adapter pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
