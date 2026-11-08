#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Wandering Eye cast line "+std::to_string(at.line()));
}
int main()try{
    for(bool npc_target:{false,true})for(bool pvp:{false,true}){
        world::Instance world;
        if(npc_target){
            world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=60;
            npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition body;body.binding.map=146;
            body.binding.actor=body.binding.definition=100;
            body.relation=combat::npcs::Relation::hostile_to_players;
            body.spawn_health=combat::Health{500,500};body.damageable=true;
            npc.combat_definition=body;world.configure_npcs({npc});
        }
        auto box=std::make_shared<world::Outbox>();
        world.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
        world.set_vitals(1,{500,500,50,100},100);
        if(!npc_target){
            world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            world.set_vitals(2,{500,500,50,100},100);
        }
        const auto caster=*world.effect_scope(1),target=*world.effect_scope(npc_target?100:2);
        skills::casting::Hooks hooks;std::vector<world::Packet> sent;unsigned commits=0;
        hooks.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};
        hooks.resources=[&](unsigned id){return world.resources(id);};
        hooks.consume_energy_at=[&](unsigned cost,auto now){check(cost==5);return world.consume_energy(1,cost,caster,now);};
        hooks.target_scope=[&](unsigned id){return world.effect_scope(id);};
        hooks.projectile_target_valid=[&](unsigned id,float range){return id==target.actor&&range==1248.f;};
        hooks.before_spell_effect=[&](auto skill,auto event,auto now){return world.before_spell_effect(caster,skill,event,now);};
        hooks.wandering_eye=[&](auto owner,const auto& plan,auto now){++commits;
            check(owner==target&&plan.rank==12&&plan.damage==94);
            return world.apply_wandering_eye(caster,owner,plan.rank,now);};
        skills::Policy policy;policy.primary=5;policy.pvp_match=pvp;policy.campaigns=255;
        policy.learned[2056]=1;skills::Slots bar{};bar[0]={2056,0};
        attributes::Ranks ranks{};ranks[1]=12;
        skills::casting::Runtime cast(1);
        const auto result=cast.start(100,{2056,0,target.actor},bar,policy,ranks,true,hooks);
        check(result==(pvp?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
        if(pvp){check(world.resources(1)->energy==50&&!cast.active()&&!commits);continue;}
        check(world.resources(1)->energy==45&&cast.active());
        check(std::count(sent.begin(),sent.end(),skills::casting::activation(1,target.actor,2056))==1);
        box->drain();cast.update(2099,hooks);check(!commits);
        cast.update(2100,hooks);check(!cast.active()&&commits==1);
        const auto status=*world.status_snapshot(target,2100);
        check(status.size()==1&&status[0].skill==2056&&status[0].expires==6100);
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(target.actor,1,2056))==1);
        check(cast.start(3000,{2056,0,target.actor},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge);
    }
    std::cout<<"PASS paid Wandering Eye PvE player/NPC casts, 2022 body and PvP gate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
