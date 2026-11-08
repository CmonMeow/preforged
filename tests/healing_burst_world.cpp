#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Healing Burst World line "+std::to_string(at.line()));
}
int main()try{
    world::Instance w;
    auto box=std::make_shared<world::Outbox>();
    for(unsigned id=1;id<=6;++id){
        const float x=id==1?0:id==2?100:id==3?200:id==4?150:id==5?1500:120;
        w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),
            0,{}, {},100,id==6?2:1);
        w.set_vitals(id,{id==1?300u:200u,500,100,100},100);
    }
    w.public_professions(1,3,0);
    for(unsigned id:{2u,3u,5u})
        check(w.party_action(1,158,id,0)&&w.party_action(id,154,1,0));
    const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
    skills::Slots bar{};bar[0]={1118,0};bar[1]={240,0};
    bar[2]={240,1};bar[3]={281,0};
    check(w.configure_adrenaline(caster,bar,100));
    const auto plan=*skills::effects::healing_burst(0,12);
    check(!w.stage_healing_burst_player(caster,*w.effect_scope(6),plan,1,200));
    auto forged=plan;++forged.divine_party_heal;
    check(!w.stage_healing_burst_player(caster,target,forged,1,200));
    box->drain();std::function<void()> publish;
    check(w.stage_healing_burst_player(caster,target,plan,1,200,&publish));
    check(bool(publish)&&box->drain().empty());
    check(w.resources(1)->health==338);
    check(w.resources(2)->health==248); // Target does not receive party bonus twice.
    check(w.resources(3)->health==238);
    check(w.resources(4)->health==200&&w.resources(5)->health==200);
    check(w.skill_disabled_slot(caster,{240,0},200)==20200);
    check(w.skill_disabled_slot(caster,{240,1},200)==20200);
    check(w.skill_disabled_slot(caster,{281,0},200)==0);
    check(w.disable_reset_revision(caster)->at(8)>0);
    publish();check(!box->drain().empty());
    check(!w.stage_healing_burst_player(caster,target,plan,1,300));
    const auto revision=*w.disable_reset_revision(caster);
    check(w.reset_skill_disables(caster,{{240,1}},revision,400));
    check(w.skill_disabled_slot(caster,{240,1},400)==0);
    check(w.skill_disabled_slot(caster,{240,0},400)==20200);
    check(w.stage_healing_burst_player(caster,*w.effect_scope(4),plan,2,500));
    check(w.resources(4)->health==248);
    check(w.resources(1)->health==376&&w.resources(2)->health==286&&
        w.resources(3)->health==276);
    check(w.resources(5)->health==200);
    for(unsigned mode=0;mode<6;++mode){
        world::Instance npc_world;
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;
        npc.x=mode==3?1800.f:60.f;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;
        definition.binding.map=146;
        definition.binding.actor=definition.binding.definition=100;
        definition.relation=mode==1?combat::npcs::Relation::neutral:
            mode==2?combat::npcs::Relation::hostile_to_players:
            combat::npcs::Relation::allied_to_players;
        definition.spawn_health=combat::Health{100,500};
        definition.damageable=definition.conditions_allowed=true;
        if(mode!=4){definition.healing_allowed=true;
            definition.healing_evidence="fixture";}
        npc.combat_definition=definition;npc_world.configure_npcs({npc});
        auto npc_box=std::make_shared<world::Outbox>();
        npc_world.join(1,"Caster",0,0,npc_box,0,{}, {},100,1);
        npc_world.join(2,"Party",100,0,std::make_shared<world::Outbox>(),
            0,{}, {},100,1);
        npc_world.set_vitals(1,{300,500,100,100},100);
        npc_world.set_vitals(2,{200,500,100,100},100);
        npc_world.public_professions(1,3,0);
        check(npc_world.party_action(1,158,2,0)&&
            npc_world.party_action(2,154,1,0));
        const auto npc_caster=*npc_world.effect_scope(1);
        const auto npc_target=*npc_world.effect_scope(100);
        check(npc_world.configure_adrenaline(npc_caster,bar,100));
        auto selected=npc_target;if(mode==5)++selected.generation;
        npc_box->drain();std::function<void()> npc_publish;
        const bool accepted=mode==0;
        check(npc_world.stage_healing_burst_npc(npc_caster,selected,
            plan,1,200,&npc_publish)==accepted);
        check(npc_box->drain().empty());
        check(npc_world.npc_combat_snapshot(npc_target)->health->current==
            (accepted?148:100));
        check(npc_world.resources(1)->health==(accepted?338u:300u));
        check(npc_world.resources(2)->health==(accepted?238u:200u));
        if(accepted){
            check(bool(npc_publish));npc_publish();
            const auto packets=npc_box->drain();
            check(std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(100,1,48,500))!=packets.end());
            check(npc_world.skill_disabled_slot(npc_caster,{240,0},200)==20200);
            check(!npc_world.stage_healing_burst_npc(npc_caster,npc_target,
                plan,1,300));
        }
    }
    std::cout<<"Healing Burst player target, party earshot, disable/reset and deferred publication pass\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
