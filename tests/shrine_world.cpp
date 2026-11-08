#include "gw_world_simulation.h"
#include <fstream>
#include <filesystem>
#include <array>
#include <bit>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("World shrine line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope a,b,pet_a,pet_b;
    Fixture(std::shared_ptr<const navigation::Mesh> mesh,unsigned mode){
        w.configure_navigation(mesh);const shrine::Evidence source{shrine::Basis::fixture,"synthetic World shrine geometry; no real catalog coordinates"};
        shrine::Site site{1,{150,150,0,0},source,{}};
        if(mode!=2)site.activation=mode==1?shrine::Activation{shrine::ActivationKind::entry,{},0,"known-entry",source}:
            shrine::Activation{shrine::ActivationKind::approach,shrine::Place{0,0,0,0},10,"",source};
        world::Instance::ShrinePolicy policy{100,source,{shrine::Basis::authored,"synthetic nearest last-fallen member; tie bysiteID"},true};
        check(w.configure_shrine_sites(42,mesh->asset_hash(),std::array{site},policy));
        std::vector<world::Instance::Npc> enemies;for(unsigned id:{100u,101u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=5;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding={42,id,id,0,0,0,0,5};d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=0;n.combat_definition=d;enemies.push_back(n);
        }
        w.configure_npcs(enemies);
        for(unsigned id:{1u,2u}){w.join(id,id==1?"First Owner":"Second Owner",float(id-1)*5,0,box,0,{}, {},100,1);
            // Explicit fixture armor: innate2 + armor2 =4 Energy pips.
            w.set_vitals(id,{100,100,id==1?12u:7u,30},100);check(w.energy_capacity(id,1,0,100,{10,2}));}
        a=*w.effect_scope(1);b=*w.effect_scope(2);check(w.party_action(1,158,2,200)&&w.party_action(2,154,1,200));
        for(const auto owner:{a,b}){check(w.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1)&&w.pet_owner_combat(owner,1,0,200));
            check(w.reconcile_pet_bar_mode(owner,42,true,false,std::array{436u},200));}
        pet_a=*w.owned_pet_scope(a);pet_b=*w.owned_pet_scope(b);
        combat::ai::ordinary::Reviewed fatal{10000,10000,0,2,false,combat::ai::ordinary::Delivery::melee,100,2000,128,{1000,32},"synthetic pet damage"};
        auto minor=fatal;minor.minimum=minor.maximum=10;
        const auto x=*w.effect_scope(100),y=*w.effect_scope(101);
        check(w.configure_npc_opposed_ordinary(x,fatal,300)&&w.configure_npc_opposed_ordinary(y,minor,300));
        check(w.begin_npc_opposed_ordinary(x,pet_a,300)&&w.begin_npc_opposed_ordinary(y,pet_b,300));
        w.tick(500);check(w.npc_combat_snapshot(pet_a)->health->current==0&&w.npc_combat_snapshot(pet_b)->health->current<180);
        w.tick(900);box->drain();
    }
    void wipe(){check(w.kill(a,1000));check(!w.pending_shrine_wipe(a));check(w.kill(b,1100));const auto pending=w.pending_shrine_wipe(a);check(pending&&pending->began==1100&&pending->due==11100);}
};
int main(int argc,char**argv)try{
    if(argc!=2)throw std::runtime_error("build nav fixture path required");const std::filesystem::path path=argv[1];
    {std::ofstream out(path,std::ios::binary);auto u32=[&](unsigned n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};auto f32=[&](float f){u32(std::bit_cast<unsigned>(f));};
        out.write("FWNAV001",8);for(unsigned n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);f32(-200);f32(-200);f32(200);f32(200);u32(1);u32(1);
        for(unsigned i=0;i<5;++i)u32(0xffffffff);f32(-200);f32(200);f32(-200);f32(200);f32(-200);f32(200);}
    auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
    {
        Fixture f(mesh,0);f.wipe();f.w.tick(11099);check(f.w.resources(1)->health==0&&f.w.resources(2)->health==0);
        const auto live_health=f.w.npc_combat_snapshot(f.pet_b)->health->current;f.w.tick(11100);
        if(f.w.resources(1)->health!=100||f.w.resources(2)->health!=100){
            const auto pending=f.w.pending_shrine_wipe(f.a);
            const auto notices=f.w.take_shrine_notices();const auto recoveries=f.w.take_shrine_recoveries();
            std::cerr<<"shrine diagnostic health="<<f.w.resources(1)->health<<","<<f.w.resources(2)->health
                <<" pending="<<bool(pending)<<" notices="<<notices.size()<<" recoveries="<<recoveries.size()<<'\n';
            if(pending)std::cerr<<"shrine pending serial="<<pending->serial<<" began="<<pending->began<<" due="<<pending->due<<'\n';
            for(const auto& notice:notices)std::cerr<<"shrine rejected issue="<<unsigned(notice.issue)<<" serial="<<notice.wipe.serial<<'\n';
        }
        check(f.w.resources(1)->health==100&&f.w.resources(2)->health==100);
        check(f.w.resources(1)->energy==13&&f.w.resources(2)->energy==8);
        const auto energy_a=f.w.shrine_current_energy_snapshot(f.a),energy_b=f.w.shrine_current_energy_snapshot(f.b);check(energy_a&&energy_b);
        const auto precise=[](const skills::effects::MindWrackEnergy& energy){return (std::int64_t(energy.current)-energy.debt)*overcast::energy_unit+energy.fractional_units;};
        if(precise(*energy_a)!=3960000||precise(*energy_b)!=2500000){
            std::cerr<<"shrine restored precision="<<precise(*energy_a)<<","<<precise(*energy_b)<<'\n';
            for(const auto owner:{f.a,f.b})if(const auto death=f.w.shrine_death_energy_snapshot(owner))
                std::cerr<<"shrine death actor="<<owner.actor<<" current="<<death->current<<" maximum="<<death->maximum
                    <<" debt="<<death->debt<<" fraction="<<death->fractional_units<<'\n';
        }
        check(precise(*energy_a)==3960000&&precise(*energy_b)==2500000);
        check(f.w.position(1,11100)==std::pair(150.f,150.f)&&f.w.position(2,11100)==std::pair(150.f,150.f));
        check(f.w.owned_pet_scope(f.a)==f.pet_a&&f.w.owned_pet_scope(f.b)==f.pet_b);
        check(f.w.npc_combat_snapshot(f.pet_a)->health->current==180&&f.w.npc_combat_snapshot(f.pet_b)->health->current==live_health);
        const auto recovered=f.w.take_shrine_recoveries();check(recovered.size()==1&&recovered[0].pets.size()==2);
        check(f.w.shrine_protection_active(f.a,11100)&&!f.w.shrine_protection_active(f.a,16100)); // Marker only; protection consumer is a separate unfinished capability.
        f.w.tick(11101);check(f.w.take_shrine_recoveries().empty()&&!f.w.pending_shrine_wipe(f.a));
    }
    for(unsigned mode:{1u,2u}){Fixture f(mesh,mode);f.wipe();f.w.tick(12000);check(f.w.resources(1)->health==0&&f.w.resources(2)->health==0);
        const auto notices=f.w.take_shrine_notices();check(notices.size()==1&&notices[0].issue==(mode==1?world::Instance::ShrineIssue::outpost_required:world::Instance::ShrineIssue::activation_unreviewed));
        check(f.w.pending_shrine_wipe(f.a).has_value()&&f.w.take_shrine_recoveries().empty());f.w.tick(13000);check(f.w.take_shrine_notices().empty());}
    {
        Fixture f(mesh,0);f.wipe();check(f.w.resurrect(f.a,5000));f.w.tick(12000);
        check(f.w.resources(1)->health==100&&f.w.resources(2)->health==0&&f.w.npc_combat_snapshot(f.pet_a)->health->current==0);
        check(f.w.take_shrine_recoveries().empty()&&!f.w.pending_shrine_wipe(f.a));
    }
    {
        Fixture f(mesh,0);f.wipe();f.w.tick(5000);const auto old=*f.w.pending_shrine_wipe(f.a);f.w.leave(2);
        const auto next=f.w.pending_shrine_wipe(f.a);check(next&&next->party.epoch!=old.party.epoch&&next->due==15000);
        f.w.tick(11100);check(f.w.resources(1)->health==0);f.w.tick(15000);check(f.w.resources(1)->health==100&&f.w.owned_pet_scope(f.a)==f.pet_a);
    }
    std::filesystem::remove(path);
    std::cout<<"Actual World wipe producer: activated site,10sec, exact deathEnergy, pet recovery, cancellation and typed missing-site fallback passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
