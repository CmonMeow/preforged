#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace fw::gw::skills::effects;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Norn Weakness World line "+std::to_string(at.line()));
}
int main()try{
    for(bool select_npc:{false,true}){
        world::Instance w;std::vector<world::Instance::Npc> population;
        for(unsigned id:{100u,101u,102u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==102?600:110;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={1000,1000};d.damageable=true;d.conditions_allowed=id!=101;
            n.combat_definition=d;population.push_back(n);
        }
        w.configure_npcs(population);
        auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u,3u,4u,5u}){
            w.join(id,"Actor",id==1?0:id==2?100:id==3?200:id==4?120:500,0,
                   id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,
                   id==1||id==4?1:2);
            w.set_vitals(id,{1000,1000,100,100},100);
        }
        const auto source=*w.effect_scope(1),target=*w.effect_scope(select_npc?100:2);
        const auto plan=*you_are_all_weaklings(5);
        std::function<void()> publish;box->drain();
        check(w.stage_you_are_all_weaklings(source,target,plan,100,&publish));
        check(bool(publish)&&box->drain().empty());
        publish();const auto packets=box->drain();
        const auto has=[&](const auto& packet){return std::find(packets.begin(),packets.end(),packet)!=packets.end();};
        check(has(combat::detail::integer(21,1,1914)));
        for(unsigned id:{2u,3u,100u})check(has(*skills::particles::targeted_body_2022(id,1,2359)));
        for(unsigned id:{4u,5u,101u,102u})check(!has(*skills::particles::targeted_body_2022(id,1,2359)));
        check(std::count_if(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>1&&packet[0]==65&&packet[1]==0;})==2);
        auto stale=target;++stale.generation;
        check(!w.stage_you_are_all_weaklings(source,stale,plan,101));
        auto forged=plan;forged.weakness_ms=999999;
        check(!w.stage_you_are_all_weaklings(source,target,forged,101));
    }
    std::cout<<"PASS Norn Weakness mixed player/NPC area, immunity, deferred bodies and scope guards\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
