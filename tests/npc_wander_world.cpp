#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC wander World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,player;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(bool elemental,float player_x){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
        if(elemental){d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;}
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Target",player_x,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
        source=*w.effect_scope(100);player=*w.effect_scope(1);
        check(w.physical_armor(player,0));check(w.elemental_armor(player,0,0));
        combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=200;
        p.acquire=200;p.follow=2000;p.leash=600;p.flight={1000,32};p.evidence="synthetic authored hostile patrol fixture";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,12),p.evidence}},4,1000));
        else check(w.configure_npc_hostile_ordinary(p,{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,250,1000,128,{1000,32},p.evidence},1000));
        check(w.configure_npc_wander(source,{{{100,0},{0,100}},200,2000,p.evidence},1000));
        if(!elemental)check(!w.npc_energy_snapshot(source));
        box->drain();
    }
    bool movement(){bool moved=false;for(const auto& p:box->drain())if(p.size()==16&&p[0]==44&&p[1]==0&&p[2]==100&&p[3]==0)moved=true;return moved;}
};
int main()try{
    for(bool elemental:{false,true}){
        {Fixture f(elemental,4000);f.w.tick(1000);check(!f.movement());
            f.w.tick(1200);check(!f.movement());
            check(f.w.npc_pursuit_movement_snapshot(f.source)->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
            f.w.tick(1300);check(f.movement());
            // Both combat owners share idle wandering; no fabricated resources.
            if(elemental)check(!f.w.npc_elemental_ai_snapshot(f.source)->engaged);
            else check(!f.w.npc_flare_ai_snapshot(f.source)->engaged);}
        {Fixture f(elemental,100);for(unsigned at=1000;at<=4000;at+=100)f.w.tick(at);
            check(f.w.resources(1)->health<1000);}
        {Fixture f(elemental,500);f.w.tick(1000);
            check(f.w.damage_npc(f.player,f.source,{combat::Kind::armor_ignoring,1,20,194},1248,1100).reason==combat::Reason::accepted);
            f.w.tick(1100);
            if(elemental)check(f.w.npc_elemental_ai_snapshot(f.source)->engaged);
            else check(f.w.npc_flare_ai_snapshot(f.source)->engaged);
            check(f.w.kill(f.player,1200));for(unsigned at=1200;at<=5000;at+=100)f.w.tick(at);
            f.box->drain();for(unsigned at=5100;at<=6500;at+=100)f.w.tick(at);check(f.movement());}
    }
    {
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc sample;sample.id=sample.definition.id=100;sample.definition.level=1;
        sample.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=160;d.binding.actor=d.binding.definition=100;d.binding.level=1;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{80,80};
        d.damageable=true;d.physical_armor=0;sample.combat_definition=d;
        w.join(1,"Quest owner",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
        const auto owner=*w.effect_scope(1);check(w.physical_armor(owner,0));
        check(w.spawn_kill_encounter(owner,75,1,{sample},[]{return true;}));const auto source=*w.effect_scope(60000);
        check(w.arm_grawl_melee(owner,1,1000));check(w.arm_grawl_wander(owner,1,1000)==1);
        check(w.arm_grawl_wander(owner,2,1000)==0);box->drain();
        w.tick(1000);check(w.arm_grawl_wander(owner,1,2000)==1);w.tick(2200);
        for(const auto& p:box->drain())check(!(p.size()==16&&p[0]==44&&p[1]==0));
        check(w.npc_pursuit_movement_snapshot(source)->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
        w.tick(2300);
        bool moved=false;for(const auto& p:box->drain())if(p.size()==16&&p[0]==44&&p[1]==0)moved=true;
        check(moved);check(!w.npc_flare_ai_snapshot(source)->engaged);check(!w.npc_energy_snapshot(source));
        check(w.damage_npc(owner,source,{combat::Kind::armor_ignoring,1,20,194},1248,2400).reason==combat::Reason::accepted);
        w.tick(2400);check(w.npc_flare_ai_snapshot(source)->engaged);check(w.npc_ordinary_pending(source).has_value());
        const auto coverage=w.npc_ai_coverage();check(coverage.active_hostiles==1&&coverage.wandering==1);
    }
    std::cout<<"NPC ordinary/elemental wander, proximity combat, damage pull, return and resume passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
