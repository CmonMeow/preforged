#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Firestarter population catalog line "+std::to_string(at.line()));
}
int main()try{
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw";
    std::ifstream appearance(root/"ai/firestarter-development-npcs.txt");check(bool(appearance));
    auto catalog=world::load_npc_catalog(appearance);
    const auto combat_path=root/"ai/firestarter-development-combat.txt";
    std::ifstream header(combat_path);std::string magic,hash;unsigned version=0;header>>magic>>version>>hash;
    check(magic=="gw-npc-combat"&&version==8);std::ifstream metadata(combat_path);
    world::bind_combat_catalog(catalog,combat::npcs::load_catalog(metadata,hash),hash);
    check(catalog.maps.size()==1&&catalog.for_map(146).size()==1);
    const auto& n=catalog.for_map(146).front();const auto& d=*n.combat_definition;
    check(n.id==59000&&n.definition.id==59000&&n.definition.model_file==116226&&n.definition.level==1);
    check(d.spawn_health->maximum==100&&d.spawn_energy==combat::npcs::Energy{40,40}&&d.energy_pips==4);
    check(!d.fleshy&&!d.holy_traits.light_sensitive&&!d.holy_traits.summoned&&!d.creature_type);
    check(!d.knockdown_allowed&&!d.healing_allowed&&!d.disease_immune&&!d.conditions_allowed);
    // Same catalog map mesh used by the host; preserve the actual coordinate.
    std::ifstream nav_catalog(root/"navigation/catalog.txt");std::string row,selected;
    while(std::getline(nav_catalog,row))if(row.starts_with("146="))selected=row;
    check(!selected.empty());std::istringstream one(selected+"\n");
    const auto meshes=navigation::load_catalog(one,root/"navigation");const auto mesh=meshes.at(146);
    check(mesh->locate_cell(n.plane,{n.x,n.y}).has_value());
    world::Instance w;w.configure_navigation(mesh);w.configure_npcs(catalog.for_map(146));
    auto box=std::make_shared<world::Outbox>();w.join(1,"Patrol observer",n.x,n.y,box,0,{}, {},100,1);
    w.set_vitals(1,{0,1000,40,40},100);const auto source=*w.effect_scope(n.id);
    combat::ai::FlarePolicy p;p.source=source;p.level=1;p.speed=220;p.acquire=1000;p.follow=1500;p.leash=2000;
    p.flight={1000,32};p.max_flights=4;p.evidence="data/gw/ai/firestarter-development-policy.txt";
    const auto enroll=[&](unsigned now){return w.configure_npc_population_elemental(source,d.binding,p,
        {{{194,0},*skills::effects::elemental(194,0),p.evidence}},
        {{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,p.evidence},now);};
    check(enroll(1000)==world::Instance::NpcPopulationEnrollment::armed);box->drain();
    float x=n.x,y=n.y;unsigned moves=0;
    const auto tick=[&](unsigned at){w.tick(at);
        for(const auto& packet:box->drain()){
            if(packet.size()!=16||packet[0]!=44||packet[1])continue;
            const auto word=[&](unsigned i){return unsigned(packet[i])|(unsigned(packet[i+1])<<8)|(unsigned(packet[i+2])<<16)|(unsigned(packet[i+3])<<24);};
            if(word(2)!=n.id)continue;const float nx=std::bit_cast<float>(word(6)),ny=std::bit_cast<float>(word(10));
            const unsigned steps=std::max(1u,unsigned(std::ceil(std::hypot(nx-x,ny-y)*4)));
            for(unsigned i=0;i<=steps;++i){const float t=float(i)/steps;check(mesh->locate_cell(n.plane,{x+(nx-x)*t,y+(ny-y)*t}).has_value());}
            x=nx;y=ny;++moves;
        }
    };
    for(unsigned at=1000;at<=15000;at+=100)tick(at);
    check(moves>0);check(enroll(15000)==world::Instance::NpcPopulationEnrollment::already_armed);
    check(!w.npc_elemental_ai_snapshot(source)->engaged&&w.npc_ai_coverage().wandering==1);
    // Exercise the installed caster on the actual map, including payment,
    // projectile damage, target loss and patrol resumption. A dead observer
    // records every authoritative motion segment throughout the encounter.
    auto fighter=std::make_shared<world::Outbox>();
    w.join(2,"Firestarter target",x,y,fighter,0,{}, {},15100,1);
    w.set_vitals(2,{1000,1000,40,40},100);
    const auto target=*w.effect_scope(2);check(w.elemental_armor(target,0,0));
    bool engaged=false,cast=false,hit=false;
    for(unsigned at=15100;at<=23000;at+=100){tick(at);
        const auto ai=*w.npc_elemental_ai_snapshot(source);engaged|=ai.engaged;
        if(ai.pending){check(ai.pending->use.target==target);cast=true;}
        hit|=w.resources(2)->health<1000;
    }
    check(engaged&&cast&&hit);w.leave(2);
    bool returning=false,home=false;
    for(unsigned at=23100;at<=45000;at+=100){tick(at);
        const auto ai=*w.npc_elemental_ai_snapshot(source);
        returning|=ai.returning;home|=returning&&!ai.returning&&!ai.engaged;
    }
    check(returning&&home&&!w.npc_elemental_ai_snapshot(source)->pending);
    const auto after_return=moves;
    for(unsigned at=45100;at<=52000;at+=100)tick(at);
    check(moves>after_return);
    std::cout<<"Firestarter real catalog and map146 nav patrol/proximity/cast/damage/return/resume passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
