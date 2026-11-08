#include "gw_spirit_light.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
#include <string>

using namespace fw::gw::skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Spirit Light policy line "+std::to_string(at.line()));
}

int main()try{
    check(!spirit_light(21));
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=spirit_light(rank);check(bool(plan));
        check(plan->rank==rank&&plan->energy==5&&plan->sacrifice_percent==17&&
              plan->activation_ms==1000&&plan->aftercast_ms==750&&plan->recharge_ms==4000&&
              plan->caster_body==2077&&plan->target_body==1089&&plan->earshot==1012);
        check(plan->heal==static_cast<unsigned>(std::lround(60.+120.*rank/15.)));
        const auto with_spirit=quote_spirit_light(*plan,SpiritProximity::present,80,100);
        check(with_spirit&&with_spirit->raw_heal==plan->heal&&!with_spirit->sacrifice);
        const auto without=quote_spirit_light(*plan,SpiritProximity::absent,80,100);
        check(without&&without->sacrifice&&without->sacrifice->requested==17&&
              without->sacrifice->remaining==63);
        check(!quote_spirit_light(*plan,SpiritProximity::unknown,80,100));
        auto forged=*plan;++forged.heal;
        check(!quote_spirit_light(forged,SpiritProximity::present,80,100));
    }
    const auto plan=*spirit_light(12);
    const auto lethal=quote_spirit_light(plan,SpiritProximity::absent,10,100);
    check(lethal&&lethal->sacrifice&&lethal->sacrifice->killed);
    check(!quote_spirit_light(plan,SpiritProximity::present,0,100));
    check(!quote_spirit_light(plan,SpiritProximity::present,101,100));
    check(!quote_spirit_light(plan,SpiritProximity::present,50,0));
    std::cout<<"Spirit Light pinned plan and conditional sacrifice policy passed; World/casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
