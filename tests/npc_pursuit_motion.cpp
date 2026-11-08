#include "gw_npc_pursuit_motion.h"
#include <array>
#include <limits>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::combat::ai::motion;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("NPC pursuit motion line "+std::to_string(at.line()));
}
bool near(double a,double b){return std::abs(a-b)<1e-8;}
int main()try{
    const std::array<Point,3> corner{{{0,50},{100,50},{100,0}}};
    const auto whole=*advance({0,0},corner,{100,0},0,175);
    check(near(whole.point.x,100)&&near(whole.point.y,25)&&near(whole.spent,175)&&!whole.reached);
    check(whole.facing&&near(*whole.facing,-std::acos(-1.)/2));
    // The same route and elapsed travel budget must survive scheduler slicing.
    const auto first=*advance({0,0},corner,{100,0},0,60);
    const std::array<Point,2> remaining{{{100,50},{100,0}}};
    const auto second=*advance(first.point,remaining,{100,0},0,115);
    check(near(second.point.x,whole.point.x)&&near(second.point.y,whole.point.y));
    check(second.facing&&near(*second.facing,*whole.facing));
    const auto stopped=*advance({0,0},corner,{100,0},10,1000);
    check(stopped.reached&&near(stopped.point.x,100)&&near(stopped.point.y,10)&&near(stopped.spent,190));
    // A detour can initially move away from the goal; it still spends its
    // budget, rather than clamping each segment by direct distance minus stop.
    const std::array<Point,3> detour{{{-50,0},{-50,100},{100,100}}};
    const auto away=*advance({0,0},detour,{100,100},100,50);
    check(near(away.point.x,-50)&&near(away.spent,50)&&!away.reached);
    check(away.facing&&near(*away.facing,std::acos(-1.)));
    const std::array<Point,1> beyond{{{200,0}}};
    const auto entry=*advance({0,0},beyond,{100,0},20,200);
    check(entry.reached&&near(entry.point.x,80)&&near(entry.spent,80));
    const std::array<Point,2> duplicate{{{0,0},{100,0}}};
    check(near(advance({0,0},duplicate,{100,0},0,40)->point.x,40));
    check(advance({95,0},beyond,{100,0},10,0)->reached);
    check(!advance({95,0},beyond,{100,0},10,0)->facing);
    check(advance({0,0},beyond,{100,0},10,0)->spent==0);
    check(!advance({0,0},beyond,{100,0},10,0)->facing);
    check(!advance({0,0},beyond,{100,0},-1,10));
    check(!advance({0,0},beyond,{100,0},10,-1));
    auto invalid=beyond;invalid[0].x=std::numeric_limits<double>::quiet_NaN();
    check(!advance({0,0},invalid,{100,0},0,100));
    check(!advance({0,0},beyond,{100,0},0,std::numeric_limits<double>::infinity()));
    std::cout<<"NPC pursuit route movement passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
