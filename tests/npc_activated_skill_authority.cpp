// Detached until typed activated-skill authority is integrated.
#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace npc=combat::npcs;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC activated authority line "+std::to_string(at.line()));
}
int main()try{
    npc::Definition definition;definition.binding={145,100,70,116378,0,0,4,2};
    definition.relation=npc::Relation::hostile_to_players;definition.spawn_health=combat::Health{120,120};
    definition.damageable=true;definition.spawn_energy=npc::Energy{40,40};definition.energy_pips=0;
    definition.knockdown_allowed=true;definition.knockdown_evidence="synthetic authority fixture";
    const npc::Lifetime life{1,1,2,145,100};
    const std::vector<combat::ai::ReviewedSpell> book{{{155,0},144,"synthetic rank/resource policy",
        combat::ai::SpellTargetClass::reviewed,combat::ai::activated::Kind::skill}};
    const combat::ai::SpellTarget target{{1,1,3},100,true,true,true,true};
    {
        auto unknown=definition;unknown.spawn_energy.reset();unknown.energy_pips.reset();npc::LiveActor actor(unknown,life);
        check(actor.configure_activated_skillbook(life,book));
        check(!actor.begin_activated_skill(life,{155,0},target,1000));
    }
    {
        npc::LiveActor actor(definition,life);check(!actor.configure_spellbook(life,book));
        check(actor.configure_activated_skillbook(life,book));
        check(!actor.begin_spell(life,{155,0},target,1000));
        auto far=target;far.distance=144.01f;check(!actor.begin_activated_skill(life,{155,0},far,1000));
        auto wall=target;wall.line_of_sight=false;check(!actor.begin_activated_skill(life,{155,0},wall,1000));
        check(actor.energy(life)->current==40);
        const auto token=actor.begin_activated_skill(life,{155,0},target,1000);
        check(token&&token->kind==npc::control::Action::skill&&token->completes==1750);
        check(!actor.owns_spell_action(life,*token)&&actor.owns_activated_skill_action(life,*token));
        check(actor.energy(life)->current==30);
        check(!actor.begin_activated_skill(life,{155,0},target,1100)&&actor.energy(life)->current==30);
        check(!actor.complete_ether_renewal_action(life,*token,1750));
        check(actor.complete_action(life,*token,1750));check(!actor.complete_action(life,*token,1750));
        check(!actor.ordinary_action_ready(life,2499)&&actor.ordinary_action_ready(life,2500));
        check(!actor.begin_activated_skill(life,{155,0},target,3749));
        check(actor.begin_activated_skill(life,{155,0},target,3750).has_value());
        check(actor.energy(life)->current==20);
    }
    {
        npc::LiveActor actor(definition,life);check(actor.configure_activated_skillbook(life,book));
        const auto token=actor.begin_activated_skill(life,{155,0},target,1000);check(token.has_value());
        check(actor.knock_down(life,1100)==npc::control::Knockdown::accepted);
        check(!actor.complete_action(life,*token,4000)&&actor.energy(life)->current==30);
        check(actor.begin_activated_skill(life,{155,0},target,4000).has_value());
        auto stale=life;++stale.instance_generation;
        check(!actor.begin_activated_skill(stale,{155,0},target,5000));
    }
    {
        npc::LiveActor actor(definition,life);check(actor.configure_activated_skillbook(life,book));
        const auto token=actor.begin_activated_skill(life,{155,0},target,1000);check(token.has_value());
        check(!actor.fail_activated_skill_target_action(life,*token,1749));
        auto forged=*token;forged.kind=npc::control::Action::spell;
        check(!actor.fail_activated_skill_target_action(life,forged,1750));
        check(actor.advance_energy(life,2000));
        check(!actor.fail_activated_skill_target_action(life,*token,1750));
        check(actor.owns_activated_skill_action(life,*token));
        check(actor.fail_activated_skill_target_action(life,*token,2000));
        check(actor.energy(life)->current==30&&actor.ordinary_action_ready(life,2000));
        check(!actor.fail_activated_skill_target_action(life,*token,2000));
        check(actor.begin_activated_skill(life,{155,0},target,2000).has_value());
        check(actor.energy(life)->current==20);
    }
    {
        // Existing spell-only wrapper remains usable; this verifies admission
        // authority only, not an immediate spell damage adapter.
        npc::LiveActor actor(definition,life);
        check(actor.configure_spellbook(life,{{{194,0},1248,"synthetic spell-wrapper regression"}}));
        const auto spell=actor.begin_spell(life,{194,0},target,1000);
        check(spell&&spell->kind==npc::control::Action::spell&&actor.owns_spell_action(life,*spell));
        check(actor.complete_action(life,*spell,spell->completes));
    }
    std::cout<<"NPC non-spell paid authority, identity, touch geometry, recovery and cancellation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
