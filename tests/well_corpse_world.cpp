#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <stdexcept>
#include <iostream>
#include <source_location>
#include <algorithm>
using namespace fw::gw;
template<class T> void check(const T& value,std::source_location where=std::source_location::current()){
    if(!static_cast<bool>(value))throw std::runtime_error("Well World test failed at "+std::to_string(where.line()));
}
int main()try{
    world::Instance world;
    world::Instance::Npc first;
    first.id=first.definition.id=100;
    first.definition.visual_adjustment={0,0,0,100};
    first.x=80;
    combat::npcs::Definition definition;
    definition.binding.map=146;
    definition.binding.actor=definition.binding.definition=100;
    definition.relation=combat::npcs::Relation::hostile_to_players;
    definition.spawn_health=combat::Health{100,100};
    definition.damageable=true;
    definition.fleshy=true;definition.fleshy_evidence="test authored";
    first.combat_definition=definition;
    auto second=first;second.id=second.definition.id=101;
    second.combat_definition->binding.actor=second.combat_definition->binding.definition=101;
    second.x=160;
    auto third=first;third.id=third.definition.id=102;
    third.combat_definition->binding.actor=third.combat_definition->binding.definition=102;
    third.x=450;
    world.configure_npcs({first,second,third});
    auto receiver=std::make_shared<world::Outbox>();
    world.join(2,"Caster",0,0,receiver,0,{}, {},0,7);
    world.set_vitals(2,{50,100,0,100},100);
    const auto caster=*world.effect_scope(2),near=*world.effect_scope(100),far=*world.effect_scope(101);
    const auto plan=*skills::effects::well_cast_plan(92,12);
    unsigned publications=0;
    const auto commit=[&](const skills::effects::WellCorpseClaim& claim){
        ++publications;check(claim.corpse==near);return true;
    };
    check(!world.claim_well_corpse(caster,plan,1,200,252,commit));
    check(world.damage_npc(caster,near,{combat::Kind::armor_ignoring,1000,20,194},1248,201).killed);
    check(world.damage_npc(caster,far,{combat::Kind::armor_ignoring,1000,20,194},1248,202).killed);
    auto stale=caster;++stale.generation;
    check(!world.claim_well_corpse(stale,plan,1,203,252,commit));
    check(!world.claim_well_corpse(caster,plan,1,203,20,commit));
    check(!world.claim_well_corpse(caster,plan,0,203,252,commit));
    check(!world.claim_well_corpse(caster,plan,1,203,252,[&](const auto& claim){
        check(claim.corpse==near);++publications;return false;
    }));
    check(world.claim_well_corpse(caster,plan,2,204,252,commit));
    check(publications==2);
    check(world.well_field_effect_at(0,80,0,7,204)->health_pips>0);
    check(world.well_field_effect_at(0,80,0,8,204)->health_pips==0);
    check(world.well_field_effect_at(0,80,0,7,100000)->health_pips==0);
    check(!world.claim_well_corpse(caster,plan,3,205,100,commit));
    check(world.claim_well_corpse(caster,plan,4,205,252,[&](const auto& claim){
        check(claim.corpse==far);++publications;return true;
    }));
    check(world.well_field_effect_at(0,160,0,7,205)->health_pips>0);
    check(!world.claim_well_corpse(caster,plan,5,206,252,commit));
    const auto stationary=world.well_player_exposure(caster,206,300);check(stationary);
    check(stationary->size()==1&&stationary->front().effect.health_pips>0);
    check(!world.well_player_exposure(stale,206,300));
    world.move(2,500,0,300);
    const auto traveling=world.well_player_exposure(caster,300,2200);check(traveling);
    check(traveling->size()>=2&&traveling->front().effect.health_pips>0&&
          traveling->back().effect.health_pips==0);
    check(!world.well_player_exposure(caster,299,500));
    world::Instance control;
    control.join(2,"Control",0,0,std::make_shared<world::Outbox>(),0,{}, {},0,7);
    control.set_vitals(2,{50,100,0,100},100);
    control.move(2,500,0,300);
    world.tick(1200);control.tick(1200);
    check(world.resources(2)->health>control.resources(2)->health);
    const auto last=*world.effect_scope(102);
    check(world.damage_npc(caster,last,{combat::Kind::armor_ignoring,1000,20,194},1248,1201).killed);
    const auto power=*skills::effects::well_cast_plan(91,12);
    check(world.claim_well_corpse(caster,power,6,20000,252,[&](const auto& claim){
        check(claim.corpse==last);return true;
    }));
    // The older fields expired, but the player's delayed resource clock still
    // needs their history. A new cast must not retire that history early.
    check(world.well_player_exposure(caster,1200,20000));
    world.move(2,450,0,21000);control.move(2,450,0,21000);
    world.tick(24000);control.tick(24000);
    check(world.resources(2)->energy>control.resources(2)->energy);
    const auto packets=receiver->drain();
    check(std::find(packets.begin(),packets.end(),vitals::energy_regeneration_2022(2,4,100))!=packets.end());
    world.tick(51000);
    const auto expired=receiver->drain();
    check(std::find(expired.begin(),expired.end(),vitals::energy_regeneration_2022(2,2,100))!=expired.end());
    world::Instance overcast_world,overcast_control;
    auto only=first;only.x=80;
    overcast_world.configure_npcs({only});
    overcast_world.join(2,"Power",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,7);
    overcast_control.join(2,"Control",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,7);
    overcast_world.set_vitals(2,{100,100,20,100},100);
    overcast_control.set_vitals(2,{100,100,20,100},100);
    check(overcast_world.energy_capacity(2,6,0,100,{10,-4}));
    check(overcast_control.energy_capacity(2,6,0,100,{10,-4}));
    overcast_world.set_vitals(2,{100,100,20,30},100);
    overcast_control.set_vitals(2,{100,100,20,30},100);
    const auto owner=*overcast_world.effect_scope(2),comparison=*overcast_control.effect_scope(2);
    check(overcast_world.activate_overcast_energy(owner,231,5,1,100));
    check(overcast_control.activate_overcast_energy(comparison,231,5,1,100));
    const auto body=*overcast_world.effect_scope(100);
    check(overcast_world.damage_npc(owner,body,{combat::Kind::armor_ignoring,1000,20,194},1248,101).killed);
    check(overcast_world.claim_well_corpse(owner,power,1,102,252,[&](const auto& claim){
        check(claim.corpse==body);return true;
    }));
    overcast_world.tick(3102);overcast_control.tick(3102);
    check(overcast::precise_current(*overcast_world.overcast_energy_snapshot(owner))>
          overcast::precise_current(*overcast_control.overcast_energy_snapshot(comparison)));
    for(const bool allied:{false,true}){
        world::Instance npc_world;
        auto body=first;body.id=body.definition.id=100;body.x=80;
        auto subject=first;subject.id=subject.definition.id=101;subject.x=100;
        subject.combat_definition->binding.actor=subject.combat_definition->binding.definition=101;
        subject.combat_definition->relation=allied?combat::npcs::Relation::allied_to_players:
            combat::npcs::Relation::hostile_to_players;
        subject.combat_definition->spawn_health={allied?50.:100.,100};
        npc_world.configure_npcs({body,subject});
        npc_world.join(2,"Field caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,7);
        npc_world.set_vitals(2,{100,100,20,100},100);
        const auto source=*npc_world.effect_scope(2),corpse=*npc_world.effect_scope(100);
        check(npc_world.damage_npc(source,corpse,{combat::Kind::armor_ignoring,1000,20,194},1248,101).killed);
        const auto field=*skills::effects::well_cast_plan(allied?92:93,12);
        check(npc_world.claim_well_corpse(source,field,1,102,252,[](const auto&){return true;}));
        npc_world.tick(1102);
        const auto observed=npc_world.npc_combat_snapshot(*npc_world.effect_scope(101));
        check(observed&&observed->health);
        check(allied?observed->health->current>50:observed->health->current<100);
        if(!allied){
            npc_world.tick(16000);
            const auto fallen=npc_world.npc_combat_snapshot(*npc_world.effect_scope(101));
            check(fallen&&fallen->health&&fallen->health->current==0);
        }
    }
    world::Instance npc_power,npc_baseline;
    auto power_body=first;power_body.id=power_body.definition.id=100;power_body.x=80;
    auto power_subject=first;power_subject.id=power_subject.definition.id=101;power_subject.x=100;
    power_subject.combat_definition->binding.actor=power_subject.combat_definition->binding.definition=101;
    power_subject.combat_definition->relation=combat::npcs::Relation::allied_to_players;
    power_subject.combat_definition->spawn_energy=combat::npcs::Energy{0,100};
    power_subject.combat_definition->energy_pips=2;
    npc_power.configure_npcs({power_body,power_subject});
    npc_baseline.configure_npcs({power_subject});
    npc_power.join(2,"Power caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,7);
    npc_power.set_vitals(2,{100,100,20,100},100);
    const auto power_owner=*npc_power.effect_scope(2);
    check(npc_power.damage_npc(power_owner,*npc_power.effect_scope(100),
        {combat::Kind::armor_ignoring,1000,20,194},1248,101).killed);
    check(npc_power.claim_well_corpse(power_owner,*skills::effects::well_cast_plan(91,12),
        1,102,252,[](const auto&){return true;}));
    npc_baseline.tick(102);
    npc_power.tick(3102);npc_baseline.tick(3102);
    check(npc_power.npc_energy_snapshot(*npc_power.effect_scope(101))->current==4);
    check(npc_baseline.npc_energy_snapshot(*npc_baseline.effect_scope(101))->current==2);
    // The packet path must preserve signed pip display even though negative
    // ordinary Energy settlement is tracked as a separate runtime gap.
    world::Instance signed_world;
    auto signed_box=std::make_shared<world::Outbox>();
    signed_world.join(2,"Signed",0,0,signed_box,0,{}, {},100,7);
    signed_world.set_vitals(2,{100,100,20,100},100);
    check(signed_world.energy_capacity(2,6,0,100,{10,-4}));
    const auto signed_packets=signed_box->drain();
    check(std::find(signed_packets.begin(),signed_packets.end(),
        vitals::energy_regeneration_2022(2,-2,30))!=signed_packets.end());
    world::Instance profane_world;
    auto profane_body=first;profane_body.id=profane_body.definition.id=100;profane_body.x=80;
    profane_world.configure_npcs({profane_body});
    auto inside_box=std::make_shared<world::Outbox>();
    profane_world.join(2,"Profane caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,7);
    profane_world.join(3,"Enemy inside",100,0,inside_box,0,{}, {},100,8);
    profane_world.join(4,"Enemy outside",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,8);
    profane_world.set_vitals(2,{100,100,20,100},100);
    profane_world.set_vitals(3,{100,100,20,100},100);
    profane_world.set_vitals(4,{100,100,20,100},100);
    const auto profane_source=*profane_world.effect_scope(2);
    const auto inside=*profane_world.effect_scope(3),outside=*profane_world.effect_scope(4);
    const auto enchant=*skills::regeneration::reviewed(299,0);
    check(profane_world.protective_enchantment(profane_source,profane_source,enchant,0,100));
    check(profane_world.protective_enchantment(inside,inside,enchant,0,100));
    check(profane_world.protective_enchantment(outside,outside,enchant,0,100));
    check(profane_world.status_snapshot(inside,100)->size()==1);
    check(profane_world.status_snapshot(outside,100)->size()==1);
    check(profane_world.damage_npc(profane_source,*profane_world.effect_scope(100),
        {combat::Kind::armor_ignoring,1000,20,194},1248,101).killed);
    check(!profane_world.claim_well_corpse(profane_source,*skills::effects::well_cast_plan(94,12),
        1,102,252,[](const auto&){return false;}));
    check(profane_world.status_snapshot(inside,102)->size()==1);
    check(profane_world.claim_well_corpse(profane_source,*skills::effects::well_cast_plan(94,12),
        1,102,252,[](const auto&){return true;}));
    check(profane_world.status_snapshot(inside,102)->empty());
    check(profane_world.status_snapshot(outside,102)->size()==1);
    check(profane_world.status_snapshot(profane_source,102)->size()==1);
    check(profane_world.protective_enchantment(inside,inside,enchant,0,103));
    check(profane_world.status_snapshot(inside,103)->empty());
    check(profane_world.status_mutation(inside,214,11,12,104,5104,true));
    check(profane_world.status_snapshot(inside,104)->empty());
    inside_box->drain();
    const auto protection=*skills::effects::protective_spirit(12);
    check(profane_world.complete_protective_spirit(inside,inside,protection,0,105));
    check(profane_world.status_snapshot(inside,105)->empty());
    const auto blocked_packets=inside_box->drain();
    check(std::none_of(blocked_packets.begin(),blocked_packets.end(),[](const auto& packet){
        return packet.size()>1&&packet[0]==65&&packet[1]==0;
    }));
    check(std::find(blocked_packets.begin(),blocked_packets.end(),
        skills::casting::terminal(58,inside.actor))!=blocked_packets.end());
    std::vector<world::Packet> demonic_visuals;
    check(profane_world.demonic_flesh(inside,17,*skills::effects::demonic_flesh(12),106,&demonic_visuals));
    check(std::find(demonic_visuals.begin(),demonic_visuals.end(),
        combat::detail::integer(21,inside.actor,240))!=demonic_visuals.end());
    check(profane_world.conjure_enchantment(inside,*skills::effects::conjure(182,12),107));
    check(profane_world.armor_earth(inside,12,108));
    check(profane_world.mantra_recall(inside,12,109));
    check(profane_world.status_snapshot(inside,109)->empty());
    const auto blocked_self_packets=inside_box->drain();
    check(std::none_of(blocked_self_packets.begin(),blocked_self_packets.end(),[](const auto& packet){
        return packet.size()>1&&packet[0]==65&&packet[1]==0;
    }));
    profane_world.move(4,100,0,103);
    profane_world.tick(150);
    check(profane_world.status_snapshot(outside,150)->size()==1);
    profane_world.tick(700);
    check(profane_world.status_snapshot(outside,700)->empty());
    check(profane_world.protective_enchantment(outside,outside,enchant,0,701));
    check(profane_world.status_snapshot(outside,701)->empty());
    {
        skills::casting::Runtime cast(3);
        skills::casting::Hooks hooks;
        std::vector<world::Packet> emitted;
        hooks.resources=[&](auto id){return profane_world.resources(id);};
        hooks.consume_energy=[&](auto cost){return profane_world.consume_energy(3,cost);};
        hooks.restore_energy=[](auto){return true;};
        hooks.heal=[](auto,auto,float,bool){return true;};
        hooks.target_valid=[](auto id,float,bool){return id==3;};
        hooks.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto ends,bool active){
            return profane_world.status_mutation(inside,skill,effect,rank,at,ends,active);
        };
        hooks.effect_snapshot=[&](auto at){return profane_world.status_snapshot(inside,at);};
        hooks.events=[&](const auto& packets){emitted.insert(emitted.end(),packets.begin(),packets.end());};
        skills::Policy policy;policy.primary=6;policy.secondary=3;policy.campaigns=255;policy.learned[180]=1;
        skills::Slots bar{};bar[0]={180,0};attributes::Ranks ranks{};ranks[12]=8;
        const auto started=cast.start(702,{180,0,3},bar,policy,ranks,true,hooks);
        if(started!=skills::casting::Reason::accepted)
            throw std::runtime_error("Profane Aura cast start rejected: "+std::to_string(static_cast<int>(started)));
        cast.update(952,hooks);
        check(profane_world.status_snapshot(inside,952)->empty());
        check(std::none_of(emitted.begin(),emitted.end(),[](const auto& packet){return packet.size()>1&&packet[0]==65&&packet[1]==0;}));
    }
    profane_world.tick(3000);
    check(profane_world.status_snapshot(outside,3000)->empty());
    check(profane_world.status_snapshot(profane_source,3000)->size()==1);
    profane_world.tick(20000);
    check(profane_world.protective_enchantment(inside,inside,enchant,0,20001));
    check(profane_world.status_snapshot(inside,20001)->size()==1);
    check(profane_world.complete_protective_spirit(inside,inside,protection,0,20002));
    check(profane_world.status_snapshot(inside,20002)->size()==2);
    world::Instance npc_profane;
    auto npc_corpse=first;npc_corpse.id=npc_corpse.definition.id=100;npc_corpse.x=80;
    auto npc_inside=first;npc_inside.id=npc_inside.definition.id=101;npc_inside.x=100;
    npc_inside.combat_definition->binding.actor=npc_inside.combat_definition->binding.definition=101;
    npc_inside.combat_definition->spawn_health={150,200};
    npc_inside.combat_definition->spawn_energy=combat::npcs::Energy{40,40};
    npc_inside.combat_definition->energy_pips=0;
    npc_inside.combat_definition->healing_allowed=true;
    npc_inside.combat_definition->healing_evidence="synthetic Profane NPC Aura fixture";
    auto npc_outside=npc_inside;npc_outside.id=npc_outside.definition.id=102;npc_outside.x=500;
    npc_outside.combat_definition->binding.actor=npc_outside.combat_definition->binding.definition=102;
    auto npc_new=npc_inside;npc_new.id=npc_new.definition.id=103;npc_new.x=120;
    npc_new.combat_definition->binding.actor=npc_new.combat_definition->binding.definition=103;
    npc_profane.configure_npcs({npc_corpse,npc_inside,npc_outside,npc_new});
    auto npc_box=std::make_shared<world::Outbox>();
    npc_profane.join(2,"NPC Profane caster",0,0,npc_box,0,{}, {},100,7);
    npc_profane.set_vitals(2,{100,100,20,100},100);
    const auto npc_source=*npc_profane.effect_scope(2);
    const auto aura=*skills::effects::aura_restoration_plan(0);
    for(const unsigned id:{101u,102u,103u})
        check(npc_profane.configure_npc_spellbook(*npc_profane.effect_scope(id),
            {{{180,0},0,"synthetic Profane Aura",combat::ai::SpellTargetClass::self}}));
    const auto prior_inside=*npc_profane.effect_scope(101);
    const auto prior_outside=*npc_profane.effect_scope(102);
    const auto first_cast=npc_profane.begin_npc_aura(prior_inside,aura,1000);
    check(first_cast&&npc_profane.complete_npc_aura(*first_cast,1250));
    const auto second_cast=npc_profane.begin_npc_aura(prior_outside,aura,1251);
    check(second_cast&&npc_profane.complete_npc_aura(*second_cast,1501));
    check(npc_profane.npc_aura_snapshot(prior_inside,1501));
    check(npc_profane.npc_aura_snapshot(prior_outside,1501));
    const auto old_aura=*npc_profane.npc_aura_snapshot(prior_inside,1501);
    npc_box->drain();
    check(npc_profane.damage_npc(npc_source,*npc_profane.effect_scope(100),
        {combat::Kind::armor_ignoring,1000,20,194},1248,1502).killed);
    const auto npc_field=*skills::effects::well_cast_plan(94,12);
    check(!npc_profane.claim_well_corpse(npc_source,npc_field,1,1503,252,[](const auto&){return false;}));
    check(npc_profane.npc_aura_snapshot(prior_inside,1503));
    check(npc_profane.claim_well_corpse(npc_source,npc_field,1,1503,252,[](const auto&){return true;}));
    check(!npc_profane.npc_aura_snapshot(prior_inside,1503));
    check(npc_profane.npc_aura_snapshot(prior_outside,1503));
    world::Packet removed{67,0,101,0,0,0};
    for(unsigned shift=0;shift<32;shift+=8)removed.push_back(static_cast<std::uint8_t>(old_aura.instance>>shift));
    const auto removed_packets=npc_box->drain();
    check(std::find(removed_packets.begin(),removed_packets.end(),removed)!=removed_packets.end());
    const auto fresh=*npc_profane.effect_scope(103);
    const auto deflection=*skills::effects::shield_deflection(12);
    npc_box->drain();
    check(npc_profane.stage_npc_shield_deflection(fresh,deflection,1600));
    check(!(npc_profane.npc_combat_snapshot(fresh)->effects&0x80));
    const auto npc_blocked_packets=npc_box->drain();
    check(std::none_of(npc_blocked_packets.begin(),npc_blocked_packets.end(),[](const auto& packet){
        return packet.size()>1&&packet[0]==65&&packet[1]==0;
    }));
    const auto suppressed=npc_profane.begin_npc_aura(fresh,aura,1600);
    check(suppressed&&npc_profane.complete_npc_aura(*suppressed,1850));
    check(!npc_profane.npc_aura_snapshot(fresh,1850));
    check(npc_profane.npc_energy_snapshot(fresh)->current==35);
    std::cout<<"World Well corpse selection, failed publication and one-use claims passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
