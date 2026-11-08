// Explicit synthetic navigation and authored Firestarter combat fixture.
#include "gw_world_simulation.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Population caster navigation line "+std::to_string(at.line()));
}
std::shared_ptr<const navigation::Mesh> mesh_fixture(bool disconnected){
    const auto path=std::filesystem::temp_directory_path()/("population-nav-"+
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
    skills::effects::Scope source,target;combat::npcs::Binding binding;
    combat::ai::FlarePolicy policy;combat::ai::wander::Policy patrol;
    float x=160,y=100;unsigned moves=0;bool detoured=false,unavailable=false;
    Fixture(bool disconnected,bool known=true):mesh(mesh_fixture(disconnected)){
        w.configure_navigation(mesh);world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;
        n.definition.flags=12;n.definition.model_file=116226;n.model_files={116642};
        n.definition.visual_adjustment={0,0,0,100};n.x=x;n.y=y;
        combat::npcs::Definition d;binding=d.binding={146,100,100,116226,0,0,12,1};
        d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=d.conditions_allowed=true;
        d.spawn_health=combat::Health{100,100};d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=4;
        d.physical_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.general_armor=0;
        if(!known)d.energy_pips.reset();n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Patrol target",2500,100,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,40,40},100);
        source=*w.effect_scope(100);target=*w.effect_scope(1);check(w.elemental_armor(target,0,0));
        policy.source=source;policy.level=1;policy.speed=220;policy.acquire=1000;policy.follow=1500;policy.leash=2000;
        policy.flight={1000,32};policy.max_flights=4;policy.evidence="explicit Firestarter development test policy";
        patrol={{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,policy.evidence};
        box->drain();
    }
    Enrollment arm(unsigned now){return w.configure_npc_population_elemental(source,binding,policy,
        {{{194,0},*skills::effects::elemental(194,0),policy.evidence}},patrol,now);}
    void drain(){for(const auto& p:box->drain()){
        if(p.size()!=16||p[0]!=44||p[1])continue;
        const auto word=[&](unsigned i){return unsigned(p[i])|(unsigned(p[i+1])<<8)|(unsigned(p[i+2])<<16)|(unsigned(p[i+3])<<24);};
        if(word(2)!=source.actor)continue;
        const float nx=std::bit_cast<float>(word(6)),ny=std::bit_cast<float>(word(10));
        const unsigned count=std::max(1u,unsigned(std::ceil(std::hypot(nx-x,ny-y)*4)));
        for(unsigned i=0;i<=count;++i){const float t=float(i)/count;check(mesh->locate_cell(0,{x+(nx-x)*t,y+(ny-y)*t}).has_value());}
        x=nx;y=ny;++moves;detoured|=y>=299;
    }}
    void tick(unsigned at){w.tick(at);drain();const auto result=w.npc_pursuit_movement_snapshot(source);
        unavailable|=result&&result->outcome.kind==combat::ai::motion::OutcomeKind::route_unavailable;}
};
int main()try{
    {Fixture f(true);check(f.arm(1000)==Enrollment::armed);f.tick(2200);
        check(f.unavailable&&f.moves==0); // First waypoint is on a disconnected island.
        f.tick(3300);check(f.moves==0); // Authored pause still applies after failure.
        f.tick(3400);check(f.moves==0); // Next reachable leg commits only future travel.
        f.tick(3500);check(f.moves>0&&f.y>100&&f.x<=180);
        check(!f.w.npc_elemental_ai_snapshot(f.source)->engaged&&f.w.resources(1)->health==1000);
    }
    {Fixture f(false,false);check(f.arm(1000)==Enrollment::unknown_combat);
        check(!f.w.npc_elemental_ai_snapshot(f.source)&&f.w.npc_ai_coverage().wandering==0);}
    for(bool disconnected:{false,true}){
        Fixture f(disconnected);auto stale=f.source;++stale.generation;auto p=f.policy;p.source=stale;
        check(f.w.configure_npc_population_elemental(stale,f.binding,p,{{{194,0},*skills::effects::elemental(194,0),p.evidence}},f.patrol,1000)==Enrollment::identity_mismatch);
        check(f.arm(1000)==Enrollment::armed);check(f.arm(1000)==Enrollment::already_armed);
        for(unsigned at=1000;at<=10000;at+=100){f.tick(at);if(at==2000)check(f.arm(at)==Enrollment::already_armed);}
        check(!f.w.npc_elemental_ai_snapshot(f.source)->engaged&&f.w.resources(1)->health==1000);
        check(f.moves>0);if(disconnected)check(f.unavailable&&f.x<=180);else check(f.detoured);
        f.w.move(1,800,100,10100);bool engaged=false,paid=false;
        for(unsigned at=10100;at<=26000;at+=100){f.tick(at);const auto ai=*f.w.npc_elemental_ai_snapshot(f.source);
            engaged|=ai.engaged;if(ai.pending){const auto energy=f.w.npc_energy_snapshot(f.source)->current;
                check(f.arm(at)==Enrollment::already_armed);check(f.w.npc_elemental_ai_snapshot(f.source)->pending==ai.pending);
                // Same skill/copy/range/evidence must not hide a changed effect rank.
                check(f.w.configure_npc_population_elemental(f.source,f.binding,f.policy,
                    {{{194,0},*skills::effects::elemental(194,12),f.policy.evidence}},f.patrol,at)==Enrollment::conflicting_controller);
                check(f.w.npc_elemental_ai_snapshot(f.source)->pending==ai.pending);
                check(f.w.npc_energy_snapshot(f.source)->current==energy);}
            paid|=f.w.resources(1)->health<1000;
        }
        check(engaged);check(paid!=disconnected);
        if(disconnected){check(f.x<=180&&f.unavailable);check(f.w.kill(f.target,26100));}
        else f.w.move(1,4500,100,26100);
        bool returning=false,home=false;
        for(unsigned at=26100;at<=58000;at+=100){f.tick(at);const auto ai=*f.w.npc_elemental_ai_snapshot(f.source);
            returning|=ai.returning;home|=returning&&!ai.returning&&!ai.engaged;}
        check(returning&&home);const auto before=f.moves;
        for(unsigned at=58100;at<=66000;at+=100)f.tick(at);check(f.moves>before);
        check(!f.w.npc_elemental_ai_snapshot(f.source)->engaged);
        // Same numeric player ID cannot inherit a former target's paid cast.
        f.w.leave(1);f.w.join(1,"Fresh target",f.x,f.y,f.box,0,{}, {},66100,1);
        const auto fresh=*f.w.effect_scope(1);check(fresh!=f.target);f.w.set_vitals(1,{1000,1000,40,40},100);
        check(f.w.elemental_armor(fresh,0,0));bool pending=false;
        for(unsigned at=66100;at<=70000;at+=100){f.tick(at);const auto ai=*f.w.npc_elemental_ai_snapshot(f.source);
            if(ai.pending){check(ai.pending->use.target==fresh);pending=true;break;}}
        check(pending);const auto health=f.w.resources(1)->health;f.w.leave(1);
        f.w.join(1,"Replacement",4500,100,f.box,0,{}, {},70100,1);f.w.set_vitals(1,{1000,1000,40,40},100);
        for(unsigned at=70100;at<=73000;at+=100)f.tick(at);
        check(f.w.resources(1)->health==1000);check(!f.w.npc_elemental_ai_snapshot(f.source)->pending);
        (void)health;
    }
    std::cout<<"Population caster enrollment, nav patrol/pull/fight/return and target lifetime passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
