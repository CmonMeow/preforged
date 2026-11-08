#include "gw_essence_strike.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw::skills::effects;
static void check(bool ok){if(!ok)throw std::runtime_error("Essence Strike plan or spirit gate");}
int main()try{
    check(essence_strike(0)->lightning_damage==15&&essence_strike(0)->spirit_energy==1);
    check(essence_strike(15)->lightning_damage==60&&essence_strike(15)->spirit_energy==9);
    check(!essence_strike(21));
    auto target=fw::gw::combat::Actor{};
    target.id=2;target.generation=1;target.health=fw::gw::combat::Health{500,500};
    target.lightning_armor=60;target.damageable=true;
    const auto plan=*essence_strike(15);
    const auto present=essence_strike_target(plan,target,20,SpiritProximity::present);
    const auto absent=essence_strike_target(plan,target,20,SpiritProximity::absent);
    check(present&&absent&&present->hit.applied==absent->hit.applied&&
          present->energy_gain==9&&absent->energy_gain==0);
    check(!essence_strike_target(plan,target,20,SpiritProximity::unknown));
    auto forged=plan;forged.spirit_energy=99;
    check(!essence_strike_target(forged,target,20,SpiritProximity::present));
    target.damageable=false;
    check(!essence_strike_target(plan,target,20,SpiritProximity::present));
    std::cout<<"PASS Essence Strike lightning hit, rank scale and authoritative spirit gate; World binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
