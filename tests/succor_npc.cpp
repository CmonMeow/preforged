#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Succor NPC line "+std::to_string(at.line()));
}
Definition profile(unsigned current=10,unsigned pips=0){
    Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
    d.relation=Relation::allied_to_players;d.spawn_health=Health{500,1000};
    d.damageable=d.conditions_allowed=true;d.spawn_energy=Energy{current,20};d.energy_pips=pips;return d;
}
int main()try{
    const Lifetime life{1,2,3,146,100};const skills::effects::Scope caster{1,1,1};
    {LiveActor actor(profile(),life);
        const auto app=actor.apply_succor(life,caster,77,1000);check(bool(app));
        check(app->lease.target==actor.hex_scope()&&app->lease.caster==caster&&app->removed.empty());
        check(skills::effects::succor_contribution(app->lease,caster,actor.hex_scope(),*actor.owned_effect_snapshot(life),1000).has_value());
        check(actor.advance_conditions(life,2500).has_value());
        check(actor.actor(life)->health->current==503&&actor.precise_energy_units(life)==3150000);
        // Removal settles the remaining half unit before removing the modifier.
        check(actor.remove_reviewed_enchantment(life,77,4000));
        check(actor.actor(life)->health->current==506&&actor.energy(life)->current==11);
        const auto ends=actor.take_enchantment_ends(life);check(ends.size()==1&&ends[0].entry.instance==77&&ends[0].at==4000);
        check(!actor.remove_reviewed_enchantment(life,77,4000));
        check(actor.advance_conditions(life,10000).has_value());
        check(actor.actor(life)->health->current==506&&actor.energy(life)->current==11);
    }
    {LiveActor actor(profile(),life);check(actor.apply_succor(life,caster,77,1000).has_value());
        const skills::effects::Scope replacement{1,2,8};
        const auto renewed=actor.apply_succor(life,replacement,78,2500);
        check(renewed&&renewed->removed.size()==1&&renewed->removed[0].instance==77);
        check(!actor.hex_effect(life,77)&&actor.hex_effect(life,78)->cause==2);
        check(!actor.remove_reviewed_enchantment(life,77,2500));
        check(actor.advance_conditions(life,4000).has_value());
        check(actor.actor(life)->health->current==506&&actor.energy(life)->current==11);
        check(actor.remove_reviewed_enchantment(life,78,5500));
        check(actor.precise_energy_units(life)==3450000&&actor.actor(life)->health->current==509);
        check(actor.advance_conditions(life,8500).has_value()&&actor.precise_energy_units(life)==3450000);
    }
    // Existing regeneration is capped at ten pips; capped Energy drops its
    // fraction and earns no hidden credit that can appear after spending.
    {LiveActor actor(profile(0,10),life);
        check(actor.configure_friendly_spellbook(life,{286,0,0,"synthetic cap-test Energy-spend authority"}));
        check(actor.apply_succor(life,caster,77,1000).has_value());
        check(actor.advance_conditions(life,4000).has_value()&&actor.energy(life)->current==10);
        check(actor.advance_conditions(life,7100).has_value()&&actor.energy(life)->current==20);
        check(actor.precise_energy_units(life)==6000000);
        check(actor.lose_energy(life,1));check(actor.advance_conditions(life,7250).has_value());
        check(actor.precise_energy_units(life)==5850000);
    }
    for(unsigned fault=0;fault<9;++fault){auto d=profile();
        if(fault==0){d.spawn_energy.reset();d.energy_pips.reset();}
        if(fault==1)d.energy_pips.reset();if(fault==2)d.relation=Relation::neutral;
        LiveActor actor(d,life);auto selected=life;auto source=caster;unsigned effect=77;
        if(fault==3)++selected.actor_generation;if(fault==4)++source.instance;
        if(fault==5)source=actor.hex_scope();if(fault==6)source.generation=0;
        if(fault==7)effect=0;if(fault==8)++selected.instance_generation;
        check(!actor.apply_succor(selected,source,effect,1000));
        check(!actor.hex_effect(life,77)&&actor.actor(life)->health->current==500);
        check(actor.energy(life)==d.spawn_energy);
    }
    {LiveActor actor(profile(),life);check(actor.apply_succor(life,caster,77,1000).has_value());
        check(!actor.apply_succor(life,caster,77,2500)); // Effect IDs cannot be reused.
        auto stale=life;++stale.actor_generation;
        check(!actor.remove_reviewed_enchantment(stale,77,2500));
        check(actor.advance_conditions(life,4000).has_value());
        check(!actor.apply_succor(life,caster,78,3999)&&!actor.remove_reviewed_enchantment(life,77,3999));
        check(actor.actor(life)->health->current==506&&actor.energy(life)->current==11);
        check(actor.advance_conditions(life,1000000).has_value());
        check(actor.hex_effect(life,77)&&!actor.hex_effect(life,77)->expires);
        check(actor.actor(life)->health->current==1000&&actor.energy(life)->current==20);
    }
    std::cout<<"Succor NPC scoped recipient regeneration, replacement and chronological removal passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
