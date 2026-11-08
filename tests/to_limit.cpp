#include "gw_to_limit.h"
#include "gw_adrenaline.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool value){if(!value)throw std::runtime_error("To the Limit regression");}
int main()try{
    check(to_limit(0)->health_bonus==10&&to_limit(12)->health_bonus==50&&to_limit(15)->health_bonus==60);
    check(to_limit(0)->maximum_foes==1&&to_limit(12)->maximum_foes==5&&to_limit(15)->maximum_foes==6&&!to_limit(21));
    for(unsigned rank=0;rank<=20;++rank){const auto plan=*to_limit(rank);auto bad=plan;++bad.health_bonus;check(!to_limit_result(bad,1));
        const auto zero=*to_limit_result(plan,0),one=*to_limit_result(plan,1),full=*to_limit_result(plan,UINT32_MAX);
        check(zero.health_bonus==plan.health_bonus&&zero.duration==plan.duration&&zero.adrenaline_units==0);
        check(one.adrenaline_units==25&&full.adrenaline_units==25*plan.maximum_foes);
        skills::Slots bar{};bar[0]={335,0};bar[1]={335,1};bar[2]={336,0};combat::adrenaline::State state;check(state.configure(bar,100));
        check(state.gain(full.adrenaline_units,100));const auto& pools=state.pools();check(pools.size()==3);
        for(const auto& pool:pools)check(pool.units==std::min(pool.cost,full.adrenaline_units));
    }
    std::cout<<"PASS21 To the Limit rank/cap/zero-foe/full-foe proposals and duplicate-slot adrenaline caps;World temporary Health/casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
