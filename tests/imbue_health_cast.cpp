#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Imbue Health cast line "+std::to_string(at.line()));
}

int main(){
    world::Instance w;
    w.join(1,"Dervish",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.set_vitals(1,{480,480,40,40},100);
    w.set_vitals(2,{100,1000,40,40},100);
    const auto source=*w.effect_scope(1);
    bool valid=true;
    std::vector<world::Packet> sent;
    skills::casting::Hooks hooks;
    hooks.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
    hooks.resources=[&](auto actor){return w.resources(actor);};
    hooks.consume_energy=[&](auto amount){return w.consume_energy(1,amount);};
    hooks.target_scope=[&](auto actor){return valid?w.effect_scope(actor):std::nullopt;};
    hooks.infuse_target_valid=[&](auto actor,auto now){return valid&&w.valid_infuse_target(source,actor,now);};
    hooks.imbue_health=[&](auto target,const auto& plan,auto now){return w.imbue_health(source,target,plan,now);};
    skills::Policy policy;
    policy.primary=10;policy.campaigns=0x1e;policy.learned[1526]=1;
    skills::Slots bar{};bar[0]={1526,0};
    attributes::Ranks ranks{};ranks[44]=12;
    skills::casting::Runtime cast(1);
    check(cast.start(100,{1526,0,1},bar,policy,ranks,true,hooks)==skills::casting::Reason::target);
    check(cast.start(100,{1526,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
    check(w.resources(2)->health==100);
    cast.update(349,hooks);
    check(w.resources(2)->health==100);
    cast.update(350,hooks);
    check(w.resources(1)->health==480&&w.resources(2)->health==297);
    check(w.resources(1)->energy==30);
    check(std::count(sent.begin(),sent.end(),skills::casting::generic(21,1,1400))==1);
    check(std::count(sent.begin(),sent.end(),skills::casting::generic(21,2,2077))==0);

    skills::casting::Runtime cancelled(1);
    check(cancelled.start(2000,{1526,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
    cancelled.cancel(hooks);cancelled.update(2250,hooks);
    check(w.resources(2)->health==297);
    skills::casting::Runtime lost(1);
    check(lost.start(3000,{1526,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
    valid=false;lost.update(3250,hooks);
    check(w.resources(2)->health==297);
    valid=true;
    skills::casting::Runtime missing(1);
    hooks.imbue_health={};
    check(missing.start(4000,{1526,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::unsupported);
    std::cout<<"PASS Imbue Health player cast, delayed heal, cost, body1400, cancel, target loss and missing hook\n";
}
