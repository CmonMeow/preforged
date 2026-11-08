#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
using namespace skills;
using namespace skills::effects;
namespace {
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bestial Fury World line "+std::to_string(at.line()));
}
}
int main()try{
    warrior_test::Fixture f(340,false);
    auto& w=f.world;
    Slots bar{};bar[0]={1209,0};bar[1]={1118,0};bar[2]={340,0};
    check(w.configure_adrenaline(f.source,bar,100));
    const auto plan=*bestial_fury(12);
    check(!w.status_mutation(f.source,1209,123,12,100,100+plan.duration+1,true));
    check(w.status_mutation(f.source,1209,123,12,100,100+plan.duration,true));
    const auto effects=w.status_snapshot(f.source,200);
    check(effects&&bestial_fury_active(*effects,200));
    check(w.weapon_attack_timing(f.source,200)->period_ms==1064);
    check(w.weapon_attack_timing(f.source,200,true)->period_ms==1064);
    check(w.attack_duration(f.source,500,200)==400);
    check(w.attack_duration(f.source,500,200,true)==400);
    check(w.skill_disabled_slot(f.source,{1118,0},200)==5100);
    check(w.skill_disabled_slot(f.source,{1209,0},200)==5100);
    check(w.skill_disabled_slot(f.source,{340,0},200)==0);
    check(w.skill_disabled_slot(f.source,{1118,1},200)==0);
    const auto before=*w.disable_reset_revision(f.source);
    check(!w.reset_skill_disables(f.source,{{1118,0}},DisableRevisions{},300));
    check(w.reset_skill_disables(f.source,{{1118,0}},before,300));
    check(w.skill_disabled_slot(f.source,{1118,0},300)==0);
    check(w.skill_disabled_slot(f.source,{1209,0},300)==5100);
    check(w.status_mutation(f.source,370,124,0,400,5400,true));
    check(w.attack_duration(f.source,500,400)==335);
    check(w.skill_disabled_slot(f.source,{1209,0},400)==5100);
    check(w.skill_disabled_slot(f.source,{1209,0},5100)==0);
    auto stale=f.source;++stale.generation;
    check(!w.skill_disabled_slot(stale,{1209,0},5100));
    std::cout<<"PASS Bestial Fury World stance, 25% IAS, exact non-attack disable, reset, replacement and lifetime; not normal-cast verified\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
