#include "gw_signet_rejuvenation.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool yes){if(!yes)throw std::runtime_error("Signet of Rejuvenation plan");}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=signet_rejuvenation(rank);check(bool(plan));
        const auto expected=interpolate(15,75,rank);
        check(quote_signet_rejuvenation(*plan,false)==expected);
        check(quote_signet_rejuvenation(*plan,true)==2*expected);
        check(!quote_signet_rejuvenation(*plan,std::nullopt));
    }
    check(!signet_rejuvenation(21));
    auto forged=*signet_rejuvenation(12);++forged.conditional_heal;
    check(!quote_signet_rejuvenation(forged,true));
    check(signet_rejuvenation(0)->activation_ms==1000);
    check(signet_rejuvenation(0)->recharge_ms==8000);
    std::cout<<"PASS Signet of Rejuvenation rank and completion-time action quote\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
