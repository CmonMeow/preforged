// Real quest81 catalog, translated together for deterministic native mechanics.
// Authored melee/patrol policy; no retail cadence or rendered-path claim.
#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Mesmer Test owned ordinary line "+std::to_string(at.line()));
}
std::vector<world::Instance::Npc> roster(){
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
    std::ifstream appearance(root/"actors/mesmer-test-skale.txt");check(bool(appearance));
    auto population=world::load_npc_catalog(appearance);
    const auto path=root/"mesmer-test-development-combat.txt";
    std::ifstream header(path);std::string magic,hash;unsigned version;header>>magic>>version>>hash;
    check(magic=="gw-npc-combat");std::ifstream metadata(path);
    world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
    auto result=population.for_map(146);check(result.size()==4);
    for(auto& n:result){n.x-=4347;n.y-=548;
        check(n.definition.level==0&&n.combat_definition->spawn_health->maximum==10);
        check(n.combat_definition->physical_armor==0);}
    return result;
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope owner,source;
    Fixture(){
        w.join(1,"Tad owner",500,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,40,40},100);owner=*w.effect_scope(1);check(w.physical_armor(owner,0));
        check(spawn(1));source=*w.effect_scope(60000);
    }
    bool spawn(unsigned cycle){return w.spawn_kill_encounter(owner,81,cycle,roster(),[]{return true;},false,{},{},false,0,true);}
    void arm(unsigned cycle,unsigned now){check(w.arm_mesmer_test_melee(owner,cycle,now)==4);check(w.arm_mesmer_test_wander(owner,cycle,now)==4);}
    void ticks(unsigned from,unsigned to){for(unsigned at=from;at<=to;at+=100)w.tick(at);}
};
int main()try{
    {
        Fixture f;f.arm(1,1000);auto stale=f.owner;++stale.generation;
        check(!f.w.arm_mesmer_test_melee(stale,1,1000));check(!f.w.arm_mesmer_test_melee(f.owner,2,1000));
        auto policy=f.w.npc_flare_ai_snapshot(f.source)->policy;
        check(policy.level==0&&!policy.valid()&&policy.valid(true));
        check(!f.w.configure_npc_elemental_ai(policy,{},1,1000));
        check(!f.w.configure_npc_wander(f.source,{{{20,0}},100,2000,"test-only"},1000));
        f.w.join(2,"Observer",100,0,std::make_shared<world::Outbox>(),0,{}, {},1000,1);
        f.w.set_vitals(2,{1000,1000,40,40},100);
        f.ticks(1000,5000);check(!f.w.npc_flare_ai_snapshot(f.source)->engaged);
        check(f.w.resources(1)->health==1000&&f.w.resources(2)->health==1000);
        check(f.w.npc_ai_coverage().wandering==4&&f.w.npc_ai_coverage().placed==4);
        check(f.w.damage_npc(f.owner,f.source,{combat::Kind::armor_ignoring,1,20,194},1248,5100).reason==combat::Reason::accepted);
        bool pending=false,paid=false,moved=false;
        for(unsigned at=5100;at<=15000;at+=100){
            f.w.tick(at);const auto state=f.w.npc_flare_ai_snapshot(f.source);check(state.has_value());
            check(state->policy.target==f.owner);
            const auto movement=f.w.npc_pursuit_movement_snapshot(f.source);
            moved|=movement&&movement->at==at&&movement->outcome.travelled>0;
            if(const auto action=f.w.npc_ordinary_pending(f.source)){
                pending=true;f.arm(1,at);check(f.w.npc_ordinary_pending(f.source)==action);
                check(f.w.npc_combat_snapshot(f.source)->health->current==9);
            }
            if(!paid&&f.w.resources(1)->health<1000){check(f.w.resources(1)->health==995);paid=true;}
        }
        check(moved);check(pending);check(paid);check(f.w.resources(2)->health==1000);
        f.w.move(1,4500,0,15100);bool returning=false;
        for(unsigned at=15100;at<=45000;at+=100){f.w.tick(at);returning|=f.w.npc_flare_ai_snapshot(f.source)->returning;}
        check(returning);const auto home=f.w.npc_flare_ai_snapshot(f.source);
        check(home&&!home->engaged&&!home->returning&&!f.w.npc_ordinary_pending(f.source));
        bool resumed=false;
        for(unsigned at=45100;at<=50000;at+=100){f.w.tick(at);const auto m=f.w.npc_pursuit_movement_snapshot(f.source);
            resumed|=m&&m->at==at&&m->outcome.travelled>0;}
        check(resumed);
        check(f.w.clear_kill_objective(f.owner,81,1,true));check(!f.w.effect_scope(f.source.actor));
        check(!f.w.arm_mesmer_test_melee(f.owner,1,50100));check(f.spawn(2));f.arm(2,50200);
        check(*f.w.effect_scope(60004)!=f.source);check(!f.w.arm_mesmer_test_melee(f.owner,1,50200));
        f.w.leave(1);f.w.tick(50300);
        f.w.join(1,"New owner",500,0,f.box,0,{}, {},50400,1);
        check(*f.w.effect_scope(1)!=f.owner);check(!f.w.arm_mesmer_test_melee(f.owner,2,50400));
    }
    {
        // Damage preceding enrollment remains a trigger; one death completes all four.
        Fixture f;
        check(f.w.damage_npc(f.owner,f.source,{combat::Kind::armor_ignoring,1,20,194},1248,1000).reason==combat::Reason::accepted);
        f.arm(1,1100);f.w.tick(1100);check(f.w.npc_flare_ai_snapshot(f.source)->engaged);
        check(f.w.npc_combat_snapshot(f.source)->health->current==9);
        check(f.w.damage_npc(f.owner,f.source,{combat::Kind::armor_ignoring,10,20,194},1248,1200).reason==combat::Reason::accepted);
        f.w.tick(1200);check(!f.w.arm_mesmer_test_melee(f.owner,1,1300));
        check(!f.w.arm_mesmer_test_wander(f.owner,1,1300));
        for(unsigned id=60000;id<60004;++id){if(const auto scope=f.w.effect_scope(id)){
            check(!f.w.npc_flare_ai_snapshot(*scope));check(!f.w.npc_ordinary_pending(*scope));}}
        check(f.w.npc_ai_coverage().wandering==0);
    }
    std::cout<<"Quest81 catalog Health10, level0 ordinary, retaliation, pull/fight/return, patrol and lifetime passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

