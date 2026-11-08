#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Mixed immediate NPC line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned energy:{10u,40u}){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=2;
        n.definition.flags=4;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding={145,100,100,0,0,0,4,2};d.relation=combat::npcs::Relation::hostile_to_players;
        d.spawn_health=combat::Health{500,500};d.spawn_energy=combat::npcs::Energy{energy,energy};d.energy_pips=0;d.damageable=true;
        n.combat_definition=d;w.configure_npcs({n});w.join(1,"Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,40,40},100);const auto source=*w.effect_scope(100),target=*w.effect_scope(1);
        check(w.physical_armor(target,0));combat::ai::FlarePolicy p;p.source=source;p.level=2;p.speed=220;
        p.acquire=200;p.follow=1500;p.leash=1000;p.flight={1000,32};p.evidence="synthetic explicit mixed attack profile";
        check(w.configure_npc_immediate_ai(p,{{{155,0},*skills::effects::shadow_damage(155,0),p.evidence}},1000));
        check(w.configure_npc_ordinary_ai(source,{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence},1000));
        w.tick(1000);const auto first=w.npc_flare_ai_snapshot(source)->cast;check(first&&first->skill->skill==155);
        check(!w.npc_ordinary_pending(source));w.tick(first->completes);check(w.resources(1)->health==980);
        const auto aftercast=first->completes+750;w.tick(aftercast-100);check(!w.npc_ordinary_pending(source));
        w.tick(aftercast);const auto swing=w.npc_ordinary_pending(source);check(swing.has_value());
        check(!w.npc_flare_ai_snapshot(source)->cast&&w.npc_energy_snapshot(source)->current==energy-10);
        w.tick(swing->token.completes-100);check(w.resources(1)->health==980);
        w.tick(swing->token.completes);check(w.resources(1)->health<980);const auto after_hit=w.resources(1)->health;
        w.tick(3650);check(!w.npc_ordinary_pending(source)&&!w.npc_flare_ai_snapshot(source)->cast);
        check(w.resources(1)->health==after_hit);
        w.tick(3750);
        if(energy==40){const auto next=w.npc_flare_ai_snapshot(source)->cast;
            check(next&&next->skill->skill==155&&w.npc_energy_snapshot(source)->current==20);
            check(!w.npc_ordinary_pending(source));
        }else{
            check(!w.npc_flare_ai_snapshot(source)->cast&&w.npc_energy_snapshot(source)->current==0);
            w.tick(4400);check(!w.npc_ordinary_pending(source));w.tick(4500);check(w.npc_ordinary_pending(source).has_value());
            w.tick(5250);check(w.resources(1)->health<after_hit&&w.npc_energy_snapshot(source)->current==0);
        }
    }
    std::cout<<"NPC155 uses explicit ordinary fallback during recharge/empty Energy without cadence bypass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
