// Real catalog identity, explicit development attack/patrol policy; no retail tuning claim.
#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Rogue Bull owned ordinary line "+std::to_string(at.line()));
}
world::Instance::Npc sample(){
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
    std::ifstream appearance(root/"actors/mesmer-burden-bull.txt");check(bool(appearance));
    auto population=world::load_npc_catalog(appearance);
    const auto path=root/"mesmer-burden-development-combat.txt";
    std::ifstream header(path);std::string magic,hash;unsigned version=0;header>>magic>>version>>hash;
    check(magic=="gw-npc-combat");std::ifstream metadata(path);
    world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
    auto actor=population.for_map(146).front();actor.x=actor.y=0;
    check(actor.definition.level==5&&actor.combat_definition->spawn_health->maximum==80);
    check(actor.combat_definition->physical_armor==120&&actor.combat_definition->earth_armor==120);
    check(!actor.combat_definition->spawn_energy);return actor;
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope owner,source;
    Fixture(float distance=2500){
        w.join(1,"Bull owner",distance,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,0,100},100);owner=*w.effect_scope(1);check(w.physical_armor(owner,0));
        check(w.spawn_kill_encounter(owner,71,1,{sample()},[]{return true;}));source=*w.effect_scope(60000);
        check(w.arm_rogue_bull_melee(owner,1,1000));check(w.arm_rogue_bull_wander(owner,1,1000)==1);
    }
    void ticks(unsigned from,unsigned to){for(unsigned at=from;at<=to;at+=100)w.tick(at);}
    bool moved(){bool result=false;for(const auto& p:box->drain()){
        if(p.size()!=16||p[0]!=44||p[1])continue;
        const auto actor=unsigned(p[2])|(unsigned(p[3])<<8)|(unsigned(p[4])<<16)|(unsigned(p[5])<<24);
        result|=actor==source.actor;
    }return result;}
};
int main()try{
    {
        Fixture f;auto wrong=f.owner;++wrong.generation;
        check(!f.w.arm_rogue_bull_melee(wrong,1,1000));check(!f.w.arm_rogue_bull_melee(f.owner,2,1000));
        check(f.w.arm_rogue_bull_wander(wrong,1,1000)==0);
        f.box->drain();f.ticks(1000,2100);
        check(f.w.arm_rogue_bull_melee(f.owner,1,2100));check(f.w.arm_rogue_bull_wander(f.owner,1,2100)==1);
        f.ticks(2200,3000);check(f.moved());check(!f.w.npc_flare_ai_snapshot(f.source)->engaged);
        check(f.w.resources(1)->health==1000);check(f.w.npc_ai_coverage().placed==1);
        // Owner enters the explicitly authored acquisition range without dealing damage.
        f.w.move(1,500,0,3100);bool engaged=false,paid_hit=false;
        for(unsigned at=3100;at<=15000;at+=100){f.w.tick(at);engaged|=f.w.npc_flare_ai_snapshot(f.source)->engaged;
            paid_hit|=f.w.resources(1)->health<1000;}
        check(engaged&&paid_hit);check(!f.w.npc_energy_snapshot(f.source));
        check(f.w.npc_combat_snapshot(f.source)->health->current==80);
        // Repeated host enrollment must preserve an existing paid windup.
        if(const auto pending=f.w.npc_ordinary_pending(f.source)){
            check(f.w.arm_rogue_bull_melee(f.owner,1,15000));
            check(f.w.npc_ordinary_pending(f.source)==pending);
        }
        f.w.move(1,4500,0,15100);bool returned=false;
        for(unsigned at=15100;at<=45000;at+=100){f.w.tick(at);returned|=f.w.npc_flare_ai_snapshot(f.source)->returning;}
        check(returned);check(std::abs(f.w.position(1,45000).first-4500)<.1f);
        const auto state=*f.w.npc_flare_ai_snapshot(f.source);check(!state.engaged&&!state.returning);
        check(!f.w.npc_ordinary_pending(f.source));f.box->drain();f.ticks(45100,49000);check(f.moved());
        // Existing generic enrollment still rejects quest-owned actors.
        check(!f.w.configure_npc_wander(f.source,{{{20,0}},100,2000,"test-only"},49000));
        check(f.w.clear_kill_objective(f.owner,71,1,true));f.w.tick(49100);
        check(!f.w.effect_scope(f.source.actor));check(!f.w.arm_rogue_bull_melee(f.owner,1,49100));
        check(f.w.spawn_kill_encounter(f.owner,71,2,{sample()},[]{return true;}));
        const auto fresh=*f.w.effect_scope(60001);check(fresh!=f.source);
        check(f.w.arm_rogue_bull_melee(f.owner,2,49200));check(f.w.arm_rogue_bull_wander(f.owner,2,49200)==1);
    }
    {
        Fixture f(100);f.w.tick(1000);const auto pending=f.w.npc_ordinary_pending(f.source);check(pending.has_value());
        check(f.w.arm_rogue_bull_melee(f.owner,1,1000));check(f.w.arm_rogue_bull_wander(f.owner,1,1000)==1);
        check(f.w.npc_ordinary_pending(f.source)==pending); // Idempotence during a live paid windup.
        const auto policy=f.w.npc_flare_ai_snapshot(f.source)->policy;
        combat::ai::ordinary::Reviewed attack{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{100,32},"test-only"};
        using Acquisition=combat::ai::FlareController::OwnedAcquisition;
        check(f.w.configure_npc_owned_ordinary(f.owner,71,1,policy,attack,Acquisition::owner_proximity,1000));
        check(!f.w.configure_npc_owned_ordinary(f.owner,71,2,policy,attack,Acquisition::owner_proximity,1000));
        check(!f.w.configure_npc_owned_ordinary(f.owner,75,1,policy,attack,Acquisition::owner_proximity,1000));
        check(!f.w.configure_npc_owned_ordinary(f.owner,71,1,policy,attack,Acquisition::damage_only,1000));
        auto stale=policy;++stale.source.generation;
        check(!f.w.configure_npc_owned_ordinary(f.owner,71,1,stale,attack,Acquisition::owner_proximity,1000));
        check(!f.w.configure_npc_hostile_ordinary(policy,attack,1000));
        f.w.set_vitals(1,{0,1000,0,100},100);f.ticks(1100,8000);
        check(!f.w.npc_ordinary_pending(f.source));check(!f.w.npc_flare_ai_snapshot(f.source)->engaged);
        const auto old=f.owner;f.w.leave(1);
        f.w.join(1,"New owner",100,0,f.box,0,{}, {},8100,1);f.w.tick(8200);
        check(*f.w.effect_scope(1)!=old);check(!f.w.arm_rogue_bull_melee(old,1,8200));
        check(!f.w.npc_flare_ai_snapshot(f.source));
    }
    std::cout<<"Rogue Bull authored owned ordinary lifecycle passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
