#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Succor NPC completion line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};
        d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{20,30};d.energy_pips=0;
        d.energy_evidence=d.energy_pips_evidence="authored test";
        d.healing_allowed=true;d.healing_evidence="authored test";
        d.knockdown_allowed=true;d.knockdown_evidence="authored test";
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Maintainer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,50,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        check(w.complete_succor(source,target,0,100));
        const bool spell=mode!=1;
        const auto token=w.begin_npc_action(target,spell?combat::npcs::control::Action::spell:combat::npcs::control::Action::attack,
            100,200,skills::Slot{spell?281u:341u,0});
        check(token.has_value());
        check(w.settle_resources(source,200));const auto before=w.resources(1)->energy;
        if(mode==2){
            check(!w.complete_npc_action(*token,199));
            check(w.resources(1)->energy==before);
        }
        if(mode==3){
            check(w.knockdown_npc(target,1000,200)==combat::npcs::control::Knockdown::accepted);
            check(!w.complete_npc_action(*token,200));
            check(w.resources(1)->energy==before);continue;
        }
        check(w.complete_npc_action(*token,200));
        check(w.resources(1)->energy==before-(spell?1u:0u));
        check(!w.complete_npc_action(*token,200));
        check(w.resources(1)->energy==before-(spell?1u:0u));
        // The public trusted observer shares the same lease deduplication as
        // the actual NPC terminal producer. Replaying its sequence cannot pay twice.
        check(w.succor_spell_completed(target,spell?281u:341u,token->sequence,200));
        check(w.resources(1)->energy==before-(spell?1u:0u));
        auto stale=target;++stale.generation;
        check(!w.succor_spell_completed(stale,281,token->sequence+1,200));
        if(mode==0){
            const auto energy=w.resources(1)->energy;
            const auto untyped=w.begin_npc_action(target,combat::npcs::control::Action::spell,200,300);
            check(untyped&&w.complete_npc_action(*untyped,300));
            const auto signet=w.begin_npc_action(target,combat::npcs::control::Action::spell,300,400,skills::Slot{297,0});
            check(signet&&w.complete_npc_action(*signet,400));
            check(w.resources(1)->energy==energy);
        }
    }
    // Exercise an actual paid friendly-heal provider, not only the generic
    // host terminal authority. It must debit once at successful completion.
    for(bool interrupt:{false,true}){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
        n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};
        d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{20,30};d.energy_pips=0;d.healing_allowed=true;
        d.healing_evidence="authored test";d.knockdown_allowed=true;d.knockdown_evidence="authored test";
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Maintainer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{500,1000,50,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        check(w.configure_npc_friendly_healer(target,{286,0,0,"authored paid-heal regression"}));
        check(w.complete_succor(source,target,0,100));
        const auto token=w.begin_npc_friendly_heal(target,source,100);check(token.has_value());
        if(interrupt)check(w.knockdown_npc(target,1000,200)==combat::npcs::control::Knockdown::accepted);
        w.tick(token->completes);check(w.resources(1)->energy==(interrupt?50u:49u));
        w.tick(token->completes+1);check(w.resources(1)->energy==(interrupt?50u:49u));
    }
    // Healing precedes Succor's reactive Energy loss. Otherwise Mind Wrack
    // would kill this low-Health maintainer before its accepted heal lands.
    {world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{20,30};d.energy_pips=0;d.healing_allowed=true;d.healing_evidence="authored reaction fixture";
        n.combat_definition=d;w.configure_npcs({n});
        for(unsigned id:{1u,2u}){w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{id==1?60u:1000u,1000,id==1?1u:100u,100},100);}
        const auto source=*w.effect_scope(1),enemy=*w.effect_scope(2),target=*w.effect_scope(100);
        check(w.energy_capacity(1,5,0,100,{80,-2}));
        check(w.configure_npc_friendly_healer(target,{286,0,0,"authored heal-before-Succor reaction"}));
        check(w.complete_succor(source,target,0,100)&&w.apply_mind_wrack(enemy,source,12,false,100));
        const auto token=w.begin_npc_friendly_heal(target,source,100);check(token.has_value());
        w.tick(token->completes);check(w.resources(1)->health==12&&w.resources(1)->energy==0);
        check(w.maintained_enchantment_count(source,token->completes)==0);
        w.tick(token->completes+1);check(w.resources(1)->health==12);
    }
    std::cout<<"PASS NPC Succor generic/paid-provider completion,attack exclusion,premature/interrupt rejection,exact sequence deduplication and stale scope\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
