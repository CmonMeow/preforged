#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC immediate AI World line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned skill:{155u}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=2;n.definition.flags=4;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding={145,100,100,0,0,0,4,2};d.relation=combat::npcs::Relation::hostile_to_players;
        d.spawn_health=combat::Health{500,500};d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;d.damageable=true;
        n.combat_definition=d;w.configure_npcs({n});w.join(1,"Target",500,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,40,40},100);const auto source=*w.effect_scope(100),target=*w.effect_scope(1);
        combat::ai::FlarePolicy p;p.source=source;p.level=2;p.speed=220;p.acquire=200;p.follow=1500;p.leash=1000;
        p.flight={1000,32};p.evidence="synthetic authored immediate-cast test";
        check(w.configure_npc_immediate_ai(p,{{{static_cast<std::uint16_t>(skill),0},*skills::effects::shadow_damage(skill,0),p.evidence}},1000));
        check(w.configure_npc_wander(source,{{{80,0},{0,80}},200,4000,p.evidence},1000));
        w.tick(1000);w.tick(1400);check(!w.npc_flare_ai_snapshot(source)->engaged);
        check(w.npc_pursuit_movement_snapshot(source)->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
        w.tick(1500);check(w.npc_pursuit_movement_snapshot(source)->outcome.travelled>0);
        // Damage pulls from outside acquisition, using the ordinary scoped
        // provocation path rather than manually selecting a controller target.
        check(w.damage_npc(target,source,{combat::Kind::armor_ignoring,1,20,194},1248,1600).reason==combat::Reason::accepted);
        bool attacked=false;
        for(unsigned at=1600;at<=12000;at+=100){w.tick(at);if(w.resources(1)->health<1000){attacked=true;break;}}
        check(attacked&&w.npc_energy_snapshot(source)->current<40);
        check(w.kill(target,12100));bool returning=false,returned=false;
        for(unsigned at=12100;at<=24000;at+=100){w.tick(at);const auto ai=*w.npc_flare_ai_snapshot(source);
            returning=returning||ai.returning;if(returning&&!ai.returning&&!ai.engaged){returned=true;break;}}
        check(returned);box->drain();
        for(unsigned at=24100;at<=27000;at+=100)w.tick(at);
        bool moved=false;for(const auto& packet:box->drain())if(packet.size()==16&&packet[0]==44&&packet[1]==0&&packet[2]==100&&packet[3]==0)moved=true;
        check(moved&&!w.npc_flare_ai_snapshot(source)->engaged);
    }
    std::cout<<"Reusable NPC immediate non-spell idle, damage pull, chase, fight, return and resume\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
