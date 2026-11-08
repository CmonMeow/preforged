// Synthetic non-quest placement; Bull identity/Health evidence does not turn
// authored weapon timing into retail tuning or duplicate the live quest Bull.
#include "gw_world_simulation.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Population ordinary navigation line "+std::to_string(at.line()));
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
        if(p.size()==6&&p[0]==40&&!p[1]&&p[2]==source.actor&&!p[3]&&!p[4]&&!p[5])++stops;
        if(p.size()!=16||p[0]!=44||p[1])continue;
        const auto word=[&](unsigned i){return unsigned(p[i])|(unsigned(p[i+1])<<8)|(unsigned(p[i+2])<<16)|(unsigned(p[i+3])<<24);};
        if(word(2)!=source.actor)continue;const float nx=std::bit_cast<float>(word(6)),ny=std::bit_cast<float>(word(10));
        const unsigned count=std::max(1u,unsigned(std::ceil(std::hypot(nx-x,ny-y)*4)));
        for(unsigned i=0;i<=count;++i){const float t=float(i)/count;check(mesh->locate_cell(0,{x+(nx-x)*t,y+(ny-y)*t}).has_value());}
        x=nx;y=ny;++moves;detoured|=y>=299;
    }const auto result=w.npc_pursuit_movement_snapshot(source);
        unavailable|=result&&result->outcome.kind==combat::ai::motion::OutcomeKind::route_unavailable;
    }
};
int main()try{
    {Fixture f(true,160);f.w.move(1,160,380,100);check(f.arm(1000)==Enrollment::armed);
        f.tick(1000);f.tick(1100);check(f.y>100&&f.stops==0);
        // A new lifetime across the wall replaces the chase destination. The
        // old route must stop on the wire even though no replacement exists.
        f.w.leave(1);f.w.join(1,"Across wall",240,100,f.box,0,{}, {},1200,1);
        f.target=*f.w.effect_scope(1);f.w.set_vitals(1,{1000,1000,40,40},100);
        check(f.w.physical_armor(f.target,0));f.box->drain();f.tick(1200);
        check(f.unavailable&&f.stops==1&&!f.w.npc_ordinary_pending(f.source));
        const auto x=f.x,y=f.y;f.tick(1300);
        check(f.x==x&&f.y==y&&f.stops==1&&f.w.resources(1)->health==1000);
    }
    {Fixture f(true);check(f.arm(1000)==Enrollment::armed);f.tick(2200);
        check(f.unavailable&&f.moves==0); // First waypoint is on a disconnected island.
        f.tick(3300);check(f.moves==0); // Authored pause still applies after failure.
        f.tick(3400);check(f.moves==0); // Next reachable leg commits only future travel.
        f.tick(3500);check(f.moves>0&&f.y>100&&f.x<=180);
        check(!f.w.npc_flare_ai_snapshot(f.source)->engaged&&f.w.resources(1)->health==1000);
    }
    {Fixture f(false,2500,false);check(f.arm(1000)==Enrollment::unknown_combat);check(f.w.npc_ai_coverage().wandering==0);}
    {Fixture f(false,100);check(f.arm(1000)==Enrollment::armed);f.tick(1000);
        const auto pending=f.w.npc_ordinary_pending(f.source);check(pending.has_value());
        check(f.arm(1000)==Enrollment::already_armed);check(!f.w.npc_energy_snapshot(f.source));
        // Each independently meaningful profile field must conflict instead of
        // silently resetting a paid attack, changing damage, or being ignored.
        for(unsigned variant=0;variant<11;++variant){auto a=f.attack;
            switch(variant){
                case 0:a.minimum=4;break;case 1:a.maximum=6;break;case 2:a.mastery=1;break;
                case 3:a.damage_type=1;break;case 4:a.caster_weapon=true;break;case 5:a.windup_ms=800;break;
                case 6:a.period_ms=2100;break;case 7:a.range=120;break;case 8:a.flight.speed=1200;break;
                case 9:a.flight.hit_radius=40;break;case 10:a.evidence+=" changed";break;
            }
            check(f.w.configure_npc_population_ordinary(f.source,f.binding,f.policy,a,f.patrol,1000)==Enrollment::conflicting_controller);
            check(f.w.npc_ordinary_pending(f.source)==pending&&!f.w.npc_energy_snapshot(f.source));
        }
        auto ranged=f.attack;ranged.delivery=combat::ai::ordinary::Delivery::ranged;
        check(f.w.configure_npc_population_ordinary(f.source,f.binding,f.policy,ranged,f.patrol,1000)==Enrollment::invalid_policy);
        auto p=f.policy;p.acquire=900;auto patrol=f.patrol;patrol.pause_ms=1300;
        check(f.w.configure_npc_population_ordinary(f.source,f.binding,p,f.attack,f.patrol,1000)==Enrollment::conflicting_controller);
        check(f.w.configure_npc_population_ordinary(f.source,f.binding,f.policy,f.attack,patrol,1000)==Enrollment::conflicting_controller);
        check(f.w.npc_ordinary_pending(f.source)==pending);
    }
    {Fixture f(false,100);check(f.arm(1000)==Enrollment::armed);f.tick(1000);check(f.w.npc_ordinary_pending(f.source).has_value());
        check(f.w.damage_npc(f.target,f.source,{combat::Kind::armor_ignoring,1000,20,194},1248,1100).killed);
        const auto before=f.moves;for(unsigned at=1200;at<=5000;at+=100)f.tick(at);
        check(!f.w.npc_ordinary_pending(f.source)&&f.w.resources(1)->health==1000&&f.moves==before);
        check(f.w.npc_ai_coverage().wandering==0);check(f.arm(5000)==Enrollment::unknown_combat);
        // Population replacement is supported only after all players leave.
        // Preserve both old scopes so neither can be accepted after rebuilding.
        const auto old=f.source,old_target=f.target;f.w.leave(1);
        f.w.configure_npcs({f.sample});const auto fresh=*f.w.effect_scope(old.actor);check(fresh!=old);
        f.w.join(1,"Rejoined melee",100,100,f.box,0,{}, {},5100,1);
        f.target=*f.w.effect_scope(1);check(f.target!=old_target);
        f.w.set_vitals(1,{1000,1000,40,40},100);check(f.w.physical_armor(f.target,0));
        check(f.arm(5100)==Enrollment::identity_mismatch);
        f.source=f.policy.source=fresh;check(f.arm(5100)==Enrollment::armed);check(!f.w.npc_ordinary_pending(fresh));
        check(f.w.resources(1)->health==1000);
    }
    {Fixture f(false);f.policy.speed=80; // Authored slow foe makes follow/acquire distinction observable.
        check(f.arm(1000)==Enrollment::armed);
        for(unsigned at=1000;at<=10000;at+=100)f.tick(at);
        check(f.moves>0&&!f.w.npc_flare_ai_snapshot(f.source)->engaged);
        f.w.move(1,800,100,10100);
        for(unsigned at=10100;at<=26000;at+=100)f.tick(at);
        check(f.w.resources(1)->health<1000&&f.w.npc_flare_ai_snapshot(f.source)->engaged);
        // Player movement pulls the same encounter beyond acquire, while
        // follow distance still retains it. No teleporting the target fixture.
        f.w.move(1,2100,100,26100);bool retained_outside_acquire=false;
        for(unsigned at=26100;at<=34000;at+=100){f.tick(at);
            const auto target=f.w.position(1,at);const auto ai=*f.w.npc_flare_ai_snapshot(f.source);
            retained_outside_acquire|=ai.engaged&&std::hypot(target.first-f.x,target.second-f.y)>f.policy.acquire;
        }
        check(retained_outside_acquire&&f.x>1200);
        const auto before=f.w.resources(1)->health;
        for(unsigned at=34100;at<=44000;at+=100)f.tick(at);
        check(f.w.resources(1)->health<before); // Fight at the pulled location.
        f.w.move(1,3200,100,44100);bool returning=false,home=false,leash_reached=false;
        for(unsigned at=44100;at<=76000;at+=100){f.tick(at);
            const auto distance=std::hypot(f.x-160,f.y-100);
            check(distance<=f.policy.leash+.01f);leash_reached|=distance>=f.policy.leash-10;
            const auto ai=*f.w.npc_flare_ai_snapshot(f.source);
            returning|=ai.returning;home|=returning&&!ai.returning&&!ai.engaged;
        }
        check(leash_reached&&returning&&home&&!f.w.npc_ordinary_pending(f.source));
        const auto moves=f.moves;
        for(unsigned at=76100;at<=84000;at+=100)f.tick(at);
        check(f.moves>moves&&!f.w.npc_flare_ai_snapshot(f.source)->engaged);
    }
    for(bool disconnected:{false,true}){
        Fixture f(disconnected);auto stale=f.source;++stale.generation;auto p=f.policy;p.source=stale;
        check(f.w.configure_npc_population_ordinary(stale,f.binding,p,f.attack,f.patrol,1000)==Enrollment::identity_mismatch);
        check(f.arm(1000)==Enrollment::armed);
        for(unsigned at=1000;at<=10000;at+=100){f.tick(at);if(at==2000)check(f.arm(at)==Enrollment::already_armed);}
        check(!f.w.npc_flare_ai_snapshot(f.source)->engaged&&f.w.resources(1)->health==1000&&f.moves>0);
        if(disconnected)check(f.unavailable&&f.x<=180);else check(f.detoured);
        f.w.move(1,800,100,10100);bool engaged=false,hit=false;
        for(unsigned at=10100;at<=26000;at+=100){f.tick(at);engaged|=f.w.npc_flare_ai_snapshot(f.source)->engaged;
            if(const auto pending=f.w.npc_ordinary_pending(f.source)){check(f.arm(at)==Enrollment::already_armed);check(f.w.npc_ordinary_pending(f.source)==pending);}
            hit|=f.w.resources(1)->health<1000;}
        check(engaged&&hit!=disconnected);check(!f.w.npc_energy_snapshot(f.source));
        if(disconnected)check(f.w.kill(f.target,26100));else f.w.move(1,4500,100,26100);
        bool returning=false,home=false;
        for(unsigned at=26100;at<=58000;at+=100){f.tick(at);const auto ai=*f.w.npc_flare_ai_snapshot(f.source);
            returning|=ai.returning;home|=returning&&!ai.returning&&!ai.engaged;}
        check(returning&&home);const auto before=f.moves;
        for(unsigned at=58100;at<=66000;at+=100)f.tick(at);check(f.moves>before);
        f.w.leave(1);f.w.join(1,"Fresh melee",f.x,f.y,f.box,0,{}, {},66100,1);
        const auto fresh=*f.w.effect_scope(1);check(fresh!=f.target);f.w.set_vitals(1,{1000,1000,40,40},100);check(f.w.physical_armor(fresh,0));
        bool pending=false;for(unsigned at=66100;at<=70000;at+=100){f.tick(at);
            if(const auto action=f.w.npc_ordinary_pending(f.source)){check(action->target==fresh);pending=true;break;}}
        check(pending);f.w.leave(1);f.w.join(1,"Replacement",4500,100,f.box,0,{}, {},70100,1);
        f.w.set_vitals(1,{1000,1000,40,40},100);for(unsigned at=70100;at<=73000;at+=100)f.tick(at);
        check(f.w.resources(1)->health==1000&&!f.w.npc_ordinary_pending(f.source));
    }
    std::cout<<"Population ordinary profile identity, navigation, pursuit/hit/return and lifetime passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
