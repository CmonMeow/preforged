#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Restful Breeze NPC line "+std::to_string(at.line()));
}
combat::npcs::LiveActor make_npc(bool spell=false){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
    d.relation=spell?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
    d.spawn_health=combat::Health{100,2000};d.damageable=d.conditions_allowed=true;
    if(spell){d.spawn_energy=combat::npcs::Energy{20,50};d.energy_pips=0;}
    return {d,{1,1,1,146,100}};
}
int main()try{
    const skills::effects::Scope caster{1,1,1};
    for(unsigned rank:{0u,12u,15u,20u})for(unsigned mode=0;mode<3;++mode){
        auto npc=make_npc();const auto life=npc.lifetime();
        const auto plan=*skills::regeneration::reviewed(886,rank);
        auto bad=plan;++bad.pips;
        check(!npc.apply_restful_breeze(life,caster,bad,1,1000));
        check(npc.apply_restful_breeze(life,caster,plan,1,1000));
        check(!npc.apply_restful_breeze(life,caster,plan,1,1000));
        const auto until=1000+plan.duration;
        if(mode==0){for(std::uint64_t at=1100;at<=until;at+=100)check(bool(npc.advance_conditions(life,at)));}
        else if(mode==1)check(bool(npc.advance_conditions(life,until)));
        else{check(npc.remove_reviewed_enchantment(life,1,3000));check(bool(npc.advance_conditions(life,until)));}
        const double expected=100+20.*double(mode==2?2000:plan.duration)/1000.;
        check(std::abs(npc.actor(life)->health->current-expected)<1e-7);
        check(bool(npc.advance_conditions(life,until+1000)));
        check(std::abs(npc.actor(life)->health->current-expected)<1e-7);
    }
    auto stale=make_npc();const auto life=stale.lifetime();auto other=life;++other.actor_generation;
    check(!stale.apply_restful_breeze(other,caster,*skills::regeneration::reviewed(886,12),1,1000));
    for(bool skill_use:{false,true}){
        auto npc=make_npc();const auto lifetime=npc.lifetime();
        check(npc.apply_restful_breeze(lifetime,caster,*skills::regeneration::reviewed(886,12),1,1000));
        const auto token=npc.begin_action(lifetime,skill_use?combat::npcs::control::Action::spell:combat::npcs::control::Action::attack,
            2000,2500,skill_use?std::optional(skills::Slot{886,0}):std::nullopt);
        check(bool(token));
        const auto ended=npc.take_enchantment_ends(lifetime);
        check(ended.size()==1&&ended[0].entry.skill==886&&ended[0].at==2000);
        check(!npc.hex_effect(lifetime,1));
        check(bool(npc.advance_conditions(lifetime,3000))&&std::abs(npc.actor(lifetime)->health->current-120)<1e-7);
    }
    {auto npc=make_npc(true);const auto lifetime=npc.lifetime();
     check(npc.configure_spellbook(lifetime,{{{194,0},1248,"Restful accepted spell fixture"}}));
     check(npc.apply_restful_breeze(lifetime,caster,*skills::regeneration::reviewed(886,12),1,1000));
     const combat::ai::SpellTarget target{{1,2,1},100,true,true,true,true};
     check(bool(npc.begin_spell(lifetime,{194,0},target,2000)));
     const auto ended=npc.take_enchantment_ends(lifetime);
     check(ended.size()==1&&ended[0].entry.skill==886&&!npc.hex_effect(lifetime,1));
     check(bool(npc.advance_conditions(lifetime,3000))&&std::abs(npc.actor(lifetime)->health->current-120)<1e-7);
    }
    std::cout<<"PASS staged NPC Restful Breeze ranked regeneration, cadence, early removal, generation and accepted action ending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
