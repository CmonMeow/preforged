#include "gw_npc_spell_replacement.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace replacement=combat::ai::replacement;namespace control=combat::npcs::control;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("NPC spell replacement line "+std::to_string(at.line()));
}
combat::npcs::Definition definition(std::optional<unsigned> energy){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
    d.conditions_allowed=true;d.knockdown_allowed=true;d.knockdown_evidence="synthetic fixture";
    if(energy){d.spawn_energy=combat::npcs::Energy{*energy,50};d.energy_pips=0;}return d;
}
struct Fixture {
    combat::npcs::LiveActor actor;combat::npcs::Lifetime life;control::Token attack;combat::ai::SpellTarget target;
    Fixture(std::optional<unsigned> energy=20):actor(definition(energy),{1,1,1,146,100}),life(actor.lifetime()){
        check(actor.configure_spellbook(life,{{{186,0},1248,"synthetic priority"},{{194,0},1248,"synthetic fallback"}}));
        target={{1,2,1},100,true,true,true,true};attack=*actor.begin_action(life,control::Action::attack,1000,2000);
    }
};
int main()try{
    {Fixture f;const auto before=f.actor.energy(f.life);const auto prepared=replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200);
        check(prepared&&prepared->spell.skill==skills::Slot{186,0}&&prepared->before.current==20&&prepared->after.current==10);
        check(f.actor.active_action(f.life,1200)==f.attack&&f.actor.energy(f.life)==before);
        check(replacement::current(*prepared,f.actor,f.life,f.target.scope,1200));
        f.actor=prepared->actor;check(f.actor.active_action(f.life,1200)==prepared->spell&&f.actor.energy(f.life)->current==10);
        check(!replacement::current(*prepared,f.actor,f.life,f.target.scope,1200));
        check(!f.actor.complete_action(f.life,f.attack,2000));
        check(!replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1300));
        check(f.actor.complete_action(f.life,prepared->spell,prepared->spell.completes));
        check(!f.actor.begin_action(f.life,control::Action::attack,prepared->spell.completes,prepared->spell.completes+500));}
    for(auto energy:{std::optional<unsigned>{},std::optional<unsigned>{0},std::optional<unsigned>{4}}){Fixture f(energy);
        const auto before=f.actor.energy(f.life);check(!replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200));
        check(f.actor.active_action(f.life,1200)==f.attack&&f.actor.energy(f.life)==before);}
    {Fixture f(5);const auto p=replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200);check(p&&p->spell.skill==skills::Slot{194,0}&&p->after.current==0);}
    {Fixture f;const combat::ai::Spellbook::Availability unavailable=[](skills::Slot,std::uint64_t)->std::optional<std::uint64_t>{return {};};
        check(!replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200,unavailable));check(f.actor.active_action(f.life,1200)==f.attack);}
    {Fixture f;const combat::ai::Spellbook::Availability alternate=[](skills::Slot s,std::uint64_t)->std::optional<std::uint64_t>{return s.skill==186?2000:0;};
        check(replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200,alternate)->spell.skill==skills::Slot{194,0});}
    for(unsigned fault=0;fault<9;++fault){Fixture f;auto token=f.attack;auto target=f.target;
        if(fault==0)token.sequence++;if(fault==1)token.epoch++;if(fault==2)token.actor.generation++;
        if(fault==3)target.scope.instance++;if(fault==4)target.alive=false;if(fault==5)target.line_of_sight=false;
        if(fault==6)target.distance=1300;if(fault==7)target.hostile=false;
        if(fault==8)target.scope.generation++;
        check(!replacement::prepare(f.actor,f.life,token,f.target.scope,target,1200)&&f.actor.active_action(f.life,1200)==f.attack);}
    {Fixture f;check(!replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,2000));
        check(f.actor.complete_action(f.life,f.attack,2000));}
    {Fixture f;check(f.actor.knock_down(f.life,1100,2000)==control::Knockdown::accepted);
        check(!replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200));}
    {Fixture f;check(f.actor.advance_energy(f.life,3000));
        check(!replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200));
        check(f.actor.active_action(f.life,1200)==f.attack);}
    {Fixture f;check(f.actor.apply_condition(f.life,combat::conditions::Kind::blind,2,1000,1000));
        check(f.actor.advance_conditions(f.life,3000).has_value());check(f.actor.health_clock(f.life)==3000);
        check(!replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200));}
    for(unsigned skill:{16u,17u}){Fixture f;check(f.actor.apply_interrupt_stance(f.life,skill,12,1,1100,10000));
        const auto p=replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200);
        check(p&&p->actor.newest_stance(f.life,1200)->skill==skill&&p->actor.newest_stance(f.life,1200)->instance==1);
        check(f.actor.newest_stance(f.life,1200)->skill==skill&&f.actor.energy(f.life)->current==20);}
    {Fixture f;const auto p=*replacement::prepare(f.actor,f.life,f.attack,f.target.scope,f.target,1200);
        check(!replacement::current(p,f.actor,f.life,{1,2,2},1200));check(!replacement::current(p,f.actor,f.life,f.target.scope,1201));}
    std::cout<<"NPC transactional ordinary-to-spell replacement passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
