#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>

using namespace fw::gw;
using namespace skills;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Slippery Ground cast line "+std::to_string(at.line()));
}
int main()try{
    world::Instance w;
    auto out=std::make_shared<world::Outbox>();
    w.join(1,"Caster",0,0,out,0,{}, {},100,1);
    w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    w.set_vitals(1,{500,500,100,100},100);
    w.set_vitals(2,{500,500,100,100},100);
    const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
    check(w.blindness(source,target,*effects::blindness(220,12),100));
    Slots bar{};bar[0]={2191,0};
    Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[2191]=1;
    attributes::Ranks ranks{};ranks[11]=12;
    casting::Runtime cast(1);casting::Hooks h;
    std::vector<world::Packet> sent;unsigned commits=0,payments=0;
    h.resources=[&](auto id){return w.resources(id);};
    h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};
    h.consume_energy_at=[&](unsigned cost,auto at){check(cost==5);++payments;return w.consume_energy(1,cost,source,at);};
    h.target_scope=[&](auto id){return w.effect_scope(id);};
    h.projectile_target_valid=[&](auto id,float range){return id==2&&range==1248.f;};
    h.before_spell_effect=[&](auto id,auto event,auto at){return w.before_spell_effect(source,id,event,at);};
    h.slippery_ground=[&](auto selected,const auto& plan,auto at){
        ++commits;check(selected==target&&plan==*effects::slippery_ground(12));
        casting::EffectCompletion result;
        result.accepted=w.slippery_ground(source,selected,plan,at,&result.visuals);
        return result;
    };
    check(casting::Runtime::effect_supported(2191));
    auto missing=h;missing.slippery_ground={};
    check(cast.start(1000,{2191,0,2},bar,policy,ranks,true,missing)==casting::Reason::unsupported);
    check(w.resources(1)->energy==100);
    check(cast.start(1000,{2191,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
    check(w.resources(1)->energy==95);
    cast.update(1999,h);check(commits==0&&!w.knocked_down(target,1999));
    cast.update(2000,h);check(commits==1&&w.knocked_down(target,2000));
    check(std::count(sent.begin(),sent.end(),casting::terminal(58,1))==1);
    check(std::count(sent.begin(),sent.end(),*particles::target_body_2022(2,2191))==1);
    check(std::count(sent.begin(),sent.end(),combat::knockdown_2022(2,2))==1);
    check(std::find(sent.begin(),sent.end(),casting::terminal(58,1))<
          std::find(sent.begin(),sent.end(),*particles::target_body_2022(2,2191)));
    check(cast.start(2750,{2191,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
    casting::Runtime stale(1);
    check(stale.start(5000,{2191,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
    auto lost=h;lost.target_scope=[&](auto id)->std::optional<effects::Scope>{
        auto scope=w.effect_scope(id);if(scope&&id==2)++scope->generation;return scope;
    };
    stale.update(6000,lost);
    check(commits==1&&payments==2);
    {
        world::Instance npc_world;
        world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=100;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;
        definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
        definition.relation=combat::npcs::Relation::hostile_to_players;
        definition.spawn_health=combat::Health{500,500};
        definition.damageable=definition.conditions_allowed=true;
        definition.knockdown_allowed=true;definition.knockdown_evidence="test";
        npc.combat_definition=definition;npc_world.configure_npcs({npc});
        npc_world.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        npc_world.set_vitals(1,{500,500,100,100},100);
        const auto owner=*npc_world.effect_scope(1),victim=*npc_world.effect_scope(100);
        check(npc_world.blindness(owner,victim,*effects::blindness(220,12),100));
        casting::Hooks npc_hooks;
        npc_hooks.resources=[&](auto id){return npc_world.resources(id);};
        npc_hooks.events=[](const auto&){};
        npc_hooks.consume_energy_at=[&](unsigned amount,auto at){return npc_world.consume_energy(1,amount,owner,at);};
        npc_hooks.target_scope=[&](auto id){return npc_world.effect_scope(id);};
        npc_hooks.projectile_target_valid=[](auto id,float range){return id==100&&range==1248.f;};
        npc_hooks.before_spell_effect=[&](auto skill,auto event,auto at){return npc_world.before_spell_effect(owner,skill,event,at);};
        npc_hooks.slippery_ground=[&](auto selected,const auto& plan,auto at){
            casting::EffectCompletion result;
            result.accepted=npc_world.slippery_ground(owner,selected,plan,at,&result.visuals);
            return result;
        };
        casting::Runtime npc_cast(1);
        check(npc_cast.start(1000,{2191,0,100},bar,policy,ranks,true,npc_hooks)==casting::Reason::accepted);
        npc_cast.update(2000,npc_hooks);
        check(npc_world.knocked_down(victim,2000));
    }
    std::cout<<"PASS Slippery Ground paid cast, one-second completion, World KD, target body and recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
