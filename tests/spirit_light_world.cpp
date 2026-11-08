#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>

using namespace fw::gw;
using skills::effects::SpiritProximity;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Spirit Light World line "+std::to_string(at.line()));
}

int main()try{
    for(unsigned mode=0;mode<6;++mode){
        world::Instance w;
        if(mode){
            world::Instance::Npc npc;
            npc.id=npc.definition.id=100;
            npc.x=mode==4?2000.f:100.f;
            npc.y=0;
            npc.plane=mode==5?1:0;
            npc.definition.level=20;
            npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition body;
            body.binding.map=146;
            body.binding.actor=body.binding.definition=100;
            body.binding.level=20;
            body.relation=combat::npcs::Relation::hostile_to_players;
            body.spawn_health=combat::Health{100,100};
            body.damageable=true;
            if(mode!=3){
                body.creature_type=mode==1?"human":"spirit";
                body.creature_type_evidence="fixture";
            }
            npc.combat_definition=body;
            w.configure_npcs({npc});
        }
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,100,50,50},100);
        const auto caster=*w.effect_scope(1);
        const auto expected=mode==2?SpiritProximity::present:
                            mode==3?SpiritProximity::unknown:SpiritProximity::absent;
        check(w.spirit_light_proximity(caster,200)==expected);
        auto stale=caster;++stale.generation;
        check(w.spirit_light_proximity(stale,200)==SpiritProximity::unknown);
    }
    for(unsigned mode=0;mode<3;++mode){
        world::Instance w;
        if(mode){
            world::Instance::Npc npc;
            npc.id=npc.definition.id=100;npc.x=100;
            npc.definition.level=20;npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition body;
            body.binding.map=146;body.binding.actor=body.binding.definition=100;
            body.binding.level=20;body.relation=combat::npcs::Relation::hostile_to_players;
            body.spawn_health=combat::Health{100,100};body.damageable=true;
            if(mode==1){body.creature_type="spirit";body.creature_type_evidence="fixture";}
            npc.combat_definition=body;w.configure_npcs({npc});
        }
        auto caster_box=std::make_shared<world::Outbox>();
        auto ally_box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,caster_box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,ally_box,0,{}, {},100,1);
        w.set_vitals(1,{100,100,50,50},100);
        w.set_vitals(2,{20,100,50,50},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        const auto plan=*skills::effects::spirit_light(0);
        auto stale=target;++stale.generation;
        check(!w.stage_spirit_light_player(source,stale,plan,200));
        check(!w.stage_spirit_light_player(source,source,plan,200));
        check(w.resources(1)->health==100&&w.resources(2)->health==20);
        caster_box->drain();ally_box->drain();
        std::function<void()> publish;
        const auto loss=w.stage_spirit_light_player(source,target,plan,200,&publish);
        check(bool(loss)==(mode!=2));
        check(w.resources(1)->health==(mode==0?83u:100u));
        check(w.resources(2)->health==(mode==2?20u:80u));
        check(caster_box->drain().empty()&&ally_box->drain().empty());
        if(loss){
            check(bool(publish)&&loss->lost==(mode==0?17:0));
            publish();
            const auto packets=ally_box->drain();
            check(std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(2,1,60,100))!=packets.end());
        }
    }
    {
        world::Instance w;
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{10,100,50,50},100);
        w.set_vitals(2,{20,100,50,50},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        const auto loss=w.stage_spirit_light_player(source,target,
            *skills::effects::spirit_light(0),200);
        check(loss&&loss->killed&&w.resources(1)->health==0&&
              w.resources(2)->health==80);
        check(!w.stage_spirit_light_player(source,target,
            *skills::effects::spirit_light(0),300));
    }
    for(unsigned mode=0;mode<5;++mode){
        world::Instance w;
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;
        npc.x=mode==3?1800.f:60.f;
        npc.definition.level=20;npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;
        body.binding.map=146;body.binding.actor=body.binding.definition=100;
        body.binding.level=20;
        body.relation=mode==1?combat::npcs::Relation::hostile_to_players:
            combat::npcs::Relation::allied_to_players;
        body.spawn_health=combat::Health{20,100};body.damageable=body.conditions_allowed=true;
        body.creature_type="human";body.creature_type_evidence="fixture";
        if(mode!=2){body.healing_allowed=true;body.healing_evidence="fixture";}
        npc.combat_definition=body;w.configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{100,100,50,50},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        auto selected=target;if(mode==4)++selected.generation;
        box->drain();std::function<void()> publish;
        const auto loss=w.stage_spirit_light_npc(source,selected,
            *skills::effects::spirit_light(0),200,&publish);
        if(bool(loss)!=(mode==0))throw std::runtime_error("Spirit Light NPC mode "+std::to_string(mode));
        check(w.resources(1)->health==(mode==0?83u:100u));
        check(w.npc_combat_snapshot(target)->health->current==(mode==0?80:20));
        check(box->drain().empty());
        if(loss){
            check(bool(publish));publish();
            const auto packets=box->drain();
            check(std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(100,1,60,100))!=packets.end());
        }
    }
    std::cout<<"Spirit Light typed proximity and player/NPC heal/sacrifice stages passed; casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
