#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <string_view>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Ash Blast cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{},0,100,1);
        w.join(2,"Target",100,0,std::make_shared<world::Outbox>(),0,{},0,100,2);
        w.set_vitals(1,{500,500,100,100},100);w.set_vitals(2,{500,500,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.combat_armor(target,60));
        box->drain();
        skills::casting::Runtime runtime(1);skills::casting::Hooks h;
        std::vector<world::Packet> events;
        h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
        h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy_at=[&](unsigned cost,auto at){check(cost==5);return w.consume_energy(1,cost,source,at);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(source,id,range,1000);};
        if(mode!=3)h.ash_blast=[&](auto foe,const auto& plan,auto at)->skills::casting::EffectCompletion{
            std::function<void()> publication;
            const bool accepted=w.stage_ash_blast(source,foe,plan,20,at,&publication);
            std::vector<world::Packet> visuals;
            if(accepted)visuals.push_back(*skills::particles::targeted_body_2022(foe.actor,source.actor,1085));
            return {accepted,std::move(visuals),{},std::move(publication),true};
        };
        skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1085]=1;
        skills::Slots bar{};bar[0]={1085,0};attributes::Ranks ranks{};ranks[9]=0;
        const auto admission=runtime.start(1000,{1085,0,2},bar,policy,ranks,true,h);
        check(admission==(mode==3?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
        if(mode==3){check(w.resources(1)->energy==100);continue;}
        check(w.resources(1)->energy==95&&w.resources(2)->health==500);
        if(mode==1)runtime.cancel(h);
        if(mode==2)check(w.kill(target,1500));
        runtime.update(1999,h);check(w.resources(2)->health==(mode==2?0u:500u));
        if(mode==0)check(w.apply_condition(source,target,combat::conditions::Kind::burning,1999,5000));
        runtime.update(2000,h);
        if(mode){check(w.resources(2)->health==(mode==2?0u:500u));continue;}
        check(w.resources(2)->health==465);
        const auto effects=w.status_snapshot(target,2000);
        check(effects&&std::any_of(effects->begin(),effects->end(),[](const auto& e){return e.skill==1085;}));
        const auto caster_body=*skills::casting::skill_success_body_effect(1,1085);
        const auto target_body=*skills::particles::targeted_body_2022(2,1,1085);
        const auto fixed_packet=[](std::string_view hex){world::Packet out;
            const auto digit=[](char c)->unsigned{return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
            for(std::size_t i=0;i<hex.size();i+=2)out.push_back(static_cast<std::uint8_t>((digit(hex[i])<<4)|digit(hex[i+1])));
            return out;
        };
        check(caster_body==fixed_packet("9e001500000001000000a1040000"));
        check(target_body==fixed_packet("9f00140000000200000001000000a2040000"));
        const auto body=std::find(events.begin(),events.end(),caster_body);
        check(body!=events.end()&&body+1<events.end()&&body[1]==target_body);
        check(std::find(events.begin(),body,skills::casting::terminal(58,1))!=body);
        check(std::count(events.begin(),events.end(),caster_body)==1);
        check(!box->drain().empty());
        check(runtime.start(2001,{1085,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::busy);
    }
    {
        world::Instance w;world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=100;npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition profile;profile.binding.map=146;
        profile.binding.actor=profile.binding.definition=100;
        profile.relation=combat::npcs::Relation::hostile_to_players;
        profile.spawn_health=combat::Health{500,500};profile.earth_armor=60;
        profile.damageable=profile.conditions_allowed=true;npc.combat_definition=profile;
        w.configure_npcs({npc});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{},0,100,1);
        w.set_vitals(1,{500,500,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        skills::casting::Runtime runtime(1);skills::casting::Hooks h;
        std::vector<world::Packet> events;
        h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
        h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,source,at);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(source,id,range,1000);};
        h.ash_blast=[&](auto foe,const auto& plan,auto at)->skills::casting::EffectCompletion{
            std::function<void()> publication;
            const bool accepted=w.stage_ash_blast(source,foe,plan,20,at,&publication);
            std::vector<world::Packet> visuals;
            if(accepted)visuals.push_back(*skills::particles::targeted_body_2022(foe.actor,source.actor,1085));
            return {accepted,std::move(visuals),{},std::move(publication),true};
        };
        skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1085]=1;
        skills::Slots bar{};bar[0]={1085,0};attributes::Ranks ranks{};
        check(runtime.start(1000,{1085,0,100},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
        runtime.update(2000,h);
        check(w.npc_combat_snapshot(target)->health->current==465);
        check(std::find(events.begin(),events.end(),*skills::particles::targeted_body_2022(100,1,1085))!=events.end());
    }
    std::cout<<"PASS Ash Blast paid cast, native body order, burning hex, NPC target, cancel and stale target\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
