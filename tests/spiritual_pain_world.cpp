#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Spiritual Pain World line "+
        std::to_string(at.line()));
}
static world::Instance::Npc test_npc(unsigned id,float x,std::optional<bool> summoned){
    world::Instance::Npc result;
    result.id=result.definition.id=id;
    result.x=x;result.y=0;
    result.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;
    d.binding.map=146;
    d.binding.actor=d.binding.definition=id;
    d.relation=combat::npcs::Relation::hostile_to_players;
    d.spawn_health=combat::Health{500,500};
    d.damageable=true;
    d.holy_traits.summoned=summoned;
    result.combat_definition=d;
    return result;
}
int main()try{
    const auto plan=*skills::effects::spiritual_pain(0);
    for(bool reversal:{false,true}){
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,100,100},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.status_mutation(target,1399,80,12,100,6100,true));
        if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
        check(w.stage_spiritual_pain(source,target,plan,20,100));check(w.resources(2)->health==(reversal?500u:490u));
        check(w.stage_spiritual_pain(source,target,plan,20,101));check(w.resources(2)->health==(reversal?495u:485u));
    }

    {
        world::Instance w;
        w.configure_npcs({test_npc(100,120,true),test_npc(101,130,false),
            test_npc(102,150,std::nullopt),test_npc(103,500,true)});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{500,500,100,100},100);
        w.set_vitals(2,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
        box->drain();std::function<void()> publish;
        check(w.stage_spiritual_pain(caster,foe,plan,20,200,&publish));
        check(bool(publish)&&box->drain().empty());
        check(w.resources(2)->health==485);
        check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==475);
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==500);
        check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==500);
        check(w.npc_combat_snapshot(*w.effect_scope(103))->health->current==500);
        publish();
        const auto published=box->drain();
        const auto body=*skills::particles::targeted_body_2022(foe.actor,caster.actor,1336);
        check(std::count(published.begin(),published.end(),body)==1);
        check(!published.empty()&&published.front()==body);
        publish();check(box->drain().empty());
    }
    {
        world::Instance w;
        w.configure_npcs({test_npc(100,100,true),test_npc(101,110,true),
            test_npc(102,120,false)});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),selected=*w.effect_scope(100);
        check(w.stage_spiritual_pain(caster,selected,plan,20,200));
        check(w.npc_combat_snapshot(selected)->health->current==460);
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==475);
        check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==500);
    }
    {
        world::Instance w;
        w.configure_npcs({test_npc(100,100,std::nullopt),test_npc(101,110,true)});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),selected=*w.effect_scope(100);
        check(!w.stage_spiritual_pain(caster,selected,plan,20,200));
        check(w.npc_combat_snapshot(selected)->health->current==500);
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==500);
    }
    {
        world::Instance w;
        auto dying=test_npc(100,100,true);
        dying.combat_definition->spawn_health=combat::Health{10,10};
        w.configure_npcs({dying,test_npc(101,110,true)});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),selected=*w.effect_scope(100);
        check(w.stage_spiritual_pain(caster,selected,plan,20,200));
        check(w.npc_combat_snapshot(selected)->health->current==0);
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==475);
        check(!w.stage_spiritual_pain(caster,selected,plan,20,300));
    }
    {
        world::Instance w;
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Spirit owner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{500,500,100,100},100);
        w.set_vitals(2,{500,500,100,100},100);
        const auto signet=*skills::effects::spirits::signet_plan(0);
        std::vector<world::Instance::Npc> spirits;
        for(unsigned i=0;i<3;++i){
            auto entry=test_npc(200+i,100+float(i*30),std::nullopt);
            entry.definition.model_file=1234;
            entry.definition.level=std::uint8_t(signet.level);
            entry.combat_definition->relation=combat::npcs::Relation::allied_to_players;
            entry.combat_definition->binding.model_file=1234;
            entry.combat_definition->binding.level=entry.definition.level;
            entry.combat_definition->spawn_health=combat::Health{500,500};
            entry.combat_definition->healing_allowed=false;
            entry.combat_definition->healing_evidence="archived spirit mechanics";
            entry.combat_definition->fleshy=false;
            entry.combat_definition->fleshy_evidence="archived spirit mechanics";
            spirits.push_back(entry);
        }
        std::vector<world::Packet> spawn_packets;
        const auto created=w.spawn_controlled_spirits(*w.effect_scope(2),
            signet,spirits,100,spawn_packets);
        check(created&&created->size()==3);
        const auto caster=*w.effect_scope(1);
        check(!w.stage_spiritual_pain(*w.effect_scope(2),(*created)[0],plan,20,200));
        check(w.npc_combat_snapshot((*created)[0])->health->current==500);
        check(w.stage_spiritual_pain(caster,(*created)[0],plan,20,200));
        check(w.npc_combat_snapshot((*created)[0])->health->current==460);
        check(w.npc_combat_snapshot((*created)[1])->health->current==475);
    }
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w;
        w.configure_npcs({test_npc(100,120,true)});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{500,500,100,100},100);
        w.set_vitals(2,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1);
        skills::casting::Runtime runtime(1);
        skills::casting::Hooks hooks;
        std::vector<world::Packet> events;
        hooks.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.consume_energy_at=[&](unsigned cost,auto at){
            check(cost==5);return w.consume_energy(1,cost,caster,at);
        };
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.projectile_target_valid=[&](auto id,float range){
            return w.projectile_target_valid(caster,id,range,1000);
        };
        if(mode!=2)hooks.spiritual_pain=[&](auto target,const auto& selected,auto at)
                ->skills::casting::EffectCompletion{
            std::function<void()> publication;
            const bool accepted=w.stage_spiritual_pain(caster,target,selected,20,at,&publication);
            return {accepted,{}, {},std::move(publication),false};
        };
        skills::Policy policy;policy.primary=5;policy.campaigns=255;
        policy.learned[1336]=1;
        skills::Slots bar{};bar[0]={1336,0};
        attributes::Ranks ranks{};
        const auto admitted=runtime.start(1000,{1336,0,2},bar,policy,ranks,true,hooks);
        check(admitted==(mode==2?skills::casting::Reason::unsupported:
            skills::casting::Reason::accepted));
        if(mode==2){check(w.resources(1)->energy==100);continue;}
        check(w.resources(1)->energy==95&&w.resources(2)->health==500);
        if(mode==1)runtime.cancel(hooks);
        if(mode==3){
            w.leave(2);
            w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{500,500,100,100},100);
        }
        runtime.update(1999,hooks);
        check(w.resources(2)->health==500);
        box->drain();runtime.update(2000,hooks);
        if(mode==1||mode==3){check(w.resources(2)->health==500);continue;}
        check(w.resources(2)->health==485);
        check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==475);
        const auto body=*skills::particles::targeted_body_2022(2,1,1336);
        const auto world_packets=box->drain();
        check(!world_packets.empty()&&world_packets.front()==body);
        check(std::count(events.begin(),events.end(),skills::casting::terminal(58,1))==1);
        check(std::count(events.begin(),events.end(),skills::casting::slot_event(228,1,{1336,0},7))==1);
        check(runtime.start(3000,{1336,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge);
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
