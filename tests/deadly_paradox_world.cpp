#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Deadly Paradox World line "+std::to_string(at.line()));
}
int main()try{
    world::Instance world;
    auto box=std::make_shared<world::Outbox>();
    world.join(25,"Assassin",0,0,box,0,{}, {},100,1);
    world.set_vitals(25,{1000,1000,100,100},100);
    const auto owner=*world.effect_scope(25);
    skills::Slots bar{};
    bar[0]={572,0};bar[1]={775,0};bar[2]={776,0};bar[3]={777,0};bar[4]={769,0};
    check(world.configure_adrenaline(owner,bar,100));
    const auto plan=*skills::effects::deadly_paradox(0);
    check(!world.status_mutation(owner,572,82,0,100,100+plan.duration+1,true));
    check(world.skill_disabled_slot(owner,{775,0},100)==0);
    check(world.status_mutation(owner,572,82,0,100,100+plan.duration,true));
    const auto active=world.status_snapshot(owner,100);
    check(active&&active->size()==1&&active->front().skill==572);
    for(unsigned skill:{775u,776u,777u})
        check(world.skill_disabled_slot(owner,{skill,0},100)==10100);
    check(world.skill_disabled_slot(owner,{769,0},100)==0);
    check(world.skill_disabled_slot(owner,{572,0},100)==0);
    check(world.skill_disabled_slot(owner,{775,1},100)==0);
    const auto revision=*world.disable_reset_revision(owner);
    check(!world.reset_skill_disables(owner,{{775,0}},skills::effects::DisableRevisions{},200));
    check(world.reset_skill_disables(owner,{{775,0}},revision,200));
    check(world.skill_disabled_slot(owner,{775,0},200)==0);
    check(world.skill_disabled_slot(owner,{776,0},200)==10100);
    check(world.status_mutation(owner,370,83,0,300,5300,true));
    check(world.skill_disabled_slot(owner,{776,0},300)==10100);
    check(world.skill_disabled_slot(owner,{776,0},10100)==0);
    auto stale=owner;++stale.generation;
    check(!world.skill_disabled_slot(stale,{776,0},10100));
    world.leave(25);
    check(!world.skill_disabled_slot(owner,{776,0},10100));
    std::cout<<"PASS Deadly Paradox World exact attack disables, reset, stance replacement, expiry and lifetime\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
