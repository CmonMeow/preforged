#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC pursuit float precision line "+std::to_string(at.line()));
}
std::uint32_t word(const world::Packet& p,std::size_t i){return std::uint32_t(p[i])|std::uint32_t(p[i+1])<<8|
    std::uint32_t(p[i+2])<<16|std::uint32_t(p[i+3])<<24;}
int main()try{
    {
        // The numerical inset must not force a return when the strict weapon
        // range is already reached exactly at the authored home leash.
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=160;d.binding.actor=d.binding.definition=100;d.binding.level=1;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{80,80};d.damageable=true;
        n.combat_definition=d;w.configure_npcs({n});w.join(1,"Leash-edge target",1128,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,0,100},100);check(w.physical_armor(*w.effect_scope(1),0));const auto source=*w.effect_scope(100);
        combat::ai::FlarePolicy p;p.source=source;p.level=1;p.speed=440;p.acquire=1500;p.follow=2000;p.leash=1000;
        p.flight={1000,32};p.evidence="synthetic strict-range/leash intersection fixture";
        check(w.configure_npc_hostile_ordinary(p,{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence},1000));
        w.tick(1000);w.tick(4000);check(w.npc_flare_ai_snapshot(source)->engaged&&!w.npc_flare_ai_snapshot(source)->returning);
        w.tick(4100);check(w.npc_ordinary_pending(source).has_value());w.tick(4900);check(w.resources(1)->health==995);
    }
    for(unsigned kind:{0u,1u,2u})for(float sign:{1.f,-1.f}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
        // Actual first Grawl placement magnitude exposes float quantization;
        // negative coordinates exercise the same numerical boundary.
        n.x=sign*13781;n.y=sign*4789;float x=n.x,y=n.y;const float tx=x+500,ty=y+500;
        combat::npcs::Definition d;d.binding.map=160;d.binding.actor=d.binding.definition=100;d.binding.level=1;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{80,80};d.damageable=true;
        if(kind==1){d.spawn_energy=combat::npcs::Energy{0,30};d.energy_pips=0;}
        n.combat_definition=d;if(kind!=2)w.configure_npcs({n});
        w.join(1,"Angled target",tx,ty,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
        const auto target=*w.effect_scope(1);check(w.physical_armor(target,0));check(w.elemental_armor(target,0,0));
        skills::effects::Scope source;
        if(kind==2){check(w.spawn_kill_encounter(target,75,1,{n},[]{return true;}));source=*w.effect_scope(60000);
            check(w.arm_grawl_melee(target,1,1000));
            check(w.damage_npc(target,source,{combat::Kind::armor_ignoring,1,20,194},1248,1000).reason==combat::Reason::accepted);
        }else{
            source=*w.effect_scope(100);combat::ai::FlarePolicy p;p.source=source;p.level=1;p.speed=220;
            p.acquire=1000;p.follow=1500;p.leash=2000;p.flight={1000,32};p.evidence="synthetic float-position stopping fixture";
            combat::ai::ordinary::Reviewed attack{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};
            if(kind==1){check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,0),p.evidence}},4,1000));
                check(w.configure_npc_ordinary_ai(source,attack,1000));}
            else check(w.configure_npc_hostile_ordinary(p,attack,1000));
        }
        box->drain();bool attacked=false;
        for(unsigned at=1000;at<=10000;at+=100){
            w.tick(at);
            for(const auto& p:box->drain())if(p.size()==16&&p[0]==44&&p[1]==0&&word(p,2)==source.actor){
                x=std::bit_cast<float>(word(p,6));y=std::bit_cast<float>(word(p,10));
            }
            if(w.npc_ordinary_pending(source)){attacked=true;check(std::hypot(double(tx)-x,double(ty)-y)<=128.0);}
        }
        check(attacked);check(w.resources(1)->health<=990); // sustained hits, strict reach
    }
    std::cout<<"NPC angled pursuit reaches strict melee range despite float-position quantization\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
