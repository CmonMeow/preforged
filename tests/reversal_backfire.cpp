#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Backfire Reversal line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope caster,target;
    bool npc;
    Fixture(bool is_npc,unsigned health):npc(is_npc){
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.spawn_health={double(health),1000};d.damageable=d.conditions_allowed=true;
        d.relation=combat::npcs::Relation::hostile_to_players;d.healing_allowed=true;d.healing_evidence="synthetic";
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Original caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        w.join(2,"Recipient",100,0,box,0,{}, {},100,2);w.set_vitals(2,{health,1000,100,100},100);
        caster=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
    }
    double hp()const{return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
    void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
    void source_state(unsigned state){
        if(state==1)check(w.kill(caster,100));
        if(state>=2)w.leave(1);
        if(state==3){w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);}
    }
};
unsigned word(const world::Packet& p,unsigned offset){unsigned result=0;for(unsigned i=0;i<4;++i)result|=unsigned(p[offset+i])<<(i*8);return result;}
bool negative_damage(const world::Packet& p){return p.size()==18&&p[0]==162&&word(p,2)==55&&std::bit_cast<float>(word(p,14))<0;}
int main()try {
    unsigned cases=0;
    for(bool npc:{false,true})for(int reversal_rank:{-1,0,12,20})for(unsigned backfire_rank:{0u,12u,20u})
    for(unsigned health:{10u,500u,1000u})for(unsigned state:{0u,1u,2u,3u}){
        ++cases;Fixture f(npc,health);if(reversal_rank>=0)f.reversal(reversal_rank);
        const auto plan=*effects::backfire(28,backfire_rank);check(f.w.backfire(f.caster,f.target,plan,20,100));
        f.source_state(state);f.box->drain();
        const double healing=reversal_rank<0?0:std::min(double(plan.damage),double(effects::reversal_fortune(reversal_rank)->maximum_healing));
        const double expected=std::max(0.,std::min(1000.,health+healing)-(plan.damage-healing));
        check(f.w.before_spell_effect(f.target,281,1,200)==(expected>0));
        if(std::abs(f.hp()-expected)>=1.01)throw std::runtime_error("Backfire health: npc="+std::to_string(npc)+" rr="+std::to_string(reversal_rank)+" rank="+std::to_string(backfire_rank)+" state="+std::to_string(state)+" actual="+std::to_string(f.hp())+" expected="+std::to_string(expected));
        const auto packets=f.box->drain();
        const auto losses=std::count_if(packets.begin(),packets.end(),negative_damage);
        check(losses==unsigned(state<2&&plan.damage>healing));
        for(const auto& p:packets)if(negative_damage(p))check(word(p,6)==f.target.actor&&word(p,10)==1);
        // Gone/replaced original caster: Health still changes, but no damage
        // notification may pretend the reused ID is its original cause.
        if(state>=2)check(std::any_of(packets.begin(),packets.end(),[&](const auto& p){return p.size()==14&&p[0]==161&&word(p,2)==34&&word(p,6)==f.target.actor;}));
        if(state==3)check(f.w.resources(1)->health==1000);
        if(npc){const auto deaths=f.w.take_npc_deaths(state<2?f.caster:*f.w.effect_scope(2));check(deaths.size()==unsigned(state<2&&expected==0));}
        if(expected>0){const auto entries=*f.w.status_snapshot(f.target,200);check(std::none_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;}));}
        check(!f.w.before_spell_effect(f.target,281,1,201)&&f.box->drain().empty());
    }
    check(cases==288);
    // Conversion healing can itself settle lethal Deep Wound debt. Grenth's
    // Balance may continue only with the exact activation's one-use proof.
    for(bool npc:{false,true})for(unsigned state:{0u,2u,3u}){
        ++cases;Fixture f(npc,10);
        if(npc){check(f.w.apply_condition(f.caster,f.caster,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.caster,f.target,*effects::condition_transfer(132,12),100));}
        else check(f.w.apply_condition(f.caster,f.target,combat::conditions::Kind::deep_wound,100,10000));
        f.reversal(12);check(f.w.backfire(f.caster,f.target,*effects::backfire(28,12),20,100));f.source_state(state);
        check(!f.w.before_spell_effect(f.target,86,1,200)&&f.hp()==0);
        if(!npc){check(f.w.take_grenths_reaction_death(f.target,1,200).has_value());check(!f.w.take_grenths_reaction_death(f.target,1,200));}
        if(npc&&state==0)check(f.w.take_npc_deaths(f.caster).size()==1);
    }
    std::cout<<"PASS "<<cases<<" Backfire Reversal cases; present/dead/departed/replaced caster, optional damage attribution, actual survival, healing death and Grenth proof;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
