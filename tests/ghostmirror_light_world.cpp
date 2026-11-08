#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Ghostmirror Light World line "+std::to_string(at.line()));
}
static world::Instance::Npc make_npc(unsigned id,bool spirit,bool typed=true,bool hostile=false){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=80;
    n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition body;
    body.binding.map=146;body.binding.actor=body.binding.definition=id;
    body.binding.level=20;body.spawn_health={20,100};body.damageable=true;
    body.relation=hostile?combat::npcs::Relation::hostile_to_players:
        combat::npcs::Relation::allied_to_players;
    if(typed){body.creature_type=spirit?"spirit":"human";
              body.creature_type_evidence="fixture";}
    if(!spirit){body.healing_allowed=true;body.healing_evidence="fixture";}
    n.combat_definition=body;return n;
}
int main()try{
    const auto plan=*skills::effects::ghostmirror_light(0);
    for(unsigned mode=0;mode<6;++mode){
        world::Instance w;
        if(mode==1)w.configure_npcs({make_npc(101,true)});
        if(mode==2)w.configure_npcs({make_npc(101,true,false)});
        if(mode==3)w.configure_npcs({make_npc(100,false),make_npc(101,true)});
        if(mode==4)w.configure_npcs({make_npc(100,false)});
        if(mode==5)w.configure_npcs({make_npc(100,false,true,true)});
        auto caster_box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,caster_box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Foe",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{20,100,50,50},100);
        const auto source=*w.effect_scope(1);
        const auto player=*w.effect_scope(2);
        const auto selected=(mode>=3)?*w.effect_scope(100):player;
        check(!w.stage_ghostmirror_light_player(source,source,plan,200));
        check(!w.stage_ghostmirror_light_player(source,*w.effect_scope(3),plan,200));
        auto stale=selected;++stale.generation;
        check(!(mode>=3?w.stage_ghostmirror_light_npc(source,stale,plan,200):
                       w.stage_ghostmirror_light_player(source,stale,plan,200)));
        caster_box->drain();
        std::function<void()> publish;
        const bool accepted=mode>=3?
            w.stage_ghostmirror_light_npc(source,selected,plan,200,&publish):
            w.stage_ghostmirror_light_player(source,selected,plan,200,&publish);
        check(accepted==(mode!=2&&mode!=5));
        const auto expected_target=accepted?35u:20u;
        check(mode>=3?w.npc_combat_snapshot(selected)->health->current==expected_target:
            w.resources(2)->health==expected_target);
        check(w.resources(1)->health==((mode==1||mode==3)?35u:20u));
        const auto premature=caster_box->drain();
        check(std::find(premature.begin(),premature.end(),
            skills::particles::health_change_2022(selected.actor,1,15,100))==premature.end());
        if(accepted){
            check(bool(publish));publish();
            const auto packets=caster_box->drain();
            check(std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(selected.actor,1,15,100))!=packets.end());
            const bool caster_healed=mode==1||mode==3;
            check((std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(1,1,15,100))!=packets.end())==caster_healed);
        }
    }
    std::cout<<"PASS Ghostmirror Light player/NPC target, typed-spirit caster heal and deferred publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
