#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Brood navigation line "+std::to_string(at.line()));
}
std::shared_ptr<const navigation::Mesh> mesh_fixture(bool disconnected){
    const auto path=std::filesystem::temp_directory_path()/("brood-nav-"+
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
world::Instance::Npc brood(){
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
    std::ifstream appearance(root/"actors/domination-magic-brood.txt");check(bool(appearance));
    auto population=world::load_npc_catalog(appearance);
    std::ifstream header(root/"domination-magic-development-combat.txt");std::string magic,hash;unsigned version;
    header>>magic>>version>>hash;check(magic=="gw-npc-combat"&&version>0);
    std::ifstream metadata(root/"domination-magic-development-combat.txt");
    world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
    auto roster=population.for_map(161);check(!roster.empty());auto result=roster.front();result.x=160;result.y=100;return result;
}
int main()try{
    {
        const std::array<combat::ai::motion::Point,3> route{{{10,0},{10,10},{20,10}}};
        const auto moved=combat::ai::motion::advance({0,0},route,{20,10},0,25);
        check(moved&&moved->spent==25&&!moved->reached&&moved->path.size()==3);
        check(moved->path[0].x==10&&moved->path[0].y==0);
        check(moved->path[1].x==10&&moved->path[1].y==10);
        check(moved->point.x==15&&moved->point.y==10&&moved->facing==std::optional(0.0));
        check(moved->path.back().x==moved->point.x&&moved->path.back().y==moved->point.y);
    }
    for(bool disconnected:{false,true}){
        world::Instance w;const auto mesh=mesh_fixture(disconnected);w.configure_navigation(mesh);
        auto box=std::make_shared<world::Outbox>();w.join(1,"Brood owner",800,100,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,40,40},100);const auto owner=*w.effect_scope(1);
        check(w.elemental_armor(owner,60,60));check(w.physical_armor(owner,0));
        check(w.spawn_kill_encounter(owner,49,1,{brood()},[]{return true;}));const auto source=*w.effect_scope(60000);
        check(w.arm_brood_caster(owner,1,1000));check(w.arm_brood_wander(owner,1,1000)==1);
        float x=160,y=100;unsigned moves=0;bool unsafe=false,detoured=false,unavailable=false;
        const char* phase="idle";
        box->drain();
        const auto drain=[&]{for(const auto& p:box->drain()){
            if(p.size()!=16||p[0]!=44||p[1]!=0)continue;
            const auto read=[&](unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);};
            if(read(2)!=source.actor)continue;
            const float nx=std::bit_cast<float>(read(6)),ny=std::bit_cast<float>(read(10));
            // Check the published segment, not merely its valid endpoint.
            // Dense sub-unit samples catch a shortcut across the 40-unit wall.
            const unsigned steps=std::max(1u,unsigned(std::ceil(std::hypot(nx-x,ny-y)*4)));
            for(unsigned i=0;i<=steps;++i){const float t=float(i)/steps;
                if(!mesh->locate_cell(0,{x+(nx-x)*t,y+(ny-y)*t})){
                    if(!unsafe)std::cerr<<(disconnected?"disconnected ":"connected ")<<phase
                        <<" unsafe published segment ("<<x<<','<<y<<") -> ("<<nx<<','<<ny<<")\n";
                    unsafe=true;
                }
            }
            x=nx;y=ny;++moves;detoured=detoured||y>=299;
        }};
        const auto tick=[&](unsigned at){w.tick(at);drain();const auto movement=w.npc_pursuit_movement_snapshot(source);
            unavailable=unavailable||(movement&&movement->outcome.kind==combat::ai::motion::OutcomeKind::route_unavailable);};
        for(unsigned at=1000;at<=10000;at+=100)tick(at);
        check(!w.npc_elemental_ai_snapshot(source)->engaged);check(w.resources(1)->health==1000);
        check(moves>0);if(disconnected)check(unavailable&&x<=180);else check(detoured);
        // A local attacker supplies legitimate damage/LOS; the owned quest
        // controller still selects its original owner across the obstacle.
        // Do not bypass authoritative damage tracing to manufacture a pull.
        w.join(2,"Local attacker",x,y,std::make_shared<world::Outbox>(),0,{}, {},10050,1);
        w.set_vitals(2,{1000,1000,40,40},100);const auto attacker=*w.effect_scope(2);
        check(w.damage_npc(attacker,source,{combat::Kind::armor_ignoring,1,20,194},1248,10100).reason==combat::Reason::accepted);
        phase="combat";
        for(unsigned at=10100;at<=30000;at+=100)tick(at);
        if(disconnected){check(w.resources(1)->health==1000);check(x<=180);check(unavailable);}
        else check(w.resources(1)->health<1000);
        check(w.kill(owner,30100));bool returning=false,returned=false;phase="return";
        for(unsigned at=30100;at<=50000;at+=100){tick(at);const auto ai=*w.npc_elemental_ai_snapshot(source);
            returning=returning||ai.returning;if(returning&&!ai.returning&&!ai.engaged){returned=true;break;}}
        check(returned);const auto before=moves;phase="resumed idle";
        for(unsigned at=50100;at<=56000;at+=100)tick(at);
        check(moves>before);check(!w.npc_elemental_ai_snapshot(source)->engaged);
        check(!unsafe);
    }
    std::cout<<"Brood nav detour/disconnection, combat, return and idle segments\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
