#include "gw_grenths_balance.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Grenth balance policy regression");}
int main()try{
 const auto normal=grenths_balance({100,500},{500,500});check(normal&&normal->caster.current==300&&normal->target.current==300&&normal->target_loss==200);
 const auto capped=grenths_balance({100,500},{2000,2000});check(capped&&capped->caster.current==500&&capped->caster_gain==400&&capped->target_loss==500&&capped->target.current==1500);
 const auto reverse=grenths_balance({500,500},{100,500});check(reverse&&reverse->caster.current==300&&reverse->target.current==300&&reverse->caster_loss==200);
 const auto equal=grenths_balance({300,500},{300,500});check(equal&&equal->caster_gain==0&&equal->caster_loss==0);
 const auto fractional=grenths_balance({100,500},{501,600});check(fractional&&fractional->caster_gain==200.5&&fractional->target_loss==200.5);
 const auto dead=grenths_balance({0,500},{2000,2000},true);check(dead&&dead->caster.current==0&&dead->caster_gain==0&&dead->target_loss==500);
 check(!grenths_balance({0,500},{500,500})&&!grenths_balance({100,500},{0,500})&&!grenths_balance({100,500},{500,500},true));
 std::cout<<"PASS candidate Grenth balance arithmetic, gain cap, reverse/equal/fractional and post-reaction death; World/retail edge verification pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
