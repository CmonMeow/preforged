#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Healing Ribbon World line "+std::to_string(at.line()));
}
static world::Instance::Npc make_npc(unsigned id,float x,bool allied=true){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;
    n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;
    d.binding.actor=d.binding.definition=id;d.binding.level=20;
    d.spawn_health={20,100};d.damageable=true;
    d.relation=allied?combat::npcs::Relation::allied_to_players:
        combat::npcs::Relation::hostile_to_players;
    d.healing_allowed=true;d.healing_evidence="fixture";
    n.combat_definition=d;return n;
}
int main()try{
    const auto plan=*skills::effects::healing_ribbon(0);
    for(bool npc_primary:{false,true}){
        world::Instance w;
        w.configure_npcs({make_npc(100,105),make_npc(101,108,false),make_npc(102,2000)});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Nearby",110,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(4,"Foe",112,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u,4u})w.set_vitals(id,{20,100,50,50},100);
        const auto source=*w.effect_scope(1);
        const auto primary=*w.effect_scope(npc_primary?100:2);
        check(!w.stage_healing_ribbon(source,source,plan,0,200));
        check(!w.stage_healing_ribbon(source,*w.effect_scope(101),plan,0,200));
        auto stale=primary;++stale.generation;
        check(!w.stage_healing_ribbon(source,stale,plan,0,200));
        box->drain();
        std::function<void()> publish;
        const auto selected=w.stage_healing_ribbon(source,primary,plan,0,200,&publish);
        check(selected&&selected->size()==3&&bool(publish));
        check((*selected)[0]==primary);
        check(w.resources(4)->health==20);
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==20);
        check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==20);
        check(w.resources(1)->health==20);
        check(box->drain().empty());
        publish();
        const auto packets=box->drain();
        check(!packets.empty());
        if(npc_primary){
            check(w.npc_combat_snapshot(primary)->health->current==40);
            check(w.resources(2)->health==30);
        }else{
            check(w.resources(2)->health==40);
            check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==30);
        }
        check(std::find(packets.begin(),packets.end(),
            skills::particles::health_change_2022(primary.actor,source.actor,20,100))!=packets.end());
        publish();check(box->drain().empty());
    }
    for(bool npc_edge:{false,true}){
        world::Instance w;w.configure_npcs({make_npc(100,npc_edge?252.f:253.f)});
        w.join(1,"Caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Primary",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Secondary",npc_edge?253.f:252.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{20,100,50,50},100);
        const auto source=*w.effect_scope(1),primary=*w.effect_scope(2),npc=*w.effect_scope(100);
        const auto selected=w.stage_healing_ribbon(source,primary,plan,0,100);
        check(selected&&selected->size()==2&&(*selected)[0]==primary);
        check((*selected)[1]==*w.effect_scope(npc_edge?100:3));
        check(w.resources(1)->health==20&&w.resources(2)->health==40);
        check(w.resources(3)->health==(npc_edge?20:30));
        check(w.npc_combat_snapshot(npc)->health->current==(npc_edge?30:20));
    }
    std::cout<<"PASS Healing Ribbon mixed allied player/NPC recipients and deferred publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
