#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Tad population catalog line "+std::to_string(at.line()));
}
int main()try{
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw";
    std::ifstream appearance(root/"npcs.txt");check(bool(appearance));
    auto catalog=world::load_npc_catalog(appearance);
    const auto combat_path=root/"ai/tad-development-combat.txt";
    std::ifstream header(combat_path);std::string magic,hash;unsigned version=0;header>>magic>>version>>hash;
    check(magic=="gw-npc-combat"&&version==6);std::ifstream metadata(combat_path);
    world::bind_combat_catalog(catalog,combat::npcs::load_catalog(metadata,hash),hash);
    check(catalog.maps.size()>1); unsigned tested=0;
    for(const auto& n:catalog.for_map(146)){if(!n.combat_definition)continue;++tested;const auto& d=*n.combat_definition;
    check(n.id>=10002&&n.id<=10003&&n.definition.id==n.id-10000&&n.definition.model_file==82023&&n.definition.level==0);
    check(d.spawn_health->maximum==10&&!d.spawn_energy&&!d.energy_pips);
    check(!d.fleshy&&!d.holy_traits.light_sensitive&&!d.holy_traits.summoned&&!d.creature_type);
    check(!d.knockdown_allowed&&!d.healing_allowed&&!d.disease_immune&&!d.conditions_allowed);
    // Same catalog map mesh used by the host; preserve the actual coordinate.
    std::ifstream nav_catalog(root/"navigation/catalog.txt");std::string row,selected;
    while(std::getline(nav_catalog,row))if(row.starts_with("146="))selected=row;
    check(!selected.empty());std::istringstream one(selected+"\n");
    const auto meshes=navigation::load_catalog(one,root/"navigation");const auto mesh=meshes.at(146);
    check(mesh->locate_cell(n.plane,{n.x,n.y}).has_value());
    world::Instance w;w.configure_navigation(mesh);w.configure_npcs({n});
    auto box=std::make_shared<world::Outbox>();w.join(1,"Patrol observer",n.x,n.y,box,0,{}, {},100,1);
    w.set_vitals(1,{0,1000,40,40},100);const auto source=*w.effect_scope(n.id);
    combat::ai::FlarePolicy p;p.source=source;p.level=0;p.proximity_acquisition=false;p.speed=220;p.acquire=1000;p.follow=1500;p.leash=2000;
    p.flight={1000,32};p.max_flights=4;p.evidence="data/gw/ai/tad-development-policy.txt";
    combat::ai::ordinary::Reviewed attack{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,
        750,2000,128,{1000,32},p.evidence};
    const auto enroll=[&](unsigned now){return w.configure_npc_population_ordinary(source,d.binding,p,attack,
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
    check(!w.npc_ordinary_pending(source)&&w.npc_ai_coverage().wandering==1);
    // The exact installed actor, actual mesh and same policy now fight a live
    // player. The dead observer remains to receive and validate every route leg.
    auto fighter=std::make_shared<world::Outbox>();
    w.join(2,"Tad target",x,y,fighter,0,{}, {},15100,1);
    w.set_vitals(2,{1000,1000,40,40},100);
    const auto old_target=*w.effect_scope(2);check(w.physical_armor(old_target,0));
    // Nearby living players are not acquired until this Tad is damaged.
    for(unsigned at=15100;at<=17000;at+=100)tick(at);
    check(!w.npc_flare_ai_snapshot(source)->engaged&&w.resources(2)->health==1000);
    auto bystander=std::make_shared<world::Outbox>();
    w.join(3,"Nearby bystander",x,y,bystander,0,{}, {},17000,1);
    w.set_vitals(3,{1000,1000,40,40},100);check(w.physical_armor(*w.effect_scope(3),0));
    const auto damage=w.damage_npc(old_target,source,{combat::Kind::armor_ignoring,1,20,194},1248,17000);
    check(damage.reason==combat::Reason::accepted&&damage.applied==1&&!damage.killed);
    // Re-enrollment preserves damage and encounter state; changing acquisition
    // policy is rejected rather than silently changing an active controller.
    check(enroll(17000)==world::Instance::NpcPopulationEnrollment::already_armed);
    p.proximity_acquisition=true;check(enroll(17000)==world::Instance::NpcPopulationEnrollment::conflicting_controller);
    p.proximity_acquisition=false;
    bool engaged=false,hit=false;
    for(unsigned at=17100;at<=21000;at+=100){tick(at);
        engaged|=w.npc_flare_ai_snapshot(source)->engaged;
        hit|=w.resources(2)->health<1000;
    }
    check(engaged&&hit&&!w.npc_energy_snapshot(source)&&w.resources(3)->health==1000);
    w.leave(2);bool returning=false,home=false;
    for(unsigned at=21100;at<=45000;at+=100){tick(at);
        const auto ai=*w.npc_flare_ai_snapshot(source);
        returning|=ai.returning;home|=returning&&!ai.returning&&!ai.engaged;
    }
    check(returning&&home&&!w.npc_ordinary_pending(source));
    const auto after_return=moves;
    for(unsigned at=45100;at<=52000;at+=100)tick(at);
    check(moves>after_return);
    w.join(2,"Tad fresh",x,y,fighter,0,{}, {},52100,1);
    const auto fresh=*w.effect_scope(2);check(fresh!=old_target);
    w.set_vitals(2,{1000,1000,40,40},100);check(w.physical_armor(fresh,0));
    for(unsigned at=52100;at<=53000;at+=100)tick(at);
    check(!w.npc_flare_ai_snapshot(source)->engaged&&w.resources(2)->health==1000&&w.resources(3)->health==1000);
    check(w.damage_npc(fresh,source,{combat::Kind::armor_ignoring,1,20,194},1248,53000).reason==combat::Reason::accepted);
    bool fresh_hit=false,pending=false;
    for(unsigned at=53100;at<=58000;at+=100){tick(at);
        if(const auto action=w.npc_ordinary_pending(source)){check(action->target==fresh);pending=true;}
        fresh_hit|=w.resources(2)->health<1000;
    }
    check(pending&&fresh_hit&&w.resources(3)->health==1000);
    check(w.damage_npc(fresh,source,{combat::Kind::armor_ignoring,100,20,194},1248,58100).killed);
    w.tick(59000);check(!w.npc_ordinary_pending(source));
    }
    check(tested==2);
    std::cout<<"Two exact baseline level0 Tads: authored metadata, map146 patrol, damage-only acquisition, retaliation, return and lifetime reset passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
