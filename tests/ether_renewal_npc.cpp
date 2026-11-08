#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Ether Renewal NPC line "+std::to_string(at.line()));
}
int main()try{
    using namespace combat;using namespace combat::npcs;using namespace skills::effects;
    const Lifetime life{7,2,3,146,100};
    for(auto relation:{Relation::hostile_to_players,Relation::allied_to_players})
    for(bool healing:{false,true})for(unsigned version:{181u,2860u}){
        Definition d;d.binding.actor=100;d.binding.map=146;d.spawn_health=Health{100,1000};
        d.relation=relation;d.damageable=true;d.spawn_energy=Energy{10,30};d.energy_pips=0;
        d.energy_evidence=d.energy_pips_evidence="authored test";
        d.healing_allowed=healing;d.healing_evidence="authored test";
        LiveActor actor(d,life);const auto plan=*ether_renewal(version,15);
        check(actor.apply_ether_renewal(life,plan,80,100));
        check(actor.apply_stoneflesh_aura(life,*stoneflesh_aura(0),81,100));
        const auto token=actor.begin_action(life,control::Action::spell,100,200,skills::Slot{174,0});
        check(bool(token));
        check(!actor.complete_ether_renewal_action(life,*token,199));
        auto forged=*token;++forged.sequence;
        check(!actor.complete_ether_renewal_action(life,forged,200));
        check(actor.actor(life)->health->current==100&&actor.energy(life)->current==10);
        const auto result=actor.complete_ether_renewal_action(life,*token,200);
        check(result&&result->reward.enchantments==2);
        check(result->after.current==(healing?140:100));
        check(result->energy_after.current==10+2*plan.energy_per_enchantment);
        check(!actor.complete_ether_renewal_action(life,*token,200));
        // A non-spell action retains its control token after rejected reward.
        const auto attack=actor.begin_action(life,control::Action::attack,200,300,skills::Slot{322,0});
        check(attack&&!actor.complete_ether_renewal_action(life,*attack,300));
        check(actor.complete_action(life,*attack,300));
        check(actor.remove_reviewed_enchantment(life,80,300));
        const auto next=actor.begin_action(life,control::Action::spell,300,400,skills::Slot{174,0});
        check(next&&!actor.complete_ether_renewal_action(life,*next,400));
        check(actor.complete_action(life,*next,400));
    }
    for(unsigned mode=0;mode<3;++mode){
        Definition d;d.binding.actor=100;d.binding.map=146;d.spawn_health=Health{999,1000};
        d.relation=Relation::hostile_to_players;d.damageable=true;d.spawn_energy=Energy{29,30};
        if(mode!=1)d.energy_pips=0;
        d.energy_evidence=d.energy_pips_evidence="authored test";
        d.healing_allowed=true;d.healing_evidence="authored test";
        LiveActor actor(d,life);check(actor.apply_ether_renewal(life,*ether_renewal(181,0),80,100));
        const auto ends=mode==2?5100u:200u;
        const auto token=actor.begin_action(life,control::Action::spell,100,ends,skills::Slot{174,0});
        check(bool(token));
        const auto receipt=actor.complete_ether_renewal_action(life,*token,ends);
        if(mode==0){check(receipt&&receipt->after.current==1000&&receipt->energy_after.current==30);}
        else{check(!receipt&&actor.actor(life)->health->current==999&&actor.energy(life)->current==29);
            check(actor.complete_action(life,*token,ends));}
    }
    std::cout<<"PASS Ether Renewal NPC completion, relations, immunity, capacities and duplicate rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
