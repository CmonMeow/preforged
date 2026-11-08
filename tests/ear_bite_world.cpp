#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Ear Bite World line "+std::to_string(at.line()));
}
struct Fixture{
    world::Instance w;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope caster,target;
    explicit Fixture(bool npc,bool conditions=true,unsigned health=1000){
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{double(health),health};
            d.damageable=true;d.conditions_allowed=conditions;
            d.fleshy=true;d.fleshy_evidence="synthetic living target";d.physical_armor=60;d.physical_armor_evidence="test";
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        if(!npc){
            w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{health,health,100,100},100);
        }
        caster=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
        if(!npc)check(w.physical_armor(target,60));
        box->drain();
    }
    double health()const{return target.actor==100?w.npc_combat_snapshot(target)->health->current:
        double(w.resources(2)->health);}
};
int main()try{
    using namespace skills;using namespace effects;
    for(bool npc:{false,true}){
        Fixture f(npc);std::function<void()> publish;
        check(f.w.ear_bite(f.caster,f.target,*ear_bite(10),20,1000,&publish));
        check(f.health()==930);
        if(!f.w.has_condition(f.target,combat::conditions::Kind::bleeding,1000))
            throw std::runtime_error(npc?"NPC missing Bleeding":"player missing Bleeding");
        check(f.box->drain().empty());
        check(bool(publish));publish();check(!f.box->drain().empty());publish();check(f.box->drain().empty());
        check(f.w.has_condition(f.target,combat::conditions::Kind::bleeding,25999));
        check(!f.w.has_condition(f.target,combat::conditions::Kind::bleeding,26000));
    }
    {
        Fixture immune(true,false);
        check(immune.w.ear_bite(immune.caster,immune.target,*ear_bite(10),20,1000));
        check(immune.health()==930);
        check(!immune.w.has_condition(immune.target,combat::conditions::Kind::bleeding,1000));
    }
    for(bool npc:{false,true}){
        Fixture lethal(npc,true,40);
        check(lethal.w.ear_bite(lethal.caster,lethal.target,*ear_bite(10),20,1000));
        check(lethal.health()==0);
        check(!lethal.w.has_condition(lethal.target,combat::conditions::Kind::bleeding,1000));
    }
    {
        Fixture f(false);auto forged=*ear_bite(10);++forged.damage;
        check(!f.w.ear_bite(f.caster,f.target,forged,20,1000));
        f.w.leave(2);check(!f.w.ear_bite(f.caster,f.target,*ear_bite(10),20,1000));
    }
    {
        Fixture f(false);casting::Runtime runtime(1);casting::Hooks h;
        unsigned payments=0,commits=0;
        h.resources=[&](unsigned id){return f.w.resources(id);};
        h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
        h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.caster,at);};
        h.consume_adrenaline=[&](auto slot,auto){check(slot.skill==2213);++payments;return true;};
        h.reconcile_adrenaline=[](auto){};
        h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};
        h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==144.f;};
        h.deldrimor_rank=[]()->std::optional<unsigned>{return 10;};
        h.ear_bite=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
            ++commits;std::function<void()> publication;
            const bool accepted=f.w.ear_bite(f.caster,target,plan,20,at,&publication);
            return {accepted,{},std::nullopt,std::move(publication)};
        };
        Slots bar{};bar[0]={2213,0};Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[2213]=1;
        attributes::Ranks ranks{};
        const auto admission=runtime.start(1000,{2213,0,2},bar,policy,ranks,true,h);
        if(admission!=casting::Reason::accepted)throw std::runtime_error("Ear Bite admission "+std::to_string(static_cast<unsigned>(admission)));
        check(payments==1&&commits==0&&f.health()==1000);
        runtime.update(1749,h);check(commits==0);
        runtime.update(1750,h);check(commits==1&&f.health()==930&&
            f.w.has_condition(f.target,combat::conditions::Kind::bleeding,1750));
        const auto packets=f.box->drain();
        const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
        const auto body=casting::skill_success_body_effect(1,2213);
        check(body.has_value());
        const auto visual=std::find(packets.begin(),packets.end(),*body);
        check(finish!=packets.end()&&visual!=packets.end()&&finish<visual&&visual+1!=packets.end());
    }
    std::cout<<"Ear Bite World passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
