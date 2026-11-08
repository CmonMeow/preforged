#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
using namespace fw::gw::skills;

static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Mend Body and Soul cast line "+std::to_string(at.line()));
}

static world::Instance::Npc make_npc(unsigned id,bool spirit){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=40;
    n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition body;
    body.binding.map=146;body.binding.actor=body.binding.definition=id;
    body.binding.level=20;body.spawn_health={20,100};body.damageable=true;
    body.relation=combat::npcs::Relation::allied_to_players;
    body.creature_type=spirit?"spirit":"human";body.creature_type_evidence="fixture";
    if(!spirit){body.healing_allowed=true;body.healing_evidence="fixture";}
    n.combat_definition=body;return n;
}

int main()try{
    using combat::conditions::Kind;
    for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<8;++mode){
        world::Instance w;
        if(mode==2)w.configure_npcs({make_npc(100,false),make_npc(101,true)});
        else if(mode==5){auto unknown=make_npc(101,true);
            unknown.combat_definition->creature_type.reset();
            w.configure_npcs({unknown});}
        else w.configure_npcs({make_npc(101,true)});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Ritualist",0,0,box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{20,100,50,50},100);
        w.set_vitals(2,{20,100,50,50},100);
        w.set_vitals(3,{20,100,50,50},100);
        const auto source=*w.effect_scope(1);
        if(version){const auto renewal=*effects::ether_renewal(version,15);
            check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));}
        const unsigned selected=mode==1?1:mode==2?100:2;
        if(mode==0||mode==1)
            check(w.apply_condition(source,*w.effect_scope(selected),Kind::blind,500,10000));
        box->drain();
        Slots bar{};bar[0]={1234,0};
        Policy policy;policy.primary=8;policy.campaigns=255;policy.learned[1234]=1;
        attributes::Ranks ranks{};ranks[33]=0;
        casting::Hooks h;
        h.events=[&](const auto& packets){box->enqueue(packets);};
        h.resources=[&](unsigned actor){return w.resources(actor);};
        h.consume_energy_at=[&](unsigned amount,auto now){
            check(amount==5);return w.consume_energy(1,amount,source,now);
        };
        h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
        h.mend_body_soul_target_valid=[&](auto target,auto now){
            return w.valid_mend_body_soul_target(source,target,now);
        };
        unsigned stages=0;
        h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(source,skill,event,now);};
        h.mend_body_soul_completed=[&](auto target,const auto& plan,auto event,auto now){
            ++stages;std::function<void()> publish;
            const bool accepted=w.resources(target.actor)?
                w.stage_mend_body_soul_player(source,target,plan,now,&publish,event):
                w.stage_mend_body_soul_npc(source,target,plan,now,&publish,event);
            if(accepted)check(!(w.resources(target.actor)?
                w.stage_mend_body_soul_player(source,target,plan,now,nullptr,event):
                w.stage_mend_body_soul_npc(source,target,plan,now,nullptr,event)));
            return casting::EffectCompletion{accepted,{}, {},std::move(publish)};
        };
        casting::Runtime cast(1);
        check(casting::Runtime::effect_supported(1234));
        auto missing=h;missing.mend_body_soul_completed={};
        casting::Runtime gated(1);
        check(gated.start(1000,{1234,0,selected},bar,policy,ranks,true,missing)==
            casting::Reason::unsupported);
        check(cast.start(1000,{1234,0,3},bar,policy,ranks,true,h)==casting::Reason::target);
        check(cast.start(1000,{1234,0,selected},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==45);
        if(mode==3)cast.cancel(h);
        if(mode==6)h.mend_body_soul_completed={};
        if(mode==7)h.before_spell_effect={};
        h.mend_body_soul=[](auto,auto,auto)->casting::EffectCompletion{throw std::runtime_error("Mend fallback");};
        if(mode==4){w.leave(2);w.join(2,"Replacement",40,0,
            std::make_shared<world::Outbox>(),0,{}, {},1200,1);
            w.set_vitals(2,{20,100,50,50},100);}
        cast.update(1749,h);check(stages==0);
        box->drain();cast.update(1750,h);
        check(stages==(mode==3||mode==4||mode>=6?0u:1u));
        const bool applied=mode!=3&&mode!=4&&mode!=5&&mode<6;
        const auto health=mode==2?w.npc_combat_snapshot(*w.effect_scope(100))->health->current:
            w.resources(selected)->health;
        check(w.resources(1)->health==20+(applied&&version?20:0)+(applied&&mode==1?20:0));
        check(w.resources(1)->energy==45+(applied&&version?effects::ether_renewal(version,15)->energy_per_enchantment:0));
        if(health!=20+(applied?20:0)+(applied&&version&&mode==1?20:0))
            throw std::runtime_error("Mend mode "+std::to_string(mode)+" health "+std::to_string(health));
        if(mode==0||mode==1)check(w.has_condition(*w.effect_scope(selected),Kind::blind,1750)==!applied);
        const auto packets=box->drain();
        const auto body=casting::skill_success_body_effect(1,1234);
        check(bool(body));
        const auto found=std::find(packets.begin(),packets.end(),*body);
        check((found!=packets.end())==applied);
        if(applied){
            const auto health=std::find(packets.begin(),packets.end(),
                particles::health_change_2022(selected,1,20,100));
            check(health!=packets.end()&&found<health);
        }
    }
    std::cout<<"PASS Mend Body and Soul cast admission, payment, target scope, cancellation, staged heal/cleanse and caster visual\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
