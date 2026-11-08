#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace fw::gw::skills::effects;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Dwarf World line "+std::to_string(at.line()));
}
int main()try{
    for(bool reversal:{false,true}){
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*40),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.status_mutation(target,1399,80,12,100,6100,true));if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
        check(w.stage_you_move_like_a_dwarf(source,target,*you_move_like_a_dwarf(0),20,100));
        check(w.resources(2)->health==(reversal?1000u:961u));
        check(w.stage_you_move_like_a_dwarf(source,target,*you_move_like_a_dwarf(0),20,101));
        check(w.resources(2)->health==(reversal?966u:927u));
        check(w.knocked_down(target,101)&&w.has_condition(target,combat::conditions::Kind::crippled,101));
    }

    for(unsigned health:{1000u,20u}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,100,100},100);
        w.set_vitals(2,{health,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        std::function<void()> publish;box->drain();
        check(w.stage_you_move_like_a_dwarf(source,target,*you_move_like_a_dwarf(0),20,100,&publish));
        check(bool(publish)&&box->drain().empty());
        check(w.resources(2)->health<health);
        publish();const auto packets=box->drain();check(!packets.empty());
        check(std::any_of(packets.begin(),packets.end(),[](const auto& packet){return packet.size()>1&&packet[0]==65&&packet[1]==0;}));
        check(std::any_of(packets.begin(),packets.end(),[](const auto& packet){return packet.size()>5&&packet[0]==161&&packet[1]==0&&packet[2]==63;}));
        auto stale=target;++stale.generation;
        check(!w.stage_you_move_like_a_dwarf(source,stale,*you_move_like_a_dwarf(0),20,101));
        auto forged=*you_move_like_a_dwarf(0);forged.damage=999;
        check(!w.stage_you_move_like_a_dwarf(source,target,forged,20,101));
    }
    for(bool conditions_allowed:{true,false})for(bool knockdown_allowed:{true,false}){
        world::Instance w;world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=100;npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;definition.binding.map=146;
        definition.binding.actor=definition.binding.definition=100;
        definition.relation=combat::npcs::Relation::hostile_to_players;
        definition.spawn_health=combat::Health{1000,1000};
        definition.damageable=true;definition.conditions_allowed=conditions_allowed;
        definition.knockdown_allowed=knockdown_allowed;
        definition.knockdown_evidence="test authored immunity";
        definition.earth_armor=definition.fire_armor=definition.cold_armor=
            definition.lightning_armor=definition.physical_armor=60;
        npc.combat_definition=definition;w.configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Norn",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        std::function<void()> publish;box->drain();
        check(w.stage_you_move_like_a_dwarf(source,target,*you_move_like_a_dwarf(0),20,100,&publish));
        const auto post=w.npc_combat_snapshot(target);
        check(post&&post->health->current<1000&&box->drain().empty());
        publish();const auto packets=box->drain();check(!packets.empty());
        const bool knock=std::any_of(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>5&&packet[0]==161&&packet[1]==0&&packet[2]==63;});
        check(knock==knockdown_allowed);
    }
    std::cout<<"PASS dwarf World player/NPC damage, condition, deferred publication, scope and forged plan\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
