#include "gw_world_simulation.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Pet obstructed pursuit line "+std::to_string(at.line()));
}
std::shared_ptr<const navigation::Mesh> mesh_fixture(bool disconnected){
    const auto path=std::filesystem::temp_directory_path()/("pet-obstructed-pursuit-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".fwnav");
    std::ofstream out(path,std::ios::binary);check(bool(out));
    const auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
    const auto real=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})word(n);
    for(float n:{0.f,0.f,500.f,400.f})real(n);word(1);word(3);
    for(unsigned i=0;i<3;++i){
        word(!disconnected&&i?i-1:0xffffffffu);word(!disconnected&&i<2?i+1:0xffffffffu);
        for(unsigned j=0;j<3;++j)word(0xffffffffu);
        const float left=i==0?0.f:i==1?180.f:220.f,right=i==0?180.f:i==1?220.f:500.f;
        for(float n:{i==1?300.f:0.f,400.f,left,right,left,right})real(n);
    }
    out.close();auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
    std::filesystem::remove(path);return mesh;
}
struct Fixture {
    world::Instance w;std::shared_ptr<const navigation::Mesh> mesh;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope owner,pet,foe;float x=160,y=100;bool moved=false,attacked=false;
    Fixture(bool disconnected,bool clear):mesh(mesh_fixture(disconnected)){
        w.configure_navigation(mesh);world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=5;
        n.definition.visual_adjustment={0,0,0,100};n.x=clear?80.f:240.f;n.y=100;
        combat::npcs::Definition d;d.binding={162,100,100,0,0,0,0,5};
        d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;d.spawn_health=combat::Health{1000,1000};
        d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=0;
        n.combat_definition=d;w.configure_npcs({n});w.join(1,"Pet owner",x,y,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);owner=*w.effect_scope(1);foe=*w.effect_scope(100);
        check(w.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1));check(w.pet_owner_combat(owner,1,0,1000));
        check(w.reconcile_pet_bar_mode(owner,162,true,false,std::array{436u},1000));pet=*w.owned_pet_scope(owner);
        check(w.command_pet(owner,{pets::wire::Command::Kind::target,pet.actor,foe.actor},1000).has_value());box->drain();
    }
    void tick(unsigned at){w.tick(at);attacked|=w.npc_ordinary_pending(pet).has_value();
        for(const auto& p:box->drain()){
            if(p.size()!=16||p[0]!=44||p[1])continue;
            const auto word=[&](unsigned i){return unsigned(p[i])|unsigned(p[i+1])<<8|unsigned(p[i+2])<<16|unsigned(p[i+3])<<24;};
            if(word(2)!=pet.actor)continue;
            const float nx=std::bit_cast<float>(word(6)),ny=std::bit_cast<float>(word(10));
            const unsigned count=std::max(1u,unsigned(std::ceil(std::hypot(nx-x,ny-y)*4)));
            for(unsigned i=0;i<=count;++i){const float t=float(i)/count;
                check(mesh->locate_cell(0,{x+(nx-x)*t,y+(ny-y)*t}).has_value());}
            moved|=std::hypot(nx-x,ny-y)>.01;x=nx;y=ny;
        }
    }
};
int main()try{
    // Same80-unit distance is inside the authored128 melee reach in all cases.
    // Only a connected navigation detour may make the obstructed foe reachable.
    for(bool disconnected:{false,true}){
        Fixture f(disconnected,false);f.tick(1000);check(!f.attacked);
        for(unsigned now=1100;now<=5000;now+=100)f.tick(now);
        check(f.moved==!disconnected);check(f.attacked==!disconnected);
        check(f.w.owned_pet_scope(f.owner)==f.pet);
    }
    Fixture clear(false,true);clear.tick(1000);check(clear.attacked&&!clear.moved);
    std::cout<<"Pet takes connected detour for blocked in-range target; disconnected and clear-contact controls preserved\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
