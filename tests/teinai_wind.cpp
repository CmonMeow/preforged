#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Teinai Wind line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source;
    std::optional<combat::npcs::control::Token> npc_action;
    Fixture(unsigned unknown=0,unsigned hp=1000,bool known_energy=true,unsigned npc_energy=20,unsigned npc_pips=0){
        std::vector<world::Instance::Npc> population;
        for(unsigned id=100;id<=104;++id){world::Instance::Npc n;n.id=n.definition.id=id;
            n.x=id==101?1166:id==102?1167:1000;n.plane=id==103?1:0;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
            d.relation=id==104?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{double(hp),1000};d.damageable=d.conditions_allowed=id!=104;
            if(id!=104&&known_energy){d.spawn_energy=combat::npcs::Energy{npc_energy,20};d.energy_evidence="test";d.energy_pips=npc_pips;d.energy_pips_evidence="test";}
            if(id!=unknown)d.cold_armor=60;n.combat_definition=d;population.push_back(n);}
        w.configure_npcs(population);
        for(unsigned id=1;id<=5;++id){w.join(id,"Actor",id==1?0:id==3?1166:id==4?1167:1000,0,
                id==1?box:std::make_shared<world::Outbox>(),0,{},0,100,id==1||id==5?1:2);
            w.set_vitals(id,{hp,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,id==unknown?std::nullopt:std::optional<int>{60}));}
        source=scope(1);box->drain();
    }
    effects::Scope scope(unsigned id){return *w.effect_scope(id);}
    double health(unsigned id){return id>=100?w.npc_combat_snapshot(scope(id))->health->current:double(w.resources(id)->health);}
    void burn(){check(w.elemental(source,scope(2),*effects::elemental(884,0),20,1000));box->drain();}
    void action(unsigned id,bool attack=false){if(id>=100){npc_action=w.begin_npc_action(scope(id),attack?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell,1000,3000,attack?std::nullopt:std::optional<Slot>{{281,0}});check(bool(npc_action));}
        else if(attack)check(w.attack_activity(scope(id),3000,1000));else check(w.cast_activity(scope(id),281,1000,3000));}
};
int main()try{
    using combat::conditions::Kind;
    check(!effects::elemental(1081,21));
    for(unsigned rank=0;rank<=20;++rank)for(bool burning:{false,true}){
        combat::Actor actor{2,1,combat::Health{1000,1000},{},0,true,{},60,60};actor.incoming_reduction=5;
        combat::conditions::State state;if(burning)check(state.apply(Kind::burning,1,1000,1000));
        const auto plan=*effects::elemental(1081,rank);const auto result=effects::elemental_target(plan,actor,state,20,1,1000);
        check(result.accepted&&result.hits.size()==1&&result.additions.empty()&&result.interrupt==burning);
        const double combined=10.+2.*rank+(burning?40.+8.*rank/3.:0.)-5.;
        check(std::abs(result.hits[0].damage-combined)<1e-9);
        const auto expired=effects::elemental_target(plan,actor,state,20,1,2000);
        check(expired.accepted&&!expired.interrupt&&expired.hits[0].damage==effects::interpolate(10,40,rank)-5);
    }
    for(bool npc:{false,true})for(bool burning:{false,true})for(bool attack:{false,true}){
        Fixture f;if(burning)f.burn();f.action(2,attack);f.action(100,attack);
        const auto epoch=*f.w.control_epoch(f.scope(2));std::vector<world::Packet> visuals;
        check(f.w.elemental(f.source,f.scope(npc?100:2),*effects::elemental(1081,12),20,1000,&visuals));
        for(unsigned id:{2u,3u,100u,101u})check(f.health(id)==(burning?894:966));
        for(unsigned id:{1u,4u,5u,102u,103u,104u})check(f.health(id)==1000);
        check((*f.w.control_epoch(f.scope(2))!=epoch)==burning);
        check(f.w.complete_npc_action(*f.npc_action,3000)==!burning);
        check(visuals.size()==4);for(unsigned id:{2u,3u,100u,101u})
            check(std::count(visuals.begin(),visuals.end(),*particles::targeted_body_2022(id,1,1081))==1);
    }
    for(bool npc:{false,true}){
        Fixture f;f.burn();const auto target=f.scope(npc?100:2);
        if(npc)check(f.w.npc_interrupt_stance(target,16,12,10000,1000));
        else check(f.w.status_mutation(target,16,99,12,1000,11000,true));
        f.action(target.actor);const auto epoch=npc?0:*f.w.control_epoch(target);
        check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));
        check(f.health(target.actor)==894);
        if(npc){const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),casting::terminal(59,target.actor))==0);}
        else check(*f.w.control_epoch(target)==epoch);
        check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));
        if(npc)check(!f.w.complete_npc_action(*f.npc_action,3000));else check(*f.w.control_epoch(target)!=epoch);
    }
    for(unsigned unknown:{3u,101u}){Fixture f(unknown);std::vector<world::Packet> visuals{{1,2}};
        check(!f.w.elemental(f.source,f.scope(2),*effects::elemental(1081,12),20,1000,&visuals));
        check(visuals.empty());for(unsigned id:{2u,3u,100u,101u})check(f.health(id)==1000);}
    for(bool npc:{false,true}){
        Fixture f;f.burn();const auto target=f.scope(npc?100:2);
        if(npc)check(f.w.npc_interrupt_stance(target,16,12,10000,1000));
        else check(f.w.status_mutation(target,16,99,12,1000,11000,true));
        // Burning but idle: no interruption attempt may consume the charge.
        check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));
        f.action(target.actor);const auto epoch=npc?0:*f.w.control_epoch(target);
        check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));
        if(npc)check(f.w.complete_npc_action(*f.npc_action,3000));else check(*f.w.control_epoch(target)==epoch);
    }
    for(bool npc:{false,true}){
        Fixture f;f.burn();const auto target=f.scope(npc?100:2);
        if(npc)check(f.w.npc_interrupt_stance(target,17,12,10000,1000));
        else {f.w.set_vitals(2,{1000,1000,20,100},100);check(f.w.status_mutation(target,17,99,12,1000,11000,true));}
        f.action(target.actor);const auto epoch=npc?0:*f.w.control_epoch(target);
        check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));
        check(f.health(target.actor)==894);
        check((npc?f.w.npc_energy_snapshot(target)->current:f.w.resources(2)->energy)==15);
        if(npc)check(f.w.complete_npc_action(*f.npc_action,3000));else check(*f.w.control_epoch(target)==epoch);
    }
    {Fixture f(0,1000,false);f.burn();check(f.w.npc_interrupt_stance(f.scope(100),17,12,10000,1000));f.action(100);
        check(!f.w.elemental(f.source,f.scope(2),*effects::elemental(1081,12),20,1000));
        for(unsigned id:{2u,3u,100u,101u})check(f.health(id)==1000);}
    for(bool npc:{false,true}){
        Fixture f(0,1000,true,npc?8:5,1);const auto target=f.scope(npc?100:2);
        if(npc){
            // Establish the NPC resource clock before the observation window.
            // Canonical Ether Feast drains3without target Health damage;
            // the full-Health caster cannot gain additional Health.
            check(f.w.energy_drain(f.source,target,*effects::energy_drain(40,0),20,100));
            check(f.w.npc_energy_snapshot(target)->current==5&&f.health(target.actor)==1000);
        }
        if(!npc){check(f.w.energy_capacity(2,5,0,100,{80,-1}));f.w.set_vitals(2,{1000,1000,5,100},100);}
        f.burn();check(f.w.apply_mind_wrack(f.source,target,12,true,1000));
        if(npc)check(f.w.npc_interrupt_stance(target,17,12,10000,1000));
        else check(f.w.status_mutation(target,17,99,12,1000,11000,true));
        f.action(target.actor);
        check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));
        const auto remaining=npc?f.w.npc_energy_snapshot(target)->current:f.w.resources(2)->energy;
        if(f.health(target.actor)!=894||remaining!=0)std::cerr<<"fractional reaction npc="<<npc<<" hp="<<f.health(target.actor)<<" energy="<<remaining<<'\n';
        check(f.health(target.actor)==894&&remaining==0);
        const auto statuses=f.w.status_snapshot(target,1000);check(bool(statuses));
        check(std::any_of(statuses->begin(),statuses->end(),[](const auto& e){return e.skill==2734;}));
        if(npc)check(*f.w.npc_precise_energy_snapshot(target)>0);
    }
    for(bool npc:{false,true})for(bool lethal:{false,true}){
        const unsigned hp=lethal?170:1000;Fixture f(0,hp,true,5);const auto target=f.scope(npc?100:2);
        if(!npc){
            // Keep precise Energy exactly5: ordinary regeneration can leave
            // a fractional positive balance even when the UI displays0.
            check(f.w.energy_capacity(2,5,0,100,{80,-2}));
            f.w.set_vitals(2,{hp,1000,5,100},100);
        }
        f.burn();
        check(f.w.apply_mind_wrack(f.source,target,12,true,1000));
        if(npc)check(f.w.npc_interrupt_stance(target,17,12,10000,1000));
        else check(f.w.status_mutation(target,17,99,12,1000,11000,true));
        f.action(target.actor);const auto epoch=npc?0:*f.w.control_epoch(target);
        check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));
        // One106Cold hit followed by one83armor-ignoring zero-Energy burst.
        if(f.health(target.actor)!=(lethal?0:811))std::cerr<<"zero reaction npc="<<npc<<" lethal="<<lethal<<" hp="<<f.health(target.actor)<<" energy="<<(npc?f.w.npc_energy_snapshot(target)->current:f.w.resources(2)->energy)<<'\n';
        check(f.health(target.actor)==(lethal?0:811));
        check((npc?f.w.npc_energy_snapshot(target)->current:f.w.resources(2)->energy)==0);
        const auto statuses=f.w.status_snapshot(target,1000);check(bool(statuses));
        check(std::none_of(statuses->begin(),statuses->end(),[](const auto& e){return e.skill==2734;}));
        if(lethal){
            if(!npc)check(*f.w.control_epoch(target)==epoch+1);
            const auto other=f.health(3);check(!f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));
            f.w.tick(1000);check(f.health(target.actor)==0&&f.health(3)==other);
        }else{
            // Exactly enough Energy did not end Resolve. Its final unpaid
            // prevention ends the stance without replaying Mind Wrack.
            check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));check(f.health(target.actor)==705);
            if(!npc)check(*f.w.control_epoch(target)==epoch);
            check(f.w.elemental(f.source,target,*effects::elemental(1081,12),20,1000));check(f.health(target.actor)==599);
            if(npc)check(!f.w.complete_npc_action(*f.npc_action,3000));else check(*f.w.control_epoch(target)==epoch+1);
        }
    }
    {Fixture f;auto plan=*effects::elemental(1081,12);++plan.bonus_if_burning;
        check(!f.w.elemental(f.source,f.scope(2),plan,20,1000));
        for(unsigned id:{2u,3u,100u,101u})check(f.health(id)==1000);}
    for(bool npc:{false,true})for(unsigned mode=0;mode<4;++mode){
        Fixture f;casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,payments=0;std::vector<world::Packet> events;
        h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){events.insert(events.end(),p.begin(),p.end());};
        h.consume_energy_at=[&](unsigned cost,auto at){++payments;check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
        h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return id==(npc?100:2)&&range==1248;};
        if(mode!=3)h.elemental=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
            ++calls;std::vector<world::Packet> visuals;const bool accepted=f.w.elemental(f.source,target,plan,20,at,&visuals);return {accepted,std::move(visuals)};};
        Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1081]=1;Slots bar{};bar[0]={1081,0};attributes::Ranks ranks{};ranks[8]=12;
        check(runtime.start(1000,{1081,0,npc?100u:2u},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
        if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1500,h);runtime.update(1999,h);check(!calls);runtime.update(2000,h);
        check(calls==unsigned(mode==0)&&payments==unsigned(mode!=3));check(f.health(2)==(mode==0?966:1000));
        if(mode==0){const auto finish=casting::generic(58,1,0),body=*casting::skill_success_body_effect(1,1081);
            auto a=std::find(events.begin(),events.end(),finish),b=std::find(events.begin(),events.end(),body);check(a!=events.end()&&b!=events.end()&&a<b);
            for(unsigned id:{2u,3u,100u,101u}){const auto effect=*particles::targeted_body_2022(id,1,1081);const auto p=std::find(events.begin(),events.end(),effect);check(p!=events.end()&&b<p);}
            runtime.update(2000,h);check(calls==1&&payments==1);}
    }
    std::cout<<"Teinai Wind single-hit Burning bonus,area recipients,interrupt protection and cast controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
