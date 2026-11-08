// Synthetic non-quest placement; Bull identity/Health evidence does not turn
// authored weapon timing into retail tuning or duplicate the live quest Bull.
#include "gw_world_simulation.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
const char* scenario="setup";
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error(std::string("Pursuit combat priority ")+scenario+" line "+std::to_string(at.line()));
}
std::shared_ptr<const navigation::Mesh> mesh_fixture(bool disconnected){
    const auto path=std::filesystem::temp_directory_path()/("ordinary-population-nav-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".fwnav");
    std::ofstream out(path,std::ios::binary);check(bool(out));
    const auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
    const auto real=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})word(n);
    for(float n:{0.f,0.f,5000.f,400.f})real(n);word(1);word(3);
    for(unsigned i=0;i<3;++i){
        word(!disconnected&&i?i-1:0xffffffffu);word(!disconnected&&i<2?i+1:0xffffffffu);
        for(unsigned j=0;j<3;++j)word(0xffffffffu);
        const float left=i==0?0.f:i==1?180.f:220.f,right=i==0?180.f:i==1?220.f:5000.f;
        for(float n:{i==1?300.f:0.f,400.f,left,right,left,right})real(n);
    }
    out.close();auto result=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
    std::filesystem::remove(path);return result;
}
using Enrollment=world::Instance::NpcPopulationEnrollment;
struct Fixture {
    world::Instance w;std::shared_ptr<const navigation::Mesh> mesh;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    world::Instance::Npc sample;skills::effects::Scope source,target;combat::npcs::Binding binding;
    combat::ai::FlarePolicy policy;combat::ai::ordinary::Reviewed attack;combat::ai::wander::Policy patrol;
    float x=160,y=100;unsigned moves=0,stops=0;bool detoured=false,unavailable=false;
    Fixture(bool disconnected,float distance=2500,bool known=true):mesh(mesh_fixture(disconnected)){
        w.configure_navigation(mesh);auto& n=sample;n.id=n.definition.id=100;n.definition.level=5;
        n.definition.flags=9;n.definition.model_file=116295;n.definition.visual_adjustment={0,0,0,100};n.x=x;n.y=y;
        combat::npcs::Definition d;binding=d.binding={146,100,100,116295,0,0,9,5};
        d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;d.spawn_health=combat::Health{80,80};
        d.physical_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.general_armor=120;
        if(!known)d.physical_armor.reset();n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Melee target",distance,100,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,40,40},100);
        source=*w.effect_scope(100);target=*w.effect_scope(1);check(w.physical_armor(target,0));
        policy.source=source;policy.level=5;policy.speed=220;policy.acquire=1000;policy.follow=1500;policy.leash=2000;
        policy.flight={1000,32};policy.evidence="explicit synthetic ordinary development policy";
        attack={5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},policy.evidence};
        patrol={{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,policy.evidence};box->drain();
    }
    Enrollment arm(unsigned at){return w.configure_npc_population_ordinary(source,binding,policy,attack,patrol,at);}
    void tick(unsigned at){w.tick(at);for(const auto& p:box->drain()){
        if(p.size()<6)continue;
        const auto word=[&](unsigned i){return unsigned(p[i])|(unsigned(p[i+1])<<8)|(unsigned(p[i+2])<<16)|(unsigned(p[i+3])<<24);};
        if(word(2)!=source.actor)continue;
        if(p.size()==6&&p[0]==40&&!p[1]){++stops;continue;}
        if(p.size()!=16||p[0]!=44||p[1])continue;
        const float nx=std::bit_cast<float>(word(6)),ny=std::bit_cast<float>(word(10));
        const unsigned count=std::max(1u,unsigned(std::ceil(std::hypot(nx-x,ny-y)*4)));
        for(unsigned i=0;i<=count;++i){const float t=float(i)/count;check(mesh->locate_cell(0,{x+(nx-x)*t,y+(ny-y)*t}).has_value());}
        x=nx;y=ny;++moves;detoured|=y>=299;
    }const auto result=w.npc_pursuit_movement_snapshot(source);
        unavailable|=result&&result->outcome.kind==combat::ai::motion::OutcomeKind::route_unavailable;
    }
};
int main()try{
    for(const bool coarse:{false,true}){
        scenario=coarse?"coarse target approaches":"fine target approaches";
        Fixture f(false,800);check(f.arm(1000)==Enrollment::armed);
        for(unsigned at=1000;at<=3000;at+=100)f.tick(at);
        check(f.moves>0&&f.w.npc_flare_ai_snapshot(f.source)->engaged);
        check(!f.w.npc_ordinary_pending(f.source));
        const auto before=f.w.resources(1)->health;
        // The original foe approaches the pursuing NPC over valid observed
        // terrain. The old route points toward the foe's former position.
        f.w.move(1,f.x,f.y,3000);
        unsigned began=0;
        for(unsigned at=3100;at<=5000;at+=100){f.tick(at);
            if(f.w.npc_ordinary_pending(f.source)){began=at;break;}
        }
        check(began!=0);
        const auto stopped_x=f.x,stopped_y=f.y;
        // End the player's approach at this actual combat decision pose. The
        // older destination could otherwise carry it past melee reach again.
        f.w.move(1,stopped_x,stopped_y,began);
        bool hit=false;
        for(unsigned at=coarse?began+1900:began+100;at<=began+1900;at+=100){f.tick(at);
            hit|=f.w.resources(1)->health<before;
            if(std::hypot(f.x-stopped_x,f.y-stopped_y)>=0.001)
                std::cerr<<scenario<<" began="<<began<<" at="<<at<<" from="<<stopped_x<<','<<stopped_y
                         <<" to="<<f.x<<','<<f.y<<" stops="<<f.stops<<'\n';
            check(std::hypot(f.x-stopped_x,f.y-stopped_y)<0.001);
        }
        check(hit);check(f.stops==1);
        // Standing combat must not disable a later legitimate chase.
        const auto stationary_moves=f.moves;
        f.w.move(1,800,100,began+2000);
        bool resumed=false;
        for(unsigned at=began+2000;at<=began+5000;at+=100){f.tick(at);
            resumed|=std::hypot(f.x-stopped_x,f.y-stopped_y)>1;
        }
        check(resumed&&f.moves>stationary_moves);
    }
    std::cout<<"Obsolete pursuit route stops at in-range combat decision\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
