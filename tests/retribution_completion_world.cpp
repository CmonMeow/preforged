#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Retribution completion line "+std::to_string(at.line()));
}
int main()try{
    for(bool npc_target:{false,true})for(bool renewal:{false,true})for(bool deferred:{false,true}){
        auto w=std::make_unique<world::Instance>();
        if(npc_target){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={100,1000};d.damageable=true;
            d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=4;d.healing_allowed=true;d.healing_evidence="authored completion fixture";
            n.combat_definition=d;w->configure_npcs({n});}
        auto box=std::make_shared<world::Outbox>();
        w->join(1,"Caster",0,0,box,0,{}, {},100,1);w->set_vitals(1,{100,1000,10,100},100);w->public_professions(1,3,0);
        if(!npc_target){w->join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w->set_vitals(2,{100,1000,100,100},100);}
        const auto source=*w->effect_scope(1),target=*w->effect_scope(npc_target?100:2);const auto plan=*retribution(12);
        check(w->retribution_target_valid(source,target,12,100));
        check(!w->complete_retribution(source,target,plan,12,100,0).accepted);
        check(!w->complete_retribution(source,target,plan,12,100,1).accepted);
        if(renewal){const auto r=*ether_renewal(181,15);check(w->status_mutation(source,181,80,15,100,100+r.duration,true));}
        auto noncanonical=plan;++noncanonical.rank;
        check(!w->complete_retribution(source,target,noncanonical,12,100,1).accepted);
        check(!w->complete_retribution(source,target,plan,21,100,1).accepted);
        auto stale_source=source;++stale_source.generation;
        auto stale_target=target;++stale_target.generation;
        check(!w->complete_retribution(stale_source,target,plan,12,100,1).accepted);
        check(!w->complete_retribution(source,stale_target,plan,12,100,1).accepted);
        check(w->before_spell_effect(source,248,1,100));box->drain();
        check(!w->complete_retribution(source,target,plan,12,100,2).accepted);
        check(w->maintained_enchantment_count(source,100)==0);
        std::vector<world::Packet> visuals;std::function<void()> publication;
        const auto result=w->complete_retribution(source,target,plan,12,100,1,deferred?&visuals:nullptr,deferred?&publication:nullptr);
        check(result.accepted&&result.completion_prefix_published==!deferred);
        check(w->maintained_enchantment_count(source,100)==1);
        const auto entries=w->status_snapshot(target,100);check(entries&&entries->size()==1&&entries->front().skill==248);
        const auto health=npc_target?w->npc_combat_snapshot(target)->health->current:double(w->resources(2)->health);
        check(health==138);
        if(renewal){check(w->resources(1)->health==120);check(w->resources(1)->energy==10+ether_renewal(181,15)->energy_per_enchantment);}
        if(deferred){check(box->drain().empty()&&visuals.size()==1&&bool(publication));publication();check(!box->drain().empty());publication();check(box->drain().empty());}
        else check(!box->drain().empty());
        check(!w->complete_retribution(source,target,plan,12,100,1).accepted);
        check(w->maintained_enchantment_count(source,100)==1);
    }
    std::cout<<"PASS staged paid Retribution event/DF/Renewal/deferred completion\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
