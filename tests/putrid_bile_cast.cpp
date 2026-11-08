#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;using namespace skills;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Putrid Bile cast line "+std::to_string(at.line()));
}
int main()try{
    for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){
        if(npc&&mode==7)continue;
        world::Instance w;
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
            n.combat_definition=d;w.configure_npcs({n});}
        w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,mode==6?9u:50u,100},100);
        if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==7?1:2);
            w.set_vitals(2,{1000,1000,50,100},100);}
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[2058]=1;
        Slots bar{};bar[0]={2058,0};attributes::Ranks ranks{};ranks[5]=15;
        casting::Hooks hooks;std::vector<Packet> packets;unsigned payments=0,calls=0;
        bool stale=false;std::uint64_t at=500;
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.target_scope=[&](auto id)->std::optional<Scope>{if(stale&&id==target.actor)return {};return w.effect_scope(id);};
        hooks.projectile_target_valid=[&](auto id,auto range){return w.projectile_target_valid(caster,id,range,at);};
        hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        hooks.consume_energy_at=[&](auto cost,auto when){++payments;check(cost==10);return w.consume_energy(1,cost,caster,when);};
        if(mode!=4)hooks.degeneration_hex=[&](auto selected,const auto& plan,auto when){
            ++calls;std::vector<Packet> visuals;
            const bool applied=w.degeneration_hex(caster,selected,plan,when,20,&visuals);
            return casting::EffectCompletion{applied,std::move(visuals)};};
        hooks.before_spell_effect=[&](auto skill,auto event,auto when){return w.before_spell_effect(caster,skill,event,when);};
        hooks.completed_action=[&](auto skill,auto when){return w.completed_skill_action(caster,skill,when);};
        casting::Runtime runtime(1);
        const auto admitted=runtime.start(at,{2058,0,target.actor},bar,policy,ranks,true,hooks);
        check(admitted==(mode==4?casting::Reason::unsupported:mode==7?casting::Reason::target:
                          mode==6?casting::Reason::energy:casting::Reason::accepted));
        if(mode==4||mode==6||mode==7){check(!calls);continue;}
        check(payments==1&&w.resources(1)->energy==40);
        if(mode==1)runtime.cancel(hooks,900);
        if(mode==2)runtime.interrupt(900,hooks);
        if(mode==3)stale=true;
        if(mode==5)check(w.kill(caster,900));
        at=1500;runtime.update(1499,hooks);check(!calls);runtime.update(1500,hooks);
        const bool applied=mode==0;
        if(calls!=unsigned(applied))throw std::runtime_error("effect call mismatch npc="+std::to_string(npc)+" mode="+std::to_string(mode)+" calls="+std::to_string(calls));
        const auto visual=*particles::targeted_body_2022(target.actor,caster.actor,2058);
        check(std::count(packets.begin(),packets.end(),visual)==unsigned(applied));
        if(applied){
            check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<
                  std::find(packets.begin(),packets.end(),visual));
            if(npc)check(w.npc_combat_snapshot(target)->effects&0x800);
            else check(w.status_snapshot(target,1500)->front().skill==2058);
            check(runtime.start(2249,{2058,0,target.actor},bar,policy,ranks,true,hooks)==casting::Reason::busy);
        }
    }
    std::cout<<"PASS Putrid Bile paid player/NPC cast, target1728 order, cancel/interrupt/stale/death gates\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
