#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC degeneration parity line "+std::to_string(at.line()));
}
combat::npcs::LiveActor npc(double hp=1000){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{hp,1000};
    d.damageable=d.conditions_allowed=true;return {d,{1,1,1,146,100}};
}
int main()try{
    unsigned cases=0;
    for(unsigned id:{99u,31u,53u,3183u,859u,109u,108u,142u,2058u})for(unsigned rank:{0u,12u,20u}){
        auto coarse=npc(),fine=npc();const auto plan=skills::effects::degeneration_hex(id,rank);check(plan.has_value());
        const auto apply=[&](auto& n){return id==99?n.apply_parasitic_bond(n.lifetime(),rank,1,7,100):
            n.apply_single_degeneration_hex(n.lifetime(),*plan,1,7,100);};
        check(apply(coarse).has_value()&&apply(fine).has_value());
        for(unsigned at=200;at<=1100;at+=100)check(fine.advance_conditions(fine.lifetime(),at).has_value());
        check(coarse.advance_conditions(coarse.lifetime(),1100).has_value());
        const double expected=1000.-2*std::min(plan->pips,10u);
        check(std::abs(coarse.actor(coarse.lifetime())->health->current-expected)<1e-8);
        check(std::abs(fine.actor(fine.lifetime())->health->current-expected)<1e-8);
        ++cases;
    }
    for(bool dense:{false,true}){
        auto n=npc(2);const auto plan=*skills::effects::degeneration_hex(2058,0);
        check(n.apply_single_degeneration_hex(n.lifetime(),plan,1,7,100).has_value());
        if(dense)for(unsigned at=200;at<=1000;at+=100)check(!n.advance_conditions(n.lifetime(),at)->killed);
        const auto death=n.advance_conditions(n.lifetime(),dense?1100:5100);
        check(death&&death->killed&&death->death_time==1100&&death->cause==1);
        check(n.actor(n.lifetime())->health->current==0);++cases;
    }
    {
        auto n=npc();const auto plan=*skills::effects::degeneration_hex(2058,0);
        check(n.apply_single_degeneration_hex(n.lifetime(),plan,1,7,100).has_value());
        n.advance_conditions(n.lifetime(),10100);check(n.actor(n.lifetime())->health->current==990);
        n.advance_conditions(n.lifetime(),20100);check(n.actor(n.lifetime())->health->current==990);
        ++cases;
    }
    std::cout<<cases<<" NPC canonical degeneration parity/death/expiry cases pass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
