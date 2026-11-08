#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;using namespace skills;
static void check(bool good,std::source_location at=std::source_location::current()){
    if(!good)throw std::runtime_error("Natural Stride cast line "+std::to_string(at.line()));
}
int main()try{
    warrior_test::Fixture fixture(341,false);auto& world=fixture.world;
    Slots bar{};bar[0]={1727,0};check(world.configure_adrenaline(fixture.source,bar,100));
    Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[1727]=1;
    attributes::Ranks ranks{};ranks[24]=0;
    casting::Runtime runtime(1);casting::Hooks hooks;std::vector<Packet> packets;
    unsigned paid=0,starts=0;
    hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
    hooks.resources=[&](auto actor){return world.resources(actor);};
    hooks.target_valid=[](auto actor,float,bool self){return actor==1&&self;};
    hooks.consume_energy_at=[&](auto amount,auto now){paid+=amount;return world.consume_energy(1,amount,fixture.source,now);};
    hooks.effect_snapshot=[&](auto now){return *world.status_snapshot(fixture.source,now);};
    hooks.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return world.status_mutation(fixture.source,skill,effect,rank,now,until,active);};
    hooks.begin_action=[&](auto slot,auto now){++starts;check(world.accepted_skill_start(fixture.source,slot,now));};
    const auto reason=runtime.start(1000,{1727,0,1},bar,policy,ranks,true,hooks);
    if(reason!=casting::Reason::accepted)std::cerr<<"Natural Stride start reason="<<unsigned(reason)<<" paid="<<paid<<" starts="<<starts<<'\n';
    check(reason==casting::Reason::accepted);
    check(paid==5&&starts==1);
    const auto active=*world.status_snapshot(fixture.source,1000);
    check(active.size()==1&&active[0].skill==1727&&active[0].expires==2000);
    check(std::count(packets.begin(),packets.end(),casting::generic(48,1,1727))==1);
    check(std::count(packets.begin(),packets.end(),casting::generic(21,1,724))==1);
    check(runtime.start(1001,{1727,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    check(paid==5&&starts==1);
    std::cout<<"PASS Natural Stride paid instant Runtime cast, body765, 5s status and recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

