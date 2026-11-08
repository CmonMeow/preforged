#include "gw_world_simulation.h"
#include <cmath>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Imbue Health World line "+std::to_string(at.line()));
}

int main(){
    world::Instance w;
    world::Instance::Npc npc;
    npc.id=npc.definition.id=100;
    npc.x=150;
    npc.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition body;
    body.binding.map=146;
    body.binding.actor=body.binding.definition=100;
    body.relation=combat::npcs::Relation::allied_to_players;
    body.damageable=true;
    body.spawn_health={100,1000};
    npc.combat_definition=body;
    w.configure_npcs({npc});
    w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.join(3,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    w.set_vitals(1,{480,480,40,40},100);
    w.set_vitals(2,{100,1000,40,40},100);
    w.set_vitals(3,{100,1000,40,40},100);
    const auto source=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
    const auto npc_ally=*w.effect_scope(100);
    const auto plan=*skills::effects::imbue_health(12);
    check(!w.imbue_health(source,source,plan,100));
    check(!w.imbue_health(source,foe,plan,100));
    auto stale=ally;++stale.generation;
    check(!w.imbue_health(source,stale,plan,100));
    auto forged=plan;forged.percent=90;
    check(!w.imbue_health(source,ally,forged,100));
    check(w.resources(2)->health==100);
    check(w.imbue_health(source,ally,plan,100));
    check(w.resources(1)->health==480);
    check(w.resources(2)->health==297);
    check(w.imbue_health(source,ally,*skills::effects::imbue_health(20),100));
    check(w.resources(2)->health==597);
    check(w.imbue_health(source,npc_ally,plan,100));
    check(std::abs(w.npc_combat_snapshot(npc_ally)->health->current-296.8)<.001);
    std::cout<<"PASS Imbue Health World scoped allied healing, current-Health cap and rejection; cast unbound\n";
}
