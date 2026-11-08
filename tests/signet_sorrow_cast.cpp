#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Signet Sorrow cast line "+std::to_string(at.line()));
}
int main()try{
    for(bool corpse_near:{false,true})for(bool hook_present:{false,true}){
        world::Instance w;std::vector<world::Instance::Npc> npcs;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc npc;npc.id=npc.definition.id=id;
            npc.definition.visual_adjustment={0,0,0,100};npc.x=id==100?300:500;
            combat::npcs::Definition definition;definition.binding.map=146;
            definition.binding.actor=definition.binding.definition=id;
            definition.relation=combat::npcs::Relation::hostile_to_players;
            definition.spawn_health=combat::Health{100,100};definition.damageable=true;
            if(id==101){definition.fleshy=true;definition.fleshy_evidence="test authored";}
            npc.combat_definition=definition;npcs.push_back(npc);
        }
        w.configure_npcs(npcs);
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,4);
        w.set_vitals(1,{500,500,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        if(corpse_near)check(w.damage_npc(source,*w.effect_scope(101),
            {combat::Kind::armor_ignoring,1000,20,194},1248,500).killed);
        skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[1363]=1;
        skills::Slots bar{};bar[0]={1363,0};attributes::Ranks ranks{};ranks[6]=12;
        skills::casting::Hooks hooks;std::vector<world::Packet> events;
        std::uint64_t at=1000;
        hooks.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());w.skill_event(1,batch,at);};
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.consume_energy_at=[&](unsigned cost,auto now){check(cost==0);return w.consume_energy(1,cost,source,now);};
        hooks.projectile_target_valid=[&](unsigned id,float range){return w.projectile_target_valid(source,id,range,at);};
        unsigned committed=0;
        if(hook_present)hooks.signet_sorrow=[&](auto selected,const auto& plan,auto now)->skills::casting::SignetSorrowCompletion{
            ++committed;
            auto result=w.complete_signet_sorrow(source,selected,plan,20,now);
            return {{result.accepted,std::move(result.visuals),{},
                std::move(result.publication),true},result.recharge_ms};
        };
        skills::casting::Runtime runtime(1);
        check(runtime.start(at,{1363,0,100},bar,policy,ranks,true,hooks)==
              (hook_present?skills::casting::Reason::accepted:skills::casting::Reason::unsupported));
        if(!hook_present)continue;
        box->drain();
        at=2000;runtime.update(1999,hooks);
        check(w.npc_combat_snapshot(target)->health->current==100);
        runtime.update(at,hooks);
        check(committed==1&&w.npc_combat_snapshot(target)->health->current==37);
        const auto terminal=std::find(events.begin(),events.end(),skills::casting::terminal(58,1));
        const auto caster_body=std::find(events.begin(),events.end(),
            combat::detail::integer(21,1,1619));
        const auto capacity_in_visuals=std::find(events.begin(),events.end(),
            combat::detail::integer(42,100,100));
        check(terminal<caster_body&&caster_body!=events.end()&&capacity_in_visuals==events.end());
        const auto wire=box->drain();
        const auto wire_finish=std::find(wire.begin(),wire.end(),skills::casting::terminal(58,1));
        const auto wire_caster=std::find(wire.begin(),wire.end(),
            combat::detail::integer(21,1,1619));
        const auto wire_target=std::find(wire.begin(),wire.end(),
            combat::detail::integer(42,100,100));
        const auto wire_damage=std::find(wire.begin(),wire.end(),combat::detail::integer(10,1,1363));
        check(wire_finish<wire_caster&&wire_caster<wire_target&&
              wire_target<wire_damage&&wire_damage!=wire.end());
        at=2750;
        check(runtime.start(at,{1363,0,100},bar,policy,ranks,true,hooks)==
              (corpse_near?skills::casting::Reason::accepted:skills::casting::Reason::recharge));
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
