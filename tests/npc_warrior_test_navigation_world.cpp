// Detached real-map navigation acceptance test; no rendered-client claim.
#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Warrior real navigation line "+std::to_string(at.line()));
}
int main()try{
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw";
    std::ifstream appearance(root/"quests/actors/warrior-test-tads.txt");check(bool(appearance));
    auto population=world::load_npc_catalog(appearance);
    const auto path=root/"quests/warrior-test-development-combat.txt";
    std::ifstream header(path);std::string magic,hash;unsigned version;header>>magic>>version>>hash;
    check(magic=="gw-npc-combat");std::ifstream metadata(path);
    world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
    const auto roster=population.for_map(146);check(roster.size()==3);
    std::ifstream nav(root/"navigation/catalog.txt");std::string row,selected;
    while(std::getline(nav,row))if(row.starts_with("146="))selected=row;
    check(!selected.empty());std::istringstream entry(selected+"\n");
    const auto mesh=navigation::load_catalog(entry,root/"navigation").at(146);
    for(const auto& n:roster){
        if(!mesh->locate_cell(n.plane,{n.x,n.y}))
            std::cerr<<"Unwalkable captured roster actor="<<n.id<<" position="<<n.x<<','<<n.y<<" plane="<<n.plane<<'\n';
        check(mesh->locate_cell(n.plane,{n.x,n.y}).has_value());
    }
    const auto& n=roster.front();const navigation::Point home{n.x,n.y};
    // Select a real connected multi-cell route, not a hand-authored NPC route.
    // This is fixture selection only; production steering receives player movement.
    std::optional<navigation::Point> destination;float best=-1;std::size_t cells=0;
    for(unsigned radius:{600u,900u,1200u})for(unsigned angle=0;angle<24;++angle){
        const double a=angle*6.283185307179586/24;
        const navigation::Point point{n.x+float(radius*std::cos(a)),n.y+float(radius*std::sin(a))};
        const auto start=mesh->locate_cell(n.plane,home),end=mesh->locate_cell(n.plane,point);
        if(!start||!end)continue;
        const navigation::LayeredMesh layered(*mesh);
        const auto route=layered.route({n.plane,*start},home,{n.plane,*end},point);
        if(!route||route->size()<2)continue;
        std::set<navigation::Surface> surfaces;double length=0;bool same_plane=true;
        for(std::size_t i=0;i<route->size();++i){const auto& p=(*route)[i];
            surfaces.insert(p.surface);same_plane&=p.surface.plane==n.plane;
            if(i){const auto previous=(*route)[i-1].point;
                const double segment=std::hypot(p.point.x-previous.x,p.point.z-previous.z);length+=segment;
                // A shortest straight run crosses cells without publishing a
                // waypoint at each portal. Count traversed cells, not corners.
                const unsigned steps=std::max(1u,unsigned(std::ceil(segment/10.)));
                for(unsigned j=0;j<=steps;++j){const float t=float(j)/steps;
                    const auto cell=mesh->locate_cell(n.plane,{previous.x+(p.point.x-previous.x)*t,previous.z+(p.point.z-previous.z)*t});
                    if(cell)surfaces.insert({n.plane,*cell});else same_plane=false;}
            }}
        if(!same_plane||surfaces.size()<3||length>1800)continue;
        const float excess=float(length)-std::hypot(point.x-home.x,point.z-home.z);
        if(excess>best){best=excess;destination=point;cells=surfaces.size();}
    }
    if(!destination)std::cerr<<"No connected multi-cell pull fixture within authored follow envelope\n";
    check(destination.has_value());
    std::cout<<"Selected real map route cells="<<cells<<" excess="<<best<<" destination="<<destination->x<<','<<destination->z<<'\n';
    world::Instance w;w.configure_navigation(mesh);auto box=std::make_shared<world::Outbox>();
    w.join(1,"Real tad owner",n.x,n.y,box,0,{}, {},100,1);
    w.set_vitals(1,{1000,1000,40,40},100);const auto owner=*w.effect_scope(1);check(w.physical_armor(owner,0));
    check(w.spawn_kill_encounter(owner,85,1,roster,[]{return true;},false,{},{},false,0,false));
    const auto source=*w.effect_scope(60000);
    check(w.arm_warrior_test_melee(owner,1,1000)==3);check(w.arm_warrior_test_wander(owner,1,1000)==3);
    std::map<unsigned,navigation::Point> positions;
    for(unsigned i=0;i<3;++i)positions.emplace(60000+i,navigation::Point{roster[i].x,roster[i].y});
    box->drain();unsigned moves=0,source_moves=0;
    const auto tick=[&](unsigned at){w.tick(at);
        for(const auto& p:box->drain()){
            if(p.size()!=16||p[0]!=44||p[1])continue;
            const auto word=[&](unsigned i){return unsigned(p[i])|(unsigned(p[i+1])<<8)|(unsigned(p[i+2])<<16)|(unsigned(p[i+3])<<24);};
            const auto id=word(2);const auto old=positions.find(id);if(old==positions.end())continue;
            const navigation::Point next{std::bit_cast<float>(word(6)),std::bit_cast<float>(word(10))};
            const auto plane=unsigned(p[14])|(unsigned(p[15])<<8);check(plane==roster[id-60000].plane);
            const unsigned steps=std::max(1u,unsigned(std::ceil(std::hypot(next.x-old->second.x,next.z-old->second.z)*4)));
            for(unsigned i=0;i<=steps;++i){const float t=float(i)/steps;
                check(mesh->locate_cell(plane,{old->second.x+(next.x-old->second.x)*t,old->second.z+(next.z-old->second.z)*t}).has_value());}
            if(std::hypot(next.x-old->second.x,next.z-old->second.z)>.01){++moves;if(id==source.actor)++source_moves;}
            old->second=next;
        }
    };
    for(unsigned at=1000;at<=5000;at+=100)tick(at);
    check(moves>0&&w.resources(1)->health==1000&&!w.npc_flare_ai_snapshot(source)->engaged);
    check(w.damage_npc(owner,source,{combat::Kind::armor_ignoring,1,20,194},1248,5100).reason==combat::Reason::accepted);
    w.move(1,destination->x,destination->z,5100);
    const auto before=source_moves;bool engaged=false,paid=false;
    for(unsigned at=5100;at<=22000;at+=100){tick(at);engaged|=w.npc_flare_ai_snapshot(source)->engaged;paid|=w.resources(1)->health<1000;}
    check(engaged&&paid&&source_moves>before);
    const auto target=w.position(1,22000);check(std::hypot(target.first-destination->x,target.second-destination->z)<1);
    // Real target loss must return by the same navigation authority and resume idle.
    check(w.kill(owner,22100));bool returning=false,returned=false;
    for(unsigned at=22100;at<=45000;at+=100){tick(at);const auto ai=*w.npc_flare_ai_snapshot(source);
        returning|=ai.returning;returned|=returning&&!ai.returning&&!ai.engaged;}
    check(returned&&!w.npc_ordinary_pending(source));const auto resumed_from=source_moves;
    for(unsigned at=45100;at<=52000;at+=100)tick(at);
    check(source_moves>resumed_from);check(w.npc_ai_coverage().wandering==3);
    std::cout<<"Quest85 real coordinates/mesh, bounded patrol, multi-cell pull/hit, return and patrol passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


