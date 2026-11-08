// Existing reviewed Brood roster; authored idle policy, not retail path evidence.
#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Brood idle wander line "+std::to_string(at.line()));
}
int main()try{
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
    std::ifstream appearance(root/"actors/domination-magic-brood.txt");check(bool(appearance));
    auto population=world::load_npc_catalog(appearance);
    const auto path=root/"domination-magic-development-combat.txt";
    std::ifstream header(path);std::string magic,hash;unsigned version=0;header>>magic>>version>>hash;
    check(magic=="gw-npc-combat"&&version>0);std::ifstream metadata(path);
    world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
    auto samples=population.for_map(161);check(!samples.empty());auto sample=samples.front();sample.x=sample.y=0;
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    w.join(1,"Brood owner",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,40,40},100);
    const auto owner=*w.effect_scope(1);check(w.elemental_armor(owner,60,60));check(w.physical_armor(owner,0));
    check(w.spawn_kill_encounter(owner,49,1,{sample},[]{return true;}));const auto source=*w.effect_scope(60000);
    check(w.arm_brood_wander(owner,1,1000)==0); // Only an existing reviewed controller.
    check(w.arm_brood_caster(owner,1,1000));
    auto wrong=owner;++wrong.generation;
    check(w.arm_brood_wander(wrong,1,1000)==0);check(w.arm_brood_wander(owner,2,1000)==0);
    check(w.arm_brood_wander(owner,1,1000)==1);
    const auto movement=[&]{bool moved=false;for(const auto& p:box->drain()){
        if(p.size()!=16||p[0]!=44||p[1]!=0)continue;
        const unsigned actor=unsigned(p[2])|(unsigned(p[3])<<8)|(unsigned(p[4])<<16)|(unsigned(p[5])<<24);
        if(actor==source.actor)moved=true;
    }return moved;};
    box->drain();w.tick(1000);check(!movement());
    check(w.arm_brood_wander(owner,1,2000)==1); // Must not restart pause.
    w.tick(2200);check(!movement());
    check(w.npc_pursuit_movement_snapshot(source)->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
    w.tick(2300);check(movement());
    for(unsigned at=2400;at<=5000;at+=100)w.tick(at);
    auto state=*w.npc_elemental_ai_snapshot(source);
    check(!state.engaged&&!state.returning&&!state.pending&&state.flights==0);
    check(w.resources(1)->health==1000);check(!w.npc_aura_snapshot(source,5000));
    check(w.npc_energy_snapshot(source)->current==40);
    check(w.npc_ai_coverage().wandering==1);
    check(w.damage_npc(owner,source,{combat::Kind::armor_ignoring,1,20,194},1248,5100).reason==combat::Reason::accepted);
    w.tick(5100);check(w.npc_elemental_ai_snapshot(source)->engaged);
    bool fought=false;
    for(unsigned at=5200;at<=20000;at+=100){w.tick(at);if(w.resources(1)->health<1000){fought=true;break;}}
    check(fought);check(w.npc_aura_snapshot(source,20000).has_value());
    w.move(1,5000,0,20100);
    unsigned started=0,finished=0;
    for(unsigned at=20100;at<=50000;at+=100){w.tick(at);const auto ai=w.npc_elemental_ai_snapshot(source);check(ai.has_value());
        if(ai->returning)started=at;
        if(started&&!ai->returning&&!ai->engaged){finished=at;break;}
    }
    check(started&&finished);box->drain();bool resumed=false;
    for(unsigned at=finished+100;at<=finished+5000;at+=100){w.tick(at);resumed=movement()||resumed;
        const auto ai=*w.npc_elemental_ai_snapshot(source);check(!ai.engaged&&!ai.returning&&!ai.pending);
    }
    check(resumed);check(w.npc_ai_coverage().wandering==1);
    w.leave(1);w.tick(finished+5100);
    check(w.arm_brood_wander(owner,1,finished+5100)==0);
    check(w.npc_ai_coverage().wandering==0);
    w.join(1,"New owner",100,0,std::make_shared<world::Outbox>(),0,{}, {},finished+5200,1);
    const auto replacement=*w.effect_scope(1);check(replacement!=owner);
    check(w.arm_brood_wander(owner,1,finished+5200)==0);
    check(w.arm_brood_wander(replacement,1,finished+5200)==0);
    std::cout<<"Existing owned Brood bounded idle, damage fight, return and resume\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
