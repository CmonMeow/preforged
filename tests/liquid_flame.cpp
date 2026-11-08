#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Liquid Flame line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source;
    Fixture(unsigned unknown=0,unsigned primary_health=1000){
        std::vector<world::Instance::Npc> population;
        for(unsigned id=100;id<=105;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;
            n.x=id==101?1252:id==102?1253:1000;n.plane=id==105?1:0;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
            d.relation=id==104?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{id==100?double(primary_health):1000.,1000};d.damageable=d.conditions_allowed=id!=104;
            if(id!=unknown)d.fire_armor=60;n.combat_definition=d;population.push_back(n);}
        w.configure_npcs(population);
        for(unsigned id=1;id<=6;++id){w.join(id,"Actor",id==1?0:id==3?1252:id==4?1253:1000,0,
                id==1?box:std::make_shared<world::Outbox>(),0,{},0,100,id==1||id==6?1:2);
            w.set_vitals(id,{id==2?primary_health:1000,1000,100,100},100);
            check(w.elemental_armor(*w.effect_scope(id),id==unknown?std::nullopt:std::optional<int>{60},60));}
        source=scope(1);box->drain();
    }
    effects::Scope scope(unsigned id){return *w.effect_scope(id);}
    double health(unsigned id){return id>=100?w.npc_combat_snapshot(scope(id))->health->current:double(w.resources(id)->health);}
    //0idle,1ordinary attack,2spell,3signet,4glyph,5attack skill,
    //6unknown NPC skill,7completed attack,8exact-end spell.
    void action(unsigned id,unsigned mode,std::uint64_t begins=200,std::uint64_t ends=2000){
        if(!mode)return;
        if(mode>=7)ends=1000;
        const unsigned skill=mode==3?1:mode==4?200:mode==5?354:281;
        if(id>=100){const bool attack=mode==1||mode==7;std::optional<Slot> slot;
            if(!attack&&mode!=6)slot=Slot{skill,0};
            const auto token=w.begin_npc_action(scope(id),attack?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell,begins,ends,slot);
            check(bool(token));if(mode==7)check(w.complete_npc_action(*token,1000));
        }else if(mode==1||mode==7){check(w.attack_activity(scope(id),ends,begins));if(mode==7)check(w.attack_activity(scope(id),{},1000));}
        else check(w.cast_activity(scope(id),skill,begins,ends));
    }
};
int main()try{
    using effects::liquid_flame_spreads;
    check(liquid_flame_spreads({})==false&&liquid_flame_spreads({},true)==true);
    check(liquid_flame_spreads(Slot{281,0})==true&&liquid_flame_spreads(Slot{354,0})==true);
    check(liquid_flame_spreads(Slot{1,0})==false&&liquid_flame_spreads(Slot{200,0})==false);
    check(!liquid_flame_spreads(Slot{})&&!liquid_flame_spreads(Slot{99999,0})&&!liquid_flame_spreads(Slot{281,65536}));
    check(!effects::elemental(845,21));
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=*effects::elemental(845,rank);check(plan.radius==252&&plan.area_if_target_active&&plan.kind==combat::Kind::fire&&plan.damage==7+7*rank);
        combat::Actor actor{2,1,combat::Health{1000,1000},{},0,true,{},60,60};
        const auto result=effects::elemental_target(plan,actor,{},20,1,1000);
        check(result.accepted&&result.hits.size()==1&&result.hits[0].damage==7+7*rank&&!result.interrupt&&result.additions.empty());
    }
    for(bool npc:{false,true})for(unsigned mode=0;mode<9;++mode){
        if(!npc&&mode==6)continue;
        Fixture f;const auto target=f.scope(npc?100:2);f.action(target.actor,mode);
        const bool spreads=mode==1||mode==2||mode==5;
        std::vector<world::Packet> visuals;check(f.w.elemental(f.source,target,*effects::elemental(845,12),20,1000,&visuals)==(mode!=6));
        for(unsigned id:{2u,3u,5u,100u,101u,103u}){
            const bool hit=mode!=6&&(spreads||id==target.actor);check(f.health(id)==(hit?909:1000));}
        for(unsigned id:{1u,4u,6u,102u,104u,105u})check(f.health(id)==1000);
        check(visuals.size()==(mode==6?0u:spreads?6u:1u));
        if(mode!=6)check(std::count(visuals.begin(),visuals.end(),*particles::targeted_body_2022(target.actor,1,845))==1);
    }
    // An active bystander cannot turn an idle primary's single hit into AoE.
    {Fixture f;f.action(100,1);check(f.w.elemental(f.source,f.scope(2),*effects::elemental(845,12),20,1000));check(f.health(2)==909&&f.health(100)==1000);}
    for(unsigned unknown:{3u,101u})for(bool active:{false,true}){
        Fixture f(unknown);if(active)f.action(2,1);std::vector<world::Packet> visuals;
        check(f.w.elemental(f.source,f.scope(2),*effects::elemental(845,12),20,1000,&visuals)==!active);
        check(f.health(2)==(active?1000:909));for(unsigned id:{3u,5u,100u,101u,103u})check(f.health(id)==1000);
    }
    for(bool npc:{false,true}){Fixture f(0,10);const auto target=f.scope(npc?100:2);f.action(target.actor,2);
        check(f.w.elemental(f.source,target,*effects::elemental(845,12),20,1000));
        check(f.health(target.actor)==0&&f.health(3)==909&&f.health(101)==909);}
    for(unsigned mode=0;mode<3;++mode){Fixture f;auto target=f.scope(2);auto plan=*effects::elemental(845,12);
        if(mode==0)++target.generation;if(mode==1)plan.area_if_target_active=false;if(mode==2)target=f.source;
        check(!f.w.elemental(f.source,target,plan,20,1000));check(f.health(2)==1000&&f.health(100)==1000);}
    for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
        Fixture f;const auto target=f.scope(npc?100:2);if(mode!=5)f.action(target.actor,mode==4?1:2,200,mode==4?2000:3000);
        casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,payments=0;std::vector<world::Packet> events;
        h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){events.insert(events.end(),p.begin(),p.end());};
        h.consume_energy_at=[&](unsigned cost,auto at){++payments;check(cost==10);return f.w.consume_energy(1,cost,f.source,at);};
        h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return id==target.actor&&range==1248;};
        if(mode!=3)h.elemental=[&](auto to,const auto& plan,auto at)->casting::EffectCompletion{
            ++calls;std::vector<world::Packet> visuals;const bool accepted=f.w.elemental(f.source,to,plan,20,at,&visuals);return {accepted,std::move(visuals)};};
        Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[845]=1;Slots bar{};bar[0]={845,0};attributes::Ranks ranks{};ranks[10]=12;
        check(runtime.start(1000,{845,0,target.actor},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
        if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1500,h);if(mode==5)f.action(target.actor,1,1500,3000);
        runtime.update(1999,h);check(!calls);runtime.update(2000,h);
        const bool hit=mode==0||mode==4||mode==5;check(calls==unsigned(hit)&&payments==unsigned(mode!=3));
        check(f.health(target.actor)==(hit?909:1000));check(f.health(3)==(hit&&mode!=4?909:1000));
        if(hit){const auto finish=casting::generic(58,1,0);const auto effect=*particles::targeted_body_2022(target.actor,1,845);
            const auto a=std::find(events.begin(),events.end(),finish),b=std::find(events.begin(),events.end(),effect);check(a!=events.end()&&b!=events.end()&&a<b);
            check(!casting::skill_success_body_effect(1,845));runtime.update(2000,h);check(calls==1&&payments==1);
            check(runtime.start(2750,{845,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);}
    }
    std::cout<<"Liquid Flame action classification,conditional area,atomic armor admission and cast controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
