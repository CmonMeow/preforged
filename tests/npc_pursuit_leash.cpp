#include "gw_npc_pursuit_leash.h"
#include <array>
#include <stdexcept>
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::combat::ai::motion;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC pursuit leash line "+std::to_string(at.line()));
}
bool near(double a,double b){return std::abs(a-b)<1e-8;}
int main()try{
    const std::array<Point,1> far{{{1000,0}}};
    const auto cap=*leash_budget({0,0},far,{0,0},100,900);
    check(cap.boundary&&near(cap.budget,100));
    const auto moved=*advance({0,0},far,{1000,0},0,cap.budget);
    check(near(moved.point.x,100)&&near(moved.point.y,0)&&!moved.reached);
    check(!leash_budget({0,0},far,{0,0},100,99)->boundary);
    // Full route geometry matters: the second segment leaves the circle even
    // though both the start and eventual goal are inside it.
    const std::array<Point,3> detour{{{0,60},{200,60},{10,0}}};
    const auto corner=*leash_budget({0,0},detour,{0,0},100,500);
    check(corner.boundary&&near(corner.budget,140));
    const auto tangent=*advance({0,0},detour,{10,0},0,corner.budget);
    check(near(tangent.point.x,80)&&near(tangent.point.y,60));
    check(tangent.facing&&near(*tangent.facing,0));
    const auto sliced=*leash_budget({0,60},std::span(detour).subspan(1),{0,0},100,440);
    check(sliced.boundary&&near(sliced.budget+60,corner.budget));
    check(leash_budget({100,0},far,{0,0},100,10)->budget==0);
    const std::array<Point,1> inward{{{0,0}}};
    check(!leash_budget({100,0},inward,{0,0},100,50)->boundary);
    check(leash_budget({101,0},inward,{0,0},100,50)->boundary);
    // A goal stop before the boundary remains a successful approach.
    const auto early=*advance({0,0},far,{90,0},10,cap.budget);
    check(early.reached&&near(early.spent,80));
    check(!leash_budget({0,0},far,{0,0},0,10));
    check(!leash_budget({0,0},far,{0,0},100,-1));
    std::cout<<"NPC pursuit leash passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
