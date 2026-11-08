#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Putrid Bile World line "+std::to_string(at.line()));
}
int main()try{
    for(bool npc:{false,true}){
        world::Instance w;
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{1000,1000,100,100},100);}
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        const auto plan=skills::effects::degeneration_hex(2058,15);
        check(plan&&plan->pips==3&&plan->duration==17000);
        check(w.degeneration_hex(caster,target,*plan,100,20));
        if(npc){
            const auto state=w.npc_combat_snapshot(target);
            check(state&&state->health->current==1000&&(state->effects&0x800));
        }else{
            const auto state=w.status_snapshot(target,100);
            check(state&&state->size()==1&&state->front().skill==2058&&
                  state->front().cause==1&&state->front().expires==17100);
        }
        auto forged=*plan;forged.pips=99;
        check(!w.degeneration_hex(caster,target,forged,200,20));
    }
    {
        world::Instance w;
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
        n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::neutral;d.spawn_health=combat::Health{1000,1000};
        d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        check(!w.degeneration_hex(*w.effect_scope(1),*w.effect_scope(100),
             *skills::effects::degeneration_hex(2058,15),100,20));
    }
    {
        world::Instance w;
        std::vector<world::Instance::Npc> npcs;
        for(unsigned id:{100u,101u,102u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==102?400.f:100.f;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{id==100?30.:200.,200};
            d.damageable=d.conditions_allowed=true;n.combat_definition=d;npcs.push_back(n);
        }
        w.configure_npcs(npcs);
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        w.join(2,"Opposing player",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(2,{200,200,100,100},100);
        const auto caster=*w.effect_scope(1),victim=*w.effect_scope(100);
        check(w.degeneration_hex(caster,victim,*skills::effects::degeneration_hex(2058,15),100,20));
        box->drain();
        check(w.iron_palm(caster,victim,*skills::effects::iron_palm(12),20,1000));
        check(w.npc_combat_snapshot(victim)->health->current==0);
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==127);
        check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==200);
        check(w.resources(2)->health==127);
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,2058))==2);
        check(!w.iron_palm(caster,victim,*skills::effects::iron_palm(12),20,1100));
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==127);
    }
    {
        world::Instance w;
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        w.join(2,"Hex victim",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(2,{30,200,100,100},100);
        w.join(3,"Nearby foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(3,{200,200,100,100},100);
        const auto caster=*w.effect_scope(1),victim=*w.effect_scope(2);
        check(w.degeneration_hex(caster,victim,*skills::effects::degeneration_hex(2058,15),100,20));
        check(w.iron_palm(caster,victim,*skills::effects::iron_palm(12),20,1000));
        check(w.resources(2)->health==0&&w.resources(3)->health==127);
    }
    for(bool expire:{false,true}){
        world::Instance w;
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        w.join(2,"Hex victim",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(2,{1000,1000,100,100},100);
        w.join(3,"Nearby foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(3,{200,200,100,100},100);
        const auto caster=*w.effect_scope(1),victim=*w.effect_scope(2);
        check(w.degeneration_hex(caster,victim,*skills::effects::degeneration_hex(2058,15),100,20));
        if(expire){w.tick(18000);check(w.kill(victim,18000));}
        else{const auto state=w.status_snapshot(victim,100);check(state&&state->size()==1);
            check(w.status_mutation(victim,2058,state->front().instance,0,200,std::nullopt,false));
            check(w.kill(victim,1000));}
        check(w.resources(3)->health==200);
    }
    {
        world::Instance w;
        std::vector<world::Instance::Npc> npcs;
        for(unsigned id:{100u,101u,102u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{id==100?30.:id==101?50.:200.,200};
            d.damageable=d.conditions_allowed=true;n.combat_definition=d;npcs.push_back(n);
        }
        w.configure_npcs(npcs);
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        const auto caster=*w.effect_scope(1);
        for(unsigned id:{100u,101u})check(w.degeneration_hex(caster,*w.effect_scope(id),
            *skills::effects::degeneration_hex(2058,15),100,20));
        check(w.iron_palm(caster,*w.effect_scope(100),*skills::effects::iron_palm(12),20,1000));
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==0);
        check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==54);
    }
    // The trusted World application must not make the unfinished death effect
    // available through ordinary paid client casting.
    skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[2058]=1;
    skills::Slots bar{};bar[0]={2058,0};attributes::Ranks ranks{};ranks[5]=15;
    skills::casting::Runtime runtime(1);skills::casting::Hooks hooks;
    hooks.events=[](const auto&){};
    check(runtime.start(100,{2058,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::unsupported);
    std::cout<<"PASS Putrid Bile trusted World hex on player/NPC; normal cast remains disabled\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
