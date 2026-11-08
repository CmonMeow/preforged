#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC elemental capacity World line "+std::to_string(at.line()));
}
int main()try{
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
    d.spawn_energy=combat::npcs::Energy{20,30};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
    w.join(1,"Stationary target",1000,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
    const auto source=*w.effect_scope(100),target=*w.effect_scope(1);check(w.elemental_armor(target,60,60));
    combat::ai::FlarePolicy policy;policy.source=source;policy.level=20;policy.fire_magic=12;policy.speed=100;
    policy.acquire=2000;policy.follow=2500;policy.leash=2500;policy.flight={100,32};
    policy.evidence="synthetic bounded capacity fixture; actual minimum valid flight speed100 and stationary target";
    check(w.configure_npc_elemental_ai(policy,{{{194,0},*skills::effects::elemental(194,12),policy.evidence}},1,1000));
    combat::ai::ordinary::Reviewed ordinary{10,10,0,5,true,combat::ai::ordinary::Delivery::ranged,
        2000,2000,1000,{10000,32},policy.evidence};check(w.configure_npc_ordinary_ai(source,ordinary,1000));
    w.tick(1000);const auto paid=w.npc_elemental_ai_snapshot(source);check(paid&&paid->pending.has_value());
    const auto token=paid->pending->token;check(w.npc_energy_snapshot(source)->current==15);
    // Capacity readiness must never cancel a cast already admitted and paid.
    w.tick(1500);const auto casting=w.npc_elemental_ai_snapshot(source);
    check(casting&&casting->pending&&casting->pending->token==token);check(!w.npc_ordinary_pending(source));
    w.tick(2000);const auto released=w.npc_elemental_ai_snapshot(source);
    check(released&&!released->pending&&released->flights==1);check(w.resources(1)->health==1000);
    // The spell has finished aftercast, but its1000-unit/speed100 flight still
    // occupies the sole slot. It is not admissible. Exercise the driver's
    // existing ordinary fallback, rather than invent another priority rule.
    w.tick(2750);const auto fallback=w.npc_ordinary_pending(source);check(fallback.has_value());
    check(w.npc_elemental_ai_snapshot(source)->flights==1);check(!w.npc_elemental_ai_snapshot(source)->pending);
    check(w.npc_energy_snapshot(source)->current==15);
    w.tick(2850);const auto retained=w.npc_ordinary_pending(source);
    check(retained&&retained->token==fallback->token);check(w.npc_energy_snapshot(source)->current==15);
    w.tick(4750);check(!w.npc_ordinary_pending(source));check(w.npc_elemental_ai_snapshot(source)->flights==1);
    w.tick(4850);check(w.resources(1)->health==990); //independent ordinary projectile
    check(w.npc_energy_snapshot(source)->current==15);
    std::cout<<"NPC elemental capacity admission and fallback passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
