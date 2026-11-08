#include "gw_npc_pursuit_outcome.h"
#include <array>
#include <iostream>
#include <limits>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::combat::ai::motion;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC pursuit outcome line "+std::to_string(at.line()));
}
int main()try{
    const std::array<Point,3> detour{{{-50,0},{-50,100},{100,100}}};
    const auto step=*advance({0,0},detour,{100,100},10,50);
    check(std::hypot(step.point.x-100,step.point.y-100)>std::hypot(100.,100.));
    const auto moving=describe(step);check(moving.kind==OutcomeKind::progressing&&moving.travelled==50&&!bool(moving));
    const auto zero=describe(*advance({0,0},detour,{100,100},10,0));
    check(zero.kind==OutcomeKind::deferred&&zero.deferral==Deferral::zero_budget);
    const auto observation=Outcome::deferred(Deferral::observation);
    check(observation.kind==OutcomeKind::deferred&&observation.deferral!=zero.deferral);
    const auto arrived=describe(*advance({100,100},detour,{100,100},10,0));
    check(arrived.kind==OutcomeKind::reached&&arrived.travelled==0&&bool(arrived));
    const auto completed=describe(*advance({0,0},detour,{100,100},10,1000));
    check(completed.kind==OutcomeKind::reached&&completed.travelled>0&&bool(completed));
    const auto leash=describe(step,true);check(leash.kind==OutcomeKind::leash_boundary&&leash.travelled==50&&!bool(leash));
    // Exact arrival remains arrival, even if a route budget also touches its
    // leash. The driver separately checks the stored position's strict stop.
    check(describe(*advance({100,100},detour,{100,100},10,0),true).kind==OutcomeKind::reached);
    const Outcome no_route{OutcomeKind::route_unavailable},layer{OutcomeKind::unsupported_layer};
    check(!bool(no_route)&&!bool(layer)&&no_route.kind!=zero.kind&&layer.kind!=no_route.kind);
    check(no_route.deferral==Deferral::none&&layer.deferral==Deferral::none);
    auto malformed=step;malformed.spent=std::numeric_limits<double>::quiet_NaN();
    check(describe(malformed).kind==OutcomeKind::invalid_request);
    std::cout<<"NPC movement outcomes distinguish detours, arrival, deferral and route boundaries\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
