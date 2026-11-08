#include "gw_ghostmirror_light.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw::skills::effects;
static void check(bool yes){if(!yes)throw std::runtime_error("Ghostmirror Light plan");}

int main()try{
    check(ghostmirror_light(0)->heal==15);
    check(ghostmirror_light(12)->heal==75);
    check(ghostmirror_light(15)->heal==90);
    check(!ghostmirror_light(21));
    const auto plan=*ghostmirror_light(12);
    check(quote_ghostmirror_light(plan,SpiritProximity::absent)->recipient_heal==75);
    check(quote_ghostmirror_light(plan,SpiritProximity::absent)->caster_heal==0);
    check(quote_ghostmirror_light(plan,SpiritProximity::present)->caster_heal==75);
    check(!quote_ghostmirror_light(plan,SpiritProximity::unknown));
    auto forged=plan;++forged.heal;
    check(!quote_ghostmirror_light(forged,SpiritProximity::present));
    std::cout<<"PASS Ghostmirror Light pinned row and conditional double-heal quote\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
