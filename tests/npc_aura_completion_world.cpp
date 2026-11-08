#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC Aura completion World line "+std::to_string(at.line()));
}
int main()try{
    for(bool elemental:{false,true})for(unsigned rank:{0u,8u})for(bool strip:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{150,200};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;d.healing_allowed=true;d.healing_evidence="synthetic shared Aura completion fixture";
        n.combat_definition=d;w.configure_npcs({n});w.join(1,"Target",1000,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,0,100},100);const auto source=*w.effect_scope(100),target=*w.effect_scope(1);
        check(w.elemental_armor(target,60,60));
        combat::ai::FlarePolicy policy;policy.source=source;policy.level=20;policy.fire_magic=12;policy.speed=100;
        policy.acquire=1500;policy.follow=2000;policy.leash=2500;policy.flight={100,32};policy.evidence="synthetic shared Aura/Flare completion fixture";
        const auto aura=*skills::effects::aura_restoration_plan(rank);
        if(elemental)check(w.configure_npc_elemental_ai(policy,{{{194,0},*skills::effects::elemental(194,12),policy.evidence}},2,1000,aura));
        else check(w.configure_npc_flare_ai(policy,1000,aura));
        const auto token=w.begin_npc_aura(source,aura,1000);check(token.has_value());check(w.complete_npc_aura(*token,1250));
        check(w.npc_combat_snapshot(source)->health->current==150);check(w.npc_energy_snapshot(source)->current==35);
        w.tick(2000);
        if(strip){check(w.enchantment_removal(target,source,*skills::effects::enchantment_removal(1347),20,2500));
            check(!w.npc_aura_snapshot(source,2500));check((w.npc_combat_snapshot(source)->effects&0x80)==0);}
        w.tick(3000);
        const auto gain=*skills::effects::aura_restoration(*skills::find_skill(194),rank);
        check(w.npc_combat_snapshot(source)->health->current==150+(strip?0:gain.healing));
        check(w.npc_energy_snapshot(source)->current==30+(strip?0:gain.energy));
        check(w.resources(1)->health==1000); //heal belongs to completion, not projectile impact
        w.tick(3100);check(w.npc_combat_snapshot(source)->health->current==150+(strip?0:gain.healing));
    }
    std::cout<<"Shared NPC Aura healing/Energy at real Flare completion passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
