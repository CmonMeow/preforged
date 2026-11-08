#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;using namespace skills;
static void check(bool good,std::source_location at=std::source_location::current()){
    if(!good)throw std::runtime_error("Heket cast line "+std::to_string(at.line()));
}
int main()try{
    warrior_test::Fixture fixture(341,false);auto& world=fixture.world;
    Slots bar{};bar[0]={1728,0};check(world.configure_adrenaline(fixture.source,bar,100));
    Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[1728]=1;
    attributes::Ranks ranks{};ranks[22]=0;
    casting::Runtime runtime(1);casting::Hooks hooks;std::vector<Packet> packets;
    unsigned paid=0,starts=0;
    hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
    hooks.resources=[&](auto actor){return world.resources(actor);};
    hooks.target_valid=[](auto actor,float,bool self){return actor==1&&self;};
    hooks.consume_energy_at=[&](auto amount,auto now){paid+=amount;return world.consume_energy(1,amount,fixture.source,now);};
    hooks.effect_snapshot=[&](auto now){return *world.status_snapshot(fixture.source,now);};
    hooks.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return world.status_mutation(fixture.source,skill,effect,rank,now,until,active);};
    hooks.begin_action=[&](auto slot,auto now){++starts;check(world.accepted_skill_start(fixture.source,slot,now));};
    const auto snapshot=hooks.effect_snapshot;hooks.effect_snapshot={};
    check(runtime.start(1000,{1728,0,1},bar,policy,ranks,true,hooks)==casting::Reason::unsupported);
    check(paid==0&&starts==0&&world.status_snapshot(fixture.source,1000)->empty());
    hooks.effect_snapshot=snapshot;
    const auto reason=runtime.start(1000,{1728,0,1},bar,policy,ranks,true,hooks);
    if(reason!=casting::Reason::accepted)std::cerr<<"Heket start reason="<<unsigned(reason)<<" paid="<<paid<<" starts="<<starts<<'\n';
    check(reason==casting::Reason::accepted);
    check(paid==5&&starts==1);
    const auto active=*world.status_snapshot(fixture.source,1000);
    check(active.size()==1&&active[0].skill==1728&&active[0].expires==6000);
    check(std::count(packets.begin(),packets.end(),casting::generic(48,1,1728))==1);
    check(std::count(packets.begin(),packets.end(),casting::generic(21,1,765))==1);
    check(runtime.start(1001,{1728,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    check(paid==5&&starts==1);
    // World owns early release removal, while Runtime retains its activation
    // cache until the next update. Reconciliation must not emit a second end
    // at the old natural expiry or clear the paid skill's recharge.
    const auto effect=active.front().instance;
    fixture.box->drain();
    check(world.melee_attack(fixture.source,fixture.target,fixture.weapon,*combat::weapon::attack_skill(341,12),1100));
    const auto released=fixture.box->drain();
    check(std::count(released.begin(),released.end(),casting::effect_remove(1,effect))==1);
    check(world.status_snapshot(fixture.source,1100)->empty());
    packets.clear();runtime.update(1100,hooks);runtime.update(6000,hooks);
    check(std::count(packets.begin(),packets.end(),casting::effect_remove(1,effect))==0);
    check(runtime.start(6001,{1728,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    check(paid==5&&starts==1);
    check(runtime.start(11000,{1728,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
    check(paid==10&&starts==2);
    const auto renewed=*world.status_snapshot(fixture.source,11000);
    check(renewed.size()==1&&renewed.front().instance!=effect&&renewed.front().expires==16000);
    packets.clear();runtime.update(16000,hooks);runtime.update(16001,hooks);
    check(std::count(packets.begin(),packets.end(),casting::effect_remove(1,renewed.front().instance))==1);
    check(world.status_snapshot(fixture.source,16001)->empty());
    std::cout<<"PASS Heket paid instant Runtime cast, body765, 5s status and recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
