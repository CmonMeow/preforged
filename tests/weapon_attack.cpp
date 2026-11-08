#include "../src/runtime/gw_weapon_attack.h"
#include <iostream>
#include <limits>
int main()try{
 using namespace fw::gw::combat::weapon;
 const auto check=[](bool ok){if(!ok)throw std::runtime_error("weapon strike regression");};
 Strike zero;zero.minimum=zero.maximum=5;zero.base_roll=zero.miss_roll=.5;
 check(!resolve(zero));const auto npc_zero=resolve(zero,true);
 check(npc_zero&&!npc_zero->missed&&npc_zero->damage==5&&zero.level==0);
 zero.level=256;check(!resolve(zero,true));
 Strike strike;strike.level=20;strike.mastery=12;strike.minimum=15;strike.maximum=22;strike.location_armor=60;strike.base_roll=.5;strike.miss_roll=.95;
 auto value=resolve(strike);check(value&&!value->missed&&value->damage==18.5);
 strike.customized=true;value=resolve(strike);check(value&&std::abs(value->damage-22.2)<1e-9);
 strike.mastery=14;value=resolve(strike);check(value&&std::abs(value->damage-22.2*std::exp2(.1))<1e-9);
 strike.critical=true;value=resolve(strike);check(value&&std::abs(value->damage-26.4*std::exp2(.6))<1e-9);
 strike.weakness=true;value=resolve(strike);check(value&&std::abs(value->damage-26.4*.34*std::exp2(.6))<1e-9);
 strike.blind=true;strike.miss_roll=.899999;value=resolve(strike);check(value&&value->missed&&value->damage==0);
 strike.miss_roll=.9;value=resolve(strike);check(value&&!value->missed);
 strike.caster_weapon=true;strike.critical=false;strike.customized=false;strike.weakness=false;strike.blind=false;strike.mastery=0;value=resolve(strike);check(value&&value->damage==18.5);
 strike.level=10;strike.location_armor=30;value=resolve(strike);check(value&&value->damage==18.5);
 strike.caster_weapon=false;strike.level=20;strike.mastery=0;strike.location_armor=60;value=resolve(strike);check(value&&std::abs(value->damage-18.5*std::exp2(-1.5))<1e-9);
 strike.base_roll=std::numeric_limits<double>::quiet_NaN();check(!resolve(strike));strike.base_roll=1;check(!resolve(strike));strike.base_roll=.5;strike.minimum=23;check(!resolve(strike));
 std::cout<<"PASS reviewed strike breakpoints, caster level, critical/customization, Blind boundary, Weakness base and invalid inputs\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

