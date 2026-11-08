#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace fw::gw::skills::effects;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Deny Hexes World line "+std::to_string(at.line()));
}
int main()try{
    world::Instance w;
    auto box=std::make_shared<world::Outbox>();
    for(unsigned id:{1u,2u,3u}){
        w.join(id,"Actor",float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),0,
               id==1?std::vector<world::Packet>{{165,0,1,0,0,0,3,0}}:std::vector<world::Packet>{},{},100,id==3?2:1);
        w.set_vitals(id,{id==3?100u:500u,1000,100,100},100);
    }
    check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
    const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
    check(w.status_mutation(target,100,1001,0,100,30000,true));
    check(w.status_mutation(target,19,1002,0,101,30000,true));
    check(w.apply_parasitic_bond(foe,target,0,102));
    const auto before=*w.status_snapshot(target,102);check(before.size()==3);
    skills::Slots bar{};bar[0]={991,0};bar[1]={279,0};bar[2]={194,0};
    std::array<Time,8> recharges{};recharges[0]=12000;recharges[1]=12000;recharges[2]=12000;
    box->drain();
    check(w.complete_deny_hexes(source,target,*deny_hexes(),bar,recharges,0,103));
    check(w.status_snapshot(target,103)->empty());
    const auto packets=box->drain();
    const auto body=*skills::particles::targeted_body_2022(2,1,991);
    check(std::count(packets.begin(),packets.end(),body)==1);
    for(const auto& entry:before){world::Packet end{67,0};combat::detail::put(end,2);combat::detail::put(end,entry.instance);
        check(std::count(packets.begin(),packets.end(),end)==1);}
    auto stale=target;++stale.generation;
    check(!w.complete_deny_hexes(source,stale,*deny_hexes(),bar,recharges,0,104));
    for(unsigned mode=0;mode<4;++mode){
        world::Instance other;world::Instance::Npc ally;
        ally.id=ally.definition.id=100;ally.x=100;ally.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=mode==3?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
        d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;
        d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.physical_armor=60;
        if(mode!=2){d.healing_allowed=mode!=1;d.healing_evidence="authored test";}
        ally.combat_definition=d;other.configure_npcs({ally});
        auto npc_box=std::make_shared<world::Outbox>();
        other.join(1,"Monk",0,0,npc_box,0,{{165,0,1,0,0,0,3,0}},{},100,1);
        other.set_vitals(1,{1000,1000,100,100},100);
        const auto owner=*other.effect_scope(1),recipient=*other.effect_scope(100);
        npc_box->drain();
        const bool accepted=other.complete_deny_hexes(owner,recipient,*deny_hexes(),bar,recharges,12,100);
        check(accepted==(mode<2));
        check(other.npc_combat_snapshot(recipient)->health->current==(mode==0?538:500));
        const auto out=npc_box->drain();const auto npc_body=*skills::particles::targeted_body_2022(100,1,991);
        check(std::count(out.begin(),out.end(),npc_body)==unsigned(mode<2));
    }
    std::cout<<"PASS Deny Hexes World authoritative player hex ends, NPC ally/divine guards, publication and scope guard\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
