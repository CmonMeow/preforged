#include "gw_world_simulation.h"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace fw::gw;
static void check(bool yes){if(!yes)throw std::runtime_error("Lightbringer World target test failed");}
int main()try{
    const auto plan=*skills::effects::lightbringers_gaze(8);
    for(unsigned mode=0;mode<7;++mode){
        world::Instance w;
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;
        npc.x=mode==4?1300.f:60.f;
        npc.definition.level=20;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;
        body.binding.map=mode==2?146:474;
        body.binding.actor=body.binding.definition=100;
        body.binding.level=20;
        body.spawn_health={100,100};
        body.damageable=true;
        body.relation=mode==3?combat::npcs::Relation::allied_to_players:
                              combat::npcs::Relation::hostile_to_players;
        if(mode!=2&&mode!=6){body.servant_of_abaddon=mode!=1;
            body.servant_of_abaddon_evidence="synthetic fixture";}
        npc.combat_definition=body;
        w.configure_npcs({npc});
        w.join(1,"Lightbringer",0,0,std::make_shared<world::Outbox>(),
            0,{}, {},100,1);
        w.set_vitals(1,{100,100,50,50},100);
        const auto source=*w.effect_scope(1);
        const auto target=*w.effect_scope(100);
        if(mode==5){
            auto stale=target;++stale.generation;
            check(!w.valid_lightbringers_gaze_primary(source,stale,plan,1000));
        }else if(w.valid_lightbringers_gaze_primary(source,target,plan,1000)!=
                    (mode==0||mode==6))throw std::runtime_error("Lightbringer primary mode "+
                        std::to_string(mode));
        check(!w.valid_lightbringers_gaze_primary(source,source,plan,1000));
    }
    std::cout<<"PASS Gaze primary rejects false, unknown, allied, remote and stale NPC\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
