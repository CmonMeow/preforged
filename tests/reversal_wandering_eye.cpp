#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Wandering Reversal line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;bool npc_owner;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,owner;std::vector<effects::Scope> targets;
    Fixture(bool npc,unsigned hp=500,unsigned wound_actor=0):npc_owner(npc){
        const unsigned primary=npc?100:2;std::vector<world::Instance::Npc> population;
        for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?60:100;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={double(id==wound_actor?10:id==primary?hp:500),1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";
            d.spawn_energy=combat::npcs::Energy{20,100};d.energy_pips=0;n.combat_definition=d;population.push_back(n);
        }w.configure_npcs(population);
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:100,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{id==wound_actor?10u:id==primary?hp:id==1?1000u:500u,1000,20,100},100);
        }
        source=*w.effect_scope(1);owner=*w.effect_scope(primary);
        for(unsigned id:{2u,3u,100u,101u})targets.push_back(*w.effect_scope(id));
    }
    double hp(effects::Scope target)const{return target.actor>=100?w.npc_combat_snapshot(target)->health->current:w.resources(target.actor)->health;}
    void reversal(unsigned rank){for(auto target:targets)check(target.actor>=100?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
    void protection(unsigned skill){if(!skill)return;check(npc_owner?w.npc_interrupt_stance(owner,skill,12,10100,100):w.status_mutation(owner,skill,950,12,100,10100,true));}
    void source_state(unsigned state){if(state==1)check(w.kill(source,100));if(state>=2)w.leave(1);if(state==3){w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(1,{1000,1000,20,100},100);}}
    void trigger(unsigned protection,bool alive){
        if(npc_owner){const auto token=w.begin_npc_action(owner,combat::npcs::control::Action::attack,200,600);check(token.has_value());check(w.complete_npc_action(*token,600)==(protection&&alive));}
        else check(w.attack_activity(owner,1000,200)==(protection&&alive));
    }
};
double expected(unsigned hp,unsigned damage,int rank){const double heal=rank<0?0:std::min(damage,effects::reversal_fortune(rank)->maximum_healing);return std::max(0.,std::min(1000.,hp+heal)-(damage-heal));}
int main()try {
    unsigned cases=0;
    for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})
    for(unsigned state:{0u,1u,2u,3u})for(unsigned protection:{0u,16u,17u}){
        ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.protection(protection);check(f.w.apply_wandering_eye(f.source,f.owner,rank,100));f.source_state(state);
        const auto amount=effects::wandering_eye(rank)->damage;const double primary=expected(hp,amount,rr),side=expected(500,amount,rr);
        f.trigger(protection,primary>0);
        for(auto target:f.targets)check(std::abs(f.hp(target)-(target==f.owner?primary:side))<1.01);
        const auto entries=*f.w.status_snapshot(f.owner,600);check(std::none_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==2056;}));
        if(state==3)check(f.w.resources(1)->health==1000-amount); // New foe, never credited as old caster.
        if(state<2)check(f.w.take_npc_deaths(f.source).size()==unsigned(npc&&primary==0));
        if(primary>0){
            if(npc){const auto next=f.w.begin_npc_action(f.owner,combat::npcs::control::Action::attack,700,1000);check(next.has_value());}
            else check(f.w.attack_activity(f.owner,1000,700));
            for(auto target:f.targets)check(std::abs(f.hp(target)-(target==f.owner?primary:side))<1.01);
        }
    }
    check(cases==864);
    for(bool npc:{false,true})for(unsigned selection:{0u,3u,101u})for(unsigned state:{0u,1u,2u,3u})for(unsigned protection:{0u,16u}){
        ++cases;const unsigned actor=selection?selection:npc?100:2;Fixture f(npc,500,actor);f.reversal(12);f.protection(protection);
        const auto wounded=*f.w.effect_scope(actor);
        if(actor>=100){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,wounded,*effects::condition_transfer(132,12),100));}
        else check(f.w.apply_condition(f.source,wounded,combat::conditions::Kind::deep_wound,100,10000));
        check(f.w.apply_wandering_eye(f.source,f.owner,12,100));f.source_state(state);f.trigger(protection,wounded!=f.owner);
        for(auto target:f.targets)check(f.hp(target)==(target==wounded?0:540));
        if(state<2)check(f.w.take_npc_deaths(f.source).size()==unsigned(actor>=100));
    }
    // Resolve's real Energy cost kills the primary through Mind Wrack. Its
    // Putrid Bile explosion damages the area BEFORE Wandering Eye's hit. An
    // old prepared NPC copy would restore those lost25 HP; current commits
    // must retain both effects instead of resurrecting the proposal snapshot.
    {++cases;Fixture f(false,40);f.w.set_vitals(2,{40,1000,5,100},100);
        check(f.w.energy_capacity(2,5,0,100,{80,-2}));f.protection(17);
        check(f.w.apply_mind_wrack(f.source,f.owner,12,false,100));check(f.w.degeneration_hex(f.source,f.owner,*effects::degeneration_hex(2058,0),100));
        check(f.w.apply_wandering_eye(f.source,f.owner,12,100));check(!f.w.attack_activity(f.owner,1000,200));
        check(f.hp(f.owner)==0);
        for(auto target:f.targets)if(target!=f.owner)check(f.hp(target)==381);
    }
    check(cases==913);
    std::cout<<"PASS "<<cases<<" Wandering Eye Reversal cases; mixed recipients/player-NPC owners, interrupt protection, caster replacement, healing death, one-shot and intervening Bile damage preserved;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
