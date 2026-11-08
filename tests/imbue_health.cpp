#include "gw_imbue_health.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("Imbue Health plan");}

int main(){
    check(imbue_health(0)->percent==5);
    check(imbue_health(12)->percent==41);
    check(imbue_health(15)->percent==50);
    check(imbue_health(20)->percent==65);
    check(!imbue_health(21));
    const auto plan=*imbue_health(12);
    check(std::abs(*imbue_health_amount(plan,480.)-196.8)<1e-9);
    check(*imbue_health_amount(*imbue_health(20),800.)==300.);
    check(!imbue_health_amount(plan,0.));
    check(!imbue_health_amount(plan,std::numeric_limits<double>::infinity()));
    auto forged=plan;forged.cap=1000;
    check(!imbue_health_amount(forged,500.));
    std::cout<<"PASS Imbue Health ranks, current-Health scaling, cap, forged-plan rejection; World/cast unbound\n";
}
