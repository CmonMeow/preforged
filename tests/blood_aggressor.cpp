#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Blood Aggressor line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;effects::Scope source,target;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(bool npc,bool conditions=true,unsigned health=500){
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),500};d.damageable=true;d.conditions_allowed=conditions;
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Necromancer",0,0,box,0,{},0,100,1);w.set_vitals(1,{100,500,100,100},100);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{},0,100,2);w.set_vitals(2,{health,500,100,100},100);
        source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
    }
    double health(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
    void action(unsigned mode){
        //0idle,1attack windup,2spell,3completed attack,4exact attack end.
        if(!mode)return;
        const auto until=mode>=3?1000u:2000u;
        if(target.actor==100){
            const auto token=w.begin_npc_action(target,mode==2?combat::npcs::control::Action::spell:combat::npcs::control::Action::attack,200,until);
            check(bool(token));if(mode==3)check(w.complete_npc_action(*token,1000));
        }else if(mode==2)check(w.cast_activity(target,194,200,2000));
        else {check(w.attack_activity(target,until,200));if(mode==3)check(w.attack_activity(target,{},1000));}
    }
};
int main()try{
    using combat::conditions::Kind;
    check(!effects::life_steal(902,21));
    for(bool npc:{false,true})for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<5;++mode){
        Fixture f(npc);f.action(mode);const auto plan=*effects::life_steal(902,rank);
        check(plan.amount==effects::interpolate(5,45,rank)&&plan.weakness_if_attacking_ms==effects::interpolate(3,12,rank)*1000);
        std::vector<world::Packet> visuals;check(f.w.life_steal(f.source,f.target,plan,1000,&visuals));
        check(f.health()==500-plan.amount&&f.w.resources(1)->health==100+plan.amount);
        check(f.w.has_condition(f.target,Kind::weakness,1000)==(mode==1));
        check(visuals==std::vector<world::Packet>{*particles::targeted_body_2022(f.target.actor,1,902)});
        if(mode==1){check(f.w.has_condition(f.target,Kind::weakness,999+plan.weakness_if_attacking_ms));check(!f.w.has_condition(f.target,Kind::weakness,1000+plan.weakness_if_attacking_ms));}
    }
    {Fixture f(true,false);f.action(1);check(f.w.life_steal(f.source,f.target,*effects::life_steal(902,12),1000));
        check(f.health()==463&&!f.w.has_condition(f.target,Kind::weakness,1000));}
    for(bool npc:{false,true}){
        Fixture f(npc);f.action(1);
        check(f.w.life_steal(f.source,f.target,*effects::life_steal(902,15),1000));
        check(f.w.life_steal(f.source,f.target,*effects::life_steal(902,0),1000));
        check(f.w.has_condition(f.target,Kind::weakness,12999)&&!f.w.has_condition(f.target,Kind::weakness,13000));
    }
    for(bool npc:{false,true})for(bool lethal:{false,true}){
        Fixture f(npc,true,lethal?4:500);f.action(1);
        check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),1000));
        check(f.w.life_steal(f.source,f.target,*effects::life_steal(902,12),1000));
        check(f.health()==(lethal?0:446)&&f.w.resources(1)->health==(lethal?105u:137u));
        check(f.w.has_condition(f.target,Kind::weakness,1000)==!lethal);
    }
    for(bool npc:{false,true})for(unsigned mode=0;mode<3;++mode){
        Fixture f(npc);f.action(1);auto target=f.target;auto plan=*effects::life_steal(902,12);
        if(mode==0)++target.generation;if(mode==1)++plan.weakness_if_attacking_ms;if(mode==2)target=f.source;
        check(!f.w.life_steal(f.source,target,plan,1000));
        check(f.health()==500&&f.w.resources(1)->health==100&&!f.w.has_condition(f.target,Kind::weakness,1000));
    }
    for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
        Fixture f(npc);if(mode!=5)f.action(1);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,payments=0;
        std::vector<world::Packet> events;
        h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
        h.consume_energy_at=[&](unsigned cost,auto at){++payments;check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
        h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return id==f.target.actor&&range==1248;};
        h.life_steal=[](auto,const auto&,auto){throw std::runtime_error("plain hook bypassed conditional completion");return false;};
        if(mode==0)h.life_steal={}; // The dedicated completion owns this skill.
        if(mode!=3)h.complete_life_steal=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
            ++calls;std::vector<world::Packet> visuals;const bool accepted=f.w.life_steal(f.source,target,plan,at,&visuals);return {accepted,std::move(visuals)};};
        Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[902]=1;Slots bar{};bar[0]={902,0};attributes::Ranks ranks{};ranks[4]=12;
        check(runtime.start(1000,{902,0,f.target.actor},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
        if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1500,h);if(mode==4)h.complete_life_steal={};
        if(mode==5){
            // A target idle when casting began starts attacking during the
            // cast. Completion must observe its new action, not cached intent.
            if(npc)check(bool(f.w.begin_npc_action(f.target,combat::npcs::control::Action::attack,1500,3000)));
            else check(f.w.attack_activity(f.target,3000,1500));
        }
        runtime.update(1999,h);check(!calls);runtime.update(2000,h);
        check(calls==unsigned(mode==0||mode==5)&&payments==unsigned(mode!=3));check(f.health()==(mode==0||mode==5?463:500));
        // The attack ended exactly when the spell completed: no Weakness.
        check(f.w.has_condition(f.target,Kind::weakness,2000)==(mode==5));
        if(mode==0||mode==5){
            const auto finish=casting::generic(58,1,0);const auto target=*particles::targeted_body_2022(f.target.actor,1,902);
            std::vector<world::Packet> actual;for(const auto& packet:events)if(packet==finish||packet==target)actual.push_back(packet);
            check(actual==std::vector<world::Packet>{finish,target});check(!casting::skill_success_body_effect(1,902));
            runtime.update(2000,h);check(calls==1&&payments==1);
        }
    }
    std::cout<<"Blood Aggressor rank/attack boundaries,player/NPC life steal/Weakness,reactions and cast controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
