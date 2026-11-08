#include "gw_world_simulation.h"
#include <cmath>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Imbue Health hostile NPC line "+std::to_string(at.line()));
}

int main()try{
    world::Instance world;
    std::vector<world::Instance::Npc> npcs;
    for(unsigned id:{100u,101u,102u,103u,104u,105u}){
        world::Instance::Npc npc;
        npc.id=npc.definition.id=id;
        npc.x=id==104?2000.f:float((id-100)*100);
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;
        body.binding.map=474;
        body.binding.actor=body.binding.definition=id;
        body.relation=combat::npcs::Relation::hostile_to_players;
        body.packed_allegiance=id==103?std::nullopt:std::optional(id==102?4u:3u);
        body.spawn_health={id==100?480.:id==105?400.:id==102?50.:id==103?20.:100.,1000};
        body.spawn_energy={20,20};body.energy_pips=0;
        body.damageable=true;
        npc.combat_definition=body;
        npcs.push_back(npc);
    }
    world.configure_npcs(npcs);
    world.join(1,"Observer",0,0,std::make_shared<world::Outbox>());
    const auto caster=*world.effect_scope(100),same=*world.effect_scope(101);
    const auto other=*world.effect_scope(102),unknown=*world.effect_scope(103);
    const auto far=*world.effect_scope(104);
    const auto plan=*skills::effects::imbue_health(12);
    check(!world.npc_imbue_health(caster,caster,plan,100));
    check(!world.npc_imbue_health(caster,other,plan,100));
    check(!world.npc_imbue_health(caster,unknown,plan,100));
    check(!world.npc_imbue_health(caster,far,plan,100));
    auto stale=same;++stale.generation;
    check(!world.npc_imbue_health(caster,stale,plan,100));
    auto forged=plan;forged.cap=999;
    check(!world.npc_imbue_health(caster,same,forged,100));
    check(world.npc_imbue_health(caster,same,plan,100));
    check(std::abs(world.npc_combat_snapshot(same)->health->current-296.8)<.001);
    check(world.npc_combat_snapshot(caster)->health->current==480.);
    check(world.npc_imbue_health(caster,same,*skills::effects::imbue_health(20),100));
    check(std::abs(world.npc_combat_snapshot(same)->health->current-596.8)<.001);
    {
        world::Instance automatic;
        automatic.configure_npcs(npcs);
        automatic.join(1,"Observer",0,0,std::make_shared<world::Outbox>());
        const auto source=*automatic.effect_scope(100),target=*automatic.effect_scope(101);
        const auto less_wounded=*automatic.effect_scope(105);
        check(!automatic.configure_npc_imbue_ai(source,12,.8));
        check(automatic.configure_npc_spellbook(source,{{{1526,0},1248,
            "completed DoA Imbue Health capture",combat::ai::SpellTargetClass::ally}}));
        check(automatic.configure_npc_imbue_ai(source,12,.8));
        check(!automatic.configure_npc_imbue_ai(source,12,.8));
        automatic.tick(1000);
        check(automatic.npc_energy_snapshot(source)->current==10);
        check(automatic.npc_combat_snapshot(target)->health->current==100);
        automatic.tick(1250);
        check(std::abs(automatic.npc_combat_snapshot(target)->health->current-296.8)<.001);
        check(automatic.npc_combat_snapshot(less_wounded)->health->current==400);
    }
    {
        world::Instance cast;
        cast.configure_npcs(npcs);
        auto box=std::make_shared<world::Outbox>();
        cast.join(1,"Observer",0,0,box);
        const auto source=*cast.effect_scope(100),target=*cast.effect_scope(101);
        check(cast.configure_npc_spellbook(source,{{{1526,0},1248,
            "completed DoA Imbue Health capture",combat::ai::SpellTargetClass::ally}}));
        box->drain();
        const auto energy=cast.npc_energy_snapshot(source)->current;
        check(!cast.begin_npc_imbue_health(source,*cast.effect_scope(103),12,999));
        check(cast.npc_energy_snapshot(source)->current==energy);
        const auto token=cast.begin_npc_imbue_health(source,target,12,1000);
        check(token&&token->skill&&token->skill->skill==1526&&token->completes==1250);
        check(cast.npc_energy_snapshot(source)->current==energy-skills::find_skill(1526)->energy_cost());
        check(cast.npc_combat_snapshot(target)->health->current==100);
        check(!cast.complete_npc_action(*token,1250));
        check(!cast.complete_npc_imbue_health(*token,1249));
        cast.tick(1250);
        check(std::abs(cast.npc_combat_snapshot(target)->health->current-296.8)<.001);
        check(!cast.complete_npc_imbue_health(*token,1250));
        check(!cast.begin_npc_imbue_health(source,target,12,1251));
        const auto packets=box->drain();
        const auto body=combat::detail::integer(21,source.actor,1400);
        check(std::count(packets.begin(),packets.end(),body)==1);
    }
    {
        world::Instance interrupted;
        interrupted.configure_npcs(npcs);
        interrupted.join(1,"Interrupter",0,0,std::make_shared<world::Outbox>());
        interrupted.set_vitals(1,{500,500,100,100},100);
        const auto player=*interrupted.effect_scope(1);
        const auto source=*interrupted.effect_scope(100),target=*interrupted.effect_scope(101);
        check(interrupted.configure_npc_spellbook(source,{{{1526,0},1248,
            "completed DoA Imbue Health capture",combat::ai::SpellTargetClass::ally}}));
        const auto token=interrupted.begin_npc_imbue_health(source,target,12,1000);
        check(bool(token));
        check(interrupted.interrupt_skill(player,source,*skills::effects::interrupt_skill(23,0),20,1100));
        interrupted.tick(1250);
        check(interrupted.npc_combat_snapshot(target)->health->current==100);
        check(!interrupted.complete_npc_imbue_health(*token,1250));
    }
    {
        world::Instance lost_target;
        lost_target.configure_npcs(npcs);
        auto box=std::make_shared<world::Outbox>();
        lost_target.join(1,"Attacker",0,0,box);
        lost_target.set_vitals(1,{500,500,100,100},100);
        const auto player=*lost_target.effect_scope(1);
        const auto source=*lost_target.effect_scope(100),target=*lost_target.effect_scope(101);
        check(lost_target.configure_npc_spellbook(source,{{{1526,0},1248,
            "completed DoA Imbue Health capture",combat::ai::SpellTargetClass::ally}}));
        const auto token=lost_target.begin_npc_imbue_health(source,target,12,1000);
        check(bool(token));box->drain();
        const auto hit=lost_target.damage_npc(player,target,
            {combat::Kind::armor_ignoring,200,20,23},1248,1100);
        check(hit.killed);
        lost_target.tick(1250);
        check(!lost_target.complete_npc_imbue_health(*token,1250));
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),
            combat::detail::integer(21,source.actor,1400))==0);
    }
    for(unsigned mode=0;mode<5;++mode){
        auto population=npcs;
        population.front().combat_definition->spawn_health={mode==4?200.:100.,1000};
        population.front().combat_definition->healing_allowed=true;
        population.front().combat_definition->healing_evidence="authored Renewal ordering";
        world::Instance w;w.configure_npcs(population);auto box=std::make_shared<world::Outbox>();
        w.join(1,"Observer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto observer=*w.effect_scope(1),source=*w.effect_scope(100),target=*w.effect_scope(101);
        check(w.configure_npc_spellbook(source,{{{1526,0},1248,
            "authored Renewal before Imbue",combat::ai::SpellTargetClass::ally}}));
        check(w.apply_npc_ether_renewal(source,*skills::effects::ether_renewal(181,15),100));
        if(mode>=3)check(w.backfire(observer,source,*skills::effects::backfire(28,12),20,100));
        const auto token=w.begin_npc_imbue_health(source,target,12,1000);check(bool(token));
        box->drain();
        if(mode==1)check(w.interrupt_skill(observer,source,*skills::effects::interrupt_skill(23,0),20,1100));
        if(mode==2)check(w.damage_npc(observer,target,{combat::Kind::armor_ignoring,200,20,23},1248,1100).killed);
        const auto health_before_terminal=w.npc_combat_snapshot(source)->health->current;
        w.tick(1250);
        const auto source_health=w.npc_combat_snapshot(source)->health->current;
        const auto target_health=w.npc_combat_snapshot(target)->health->current;
        if(mode==0||mode==4){
            const double expected_source=mode==0?120.:101.;
            check(std::abs(source_health-expected_source)<.001);
            check(std::abs(target_health-(100+expected_source*.41))<.001);
            check(w.npc_energy_snapshot(source)->current==14);
        }else{
            check(source_health==(mode==3?0.:health_before_terminal));
            check(target_health==(mode==2?0.:100.));
            check(w.npc_energy_snapshot(source)->current==10);
        }
        check(!w.complete_npc_imbue_health(*token,1250));
    }
    std::cout<<"PASS hostile NPC Imbue Health paid action and Renewal/reaction ordering\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
