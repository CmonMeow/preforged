#include "gw_world_simulation.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC pursuit outcome World line "+std::to_string(at.line()));
}
std::shared_ptr<const navigation::Mesh> fixture_mesh(bool disconnected){
    const auto nonce=std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path=std::filesystem::temp_directory_path()/("npc-pursuit-outcome-"+std::to_string(nonce)+".fwnav");
    std::ofstream out(path,std::ios::binary|std::ios::trunc);check(bool(out));
    const auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
    const auto real=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xEEFE704cu,12u,1u,1u})word(n);
    for(float n:{0.f,0.f,1000.f,400.f})real(n);word(1);word(disconnected?2:1);
    for(unsigned i=0;i<(disconnected?2u:1u);++i){
        for(unsigned j=0;j<5;++j)word(0xffffffffu);
        const float left=i?220.f:0.f,right=disconnected&&!i?200.f:1000.f;
        for(float n:{0.f,400.f,left,right,left,right})real(n);
    }
    out.close();auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
    std::filesystem::remove(path);return mesh;
}
struct Fixture {
    world::Instance w;skills::effects::Scope source;
    Fixture(bool elemental,bool disconnected){
        w.configure_navigation(fixture_mesh(disconnected));
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};n.x=n.y=100;
        combat::npcs::Definition d;d.binding.map=160;d.binding.actor=d.binding.definition=100;d.binding.level=1;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{80,80};d.damageable=true;
        if(elemental){d.spawn_energy=combat::npcs::Energy{0,30};d.energy_pips=0;}
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Target",400,100,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
        const auto target=*w.effect_scope(1);source=*w.effect_scope(100);check(w.physical_armor(target,0));check(w.elemental_armor(target,0,0));
        combat::ai::FlarePolicy p;p.source=source;p.level=1;p.speed=220;p.acquire=1000;p.follow=1500;p.leash=2000;
        p.flight={1000,32};p.evidence="synthetic disconnected/connected navigation outcome fixture";
        combat::ai::ordinary::Reviewed attack{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};
        if(elemental){check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,0),p.evidence}},4,1000));
            check(w.configure_npc_ordinary_ai(source,attack,1000));}
        else check(w.configure_npc_hostile_ordinary(p,attack,1000));
        check(!w.npc_pursuit_movement_snapshot(source));
    }
};
int main()try{
    using Kind=combat::ai::motion::OutcomeKind;using Deferral=combat::ai::motion::Deferral;
    for(bool elemental:{false,true}){
        {Fixture f(elemental,true);f.w.tick(1000);const auto blocked=f.w.npc_pursuit_movement_snapshot(f.source);
            check(blocked&&blocked->at==1000&&blocked->outcome.kind==Kind::route_unavailable&&blocked->outcome.travelled==0);
            f.w.tick(1100);check(f.w.npc_pursuit_movement_snapshot(f.source)->outcome.kind==Kind::route_unavailable);
            check(!f.w.npc_ordinary_pending(f.source)&&f.w.resources(1)->health==1000);
            // Diagnostics do not invent a disengagement timeout or bypass the wall.
            check(elemental?f.w.npc_elemental_ai_snapshot(f.source)->engaged:f.w.npc_flare_ai_snapshot(f.source)->engaged);}
        {Fixture f(elemental,false);f.w.tick(1000);const auto zero=f.w.npc_pursuit_movement_snapshot(f.source);
            check(zero&&zero->outcome.kind==Kind::deferred&&zero->outcome.deferral==Deferral::zero_budget);
            f.w.tick(1100);const auto moving=f.w.npc_pursuit_movement_snapshot(f.source);
            check(moving&&moving->at==1100&&moving->outcome.kind==Kind::progressing&&moving->outcome.travelled>0);
            f.w.tick(2100);const auto reached=f.w.npc_pursuit_movement_snapshot(f.source);
            check(reached&&reached->outcome.kind==Kind::reached&&bool(reached->outcome));
            auto wrong=f.source;++wrong.generation;check(!f.w.npc_pursuit_movement_snapshot(wrong));}
    }
    std::cout<<"NPC pursuit diagnostics distinguish disconnected routes, deferred budget and progress\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
