#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Putrid Bile Reversal line "+std::to_string(at.line()));
}
struct Fixture{
    world::Instance w;bool npc;effects::Scope caster,dead,target;
    Fixture(bool is_npc,unsigned hp):npc(is_npc){
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};
        d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==3?hp:1000u,1000,100,100},100);}
        caster=*w.effect_scope(1);dead=*w.effect_scope(2);target=*w.effect_scope(npc?100:3);
    }
    double hp()const{return npc?w.npc_combat_snapshot(target)->health->current:w.resources(3)->health;}
    void reversal(unsigned rr){check(npc?w.stage_npc_reversal(target,rr,100):w.status_mutation(target,307,900,rr,100,8100,true));}
    void bile(effects::Scope scope,unsigned rank){check(w.degeneration_hex(caster,scope,*effects::degeneration_hex(2058,rank),100,20));}
    bool has_reversal(){auto s=*w.status_snapshot(target,101);return std::any_of(s.begin(),s.end(),[](const auto& e){return e.skill==307;});}
};
int main()try{
    unsigned cases=0;
    for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
        ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.bile(f.dead,rank);
        const auto damage=effects::putrid_bile(rank)->death_damage;
        const auto heal=rr<0?0u:std::min(damage,effects::reversal_fortune(rr)->maximum_healing);
        const auto wanted=std::max(0.,std::min(1000.,double(hp+heal))-double(damage-heal));
        check(f.w.kill(f.dead,101));check(std::abs(f.hp()-wanted)<1.01&&!f.has_reversal());
        if(npc)check(f.w.take_npc_deaths(f.caster).size()==unsigned(wanted==0));
    }
    // Nested explosion commits must retain both hits, never restore an old
    // prepared Health snapshot. Conversion belongs to the first nested hit.
    for(bool npc:{false,true})for(int rr:{-1,0,12,20}){
        ++cases;Fixture f(npc,500);if(rr>=0)f.reversal(rr);
        f.bile(f.dead,0);f.bile(*f.w.effect_scope(3),0);
        if(npc){f.w.set_vitals(3,{10,1000,100,100},100);}
        else{
            // NPC100 dies first in the NPC pass only after player3 is hit.
            // Use player4 as the low-Health chain source before player3.
            f.w.join(4,"Chain",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            f.w.set_vitals(4,{10,1000,100,100},100);
            f.bile(*f.w.effect_scope(4),0);
        }
        check(f.w.kill(f.dead,101));
        const auto heal=rr<0?0u:std::min(25u,effects::reversal_fortune(rr)->maximum_healing);
        check(std::abs(f.hp()-(500.+heal-(25-heal)-25))<1.01&&!f.has_reversal());
    }
    // Healing debt can kill before the remaining-damage phase; no double death.
    for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
        ++cases;Fixture f(npc,10);
        if(npc){check(f.w.apply_condition(f.caster,f.caster,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.caster,f.target,*effects::condition_transfer(132,12),100));}
        else check(f.w.apply_condition(f.caster,f.target,combat::conditions::Kind::deep_wound,100,10000));
        f.reversal(rr);f.bile(f.dead,0);check(f.w.kill(f.dead,101));check(f.hp()==0&&!f.has_reversal());
        if(npc)check(f.w.take_npc_deaths(f.caster).size()==1);
    }
    check(cases==86);std::cout<<"PASS "<<cases<<" Putrid Bile conversion/chain/healing-death cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
