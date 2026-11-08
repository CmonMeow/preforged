#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace skills;
using namespace skills::effects;
namespace {
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Deadly Paradox cast line "+std::to_string(at.line()));
}
}
int main()try{
    warrior_test::Fixture f(340,false);auto& w=f.world;
    Slots bar{};bar[0]={572,0};bar[1]={775,0};bar[2]={769,0};
    bar[3]={776,0};bar[4]={777,0};
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
    Policy policy;policy.primary=7;policy.campaigns=255;
    for(const auto skill:{572u,775u,769u,776u,777u})policy.learned[skill]=1;
    attributes::Ranks ranks{};
    const auto energy=w.resources(1)->energy;
    const auto result=runtime.start(100,{572,0,1},bar,policy,ranks,true,hooks);
    if(result!=casting::Reason::accepted)std::cerr<<casting::reason_text(result)<<'\n';
    check(result==casting::Reason::accepted);
    check(w.resources(1)->energy+15==energy);
    const auto active=w.status_snapshot(f.source,100);
    check(active&&active->size()==1&&active->front().skill==572&&active->front().expires==5100);
    for(const auto skill:{775u,776u,777u}){
        check(w.skill_disabled_slot(f.source,{skill,0},100)==10100);
        check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{skill,0},10))==1);
    }
    check(w.skill_disabled_slot(f.source,{769,0},100)==0);
    check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{769,0},10))==0);
    check(std::count(packets.begin(),packets.end(),casting::generic(21,1,1360))==1);
    check(std::count(packets.begin(),packets.end(),casting::effect_add(1,572,0,1,5.f))==1);
    check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{572,0},10))==1);
    const auto stance=std::find(packets.begin(),packets.end(),casting::effect_add(1,572,0,1,5.f));
    const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,1,1360));
    for(const auto skill:{775u,776u,777u})
        check(body<std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{skill,0},10))&&
              std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{skill,0},10))<stance);
    check(runtime.start(101,{572,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    runtime.update(5100,hooks);
    check(w.skill_disabled_slot(f.source,{775,0},5100)==10100);
    runtime.update(10100,hooks);
    check(w.skill_disabled_slot(f.source,{775,0},10100)==0);
    std::cout<<"PASS Deadly Paradox paid native cast, body/status, exact attack disables and expiry; rendered 2022 pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
