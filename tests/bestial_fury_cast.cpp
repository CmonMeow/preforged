#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
using namespace skills;
using namespace skills::effects;
namespace {
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bestial Fury cast line "+std::to_string(at.line()));
}
}
int main()try{
    warrior_test::Fixture f(340,false);auto& w=f.world;
    Slots bar{};bar[0]={1209,0};bar[1]={456,0};bar[2]={395,0};
    check(w.configure_adrenaline(f.source,bar,100));
    casting::Runtime runtime(1);casting::Hooks hooks;
    std::vector<Packet> packets;
    hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
    hooks.resources=[&](auto id){return w.resources(id);};
    hooks.consume_energy=[&](auto amount){return w.consume_energy(1,amount,f.source);};
    hooks.target_valid=[](auto id,float,bool self){return id==1&&self;};
    hooks.target_scope=[&](auto id){return w.effect_scope(id);};
    hooks.current_skillbar=[&]()->std::optional<Slots>{return bar;};
    hooks.skill_disabled_slot=[&](Slot slot,auto at){return w.skill_disabled_slot(f.source,slot,at);};
    hooks.effect_snapshot=[&](auto at){return w.status_snapshot(f.source,at);};
    hooks.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto until,bool active){
        return w.status_mutation(f.source,skill,effect,rank,at,until,active);
    };
    Policy policy;policy.primary=2;policy.campaigns=255;
    policy.learned[1209]=policy.learned[456]=policy.learned[395]=1;
    attributes::Ranks ranks{};ranks[22]=12;
    const auto result=runtime.start(100,{1209,0,2},bar,policy,ranks,true,hooks);
    if(result!=casting::Reason::accepted)std::cerr<<casting::reason_text(result)<<'\n';
    check(result==casting::Reason::accepted);
    check(w.resources(1)->energy==20);
    const auto active=w.status_snapshot(f.source,100);
    check(active&&active->size()==1&&active->front().skill==1209&&
          active->front().expires==10100);
    check(std::count(packets.begin(),packets.end(),casting::effect_add(1,1209,12,1,10.f))==1);
    check(std::count(packets.begin(),packets.end(),casting::generic(21,1,766))==1);
    check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{1209,0},10))==1);
    runtime.update(101,hooks);
    check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{456,0},5))==1);
    check(w.skill_disabled_slot(f.source,{456,0},101)==5100);
    check(w.skill_disabled_slot(f.source,{395,0},101)==0);
    check(runtime.start(102,{1209,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    std::cout<<"PASS Bestial Fury paid native cast, effect/body, recharge and exact non-attack disable feedback; rendered 2022 pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
