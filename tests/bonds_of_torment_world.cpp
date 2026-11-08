#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bonds World line "+std::to_string(at.line()));
}
static world::Instance::Npc tormenter(unsigned id,double health=500){
    world::Instance::Npc n;
    n.id=n.definition.id=id;n.definition.level=20;n.x=100;n.y=0;
    n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;
    d.binding.map=146;d.binding.actor=d.binding.definition=id;
    d.binding.level=n.definition.level;
    d.relation=combat::npcs::Relation::hostile_to_players;
    d.spawn_health=combat::Health{health,unsigned(health)};
    d.damageable=true;d.spawn_energy=combat::npcs::Energy{20,20};d.energy_pips=0;
    d.physical_armor=60;d.physical_armor_evidence="test fixture";
    n.combat_definition=d;return n;
}
int main()try{
    const auto plan=*skills::effects::bonds_of_torment_npc();
    world::Instance w;
    w.configure_npcs({tormenter(100)});
    auto attacker_box=std::make_shared<world::Outbox>();
    auto victim_box=std::make_shared<world::Outbox>();
    w.join(1,"Attacker",0,0,attacker_box,0,{}, {},100,1);
    w.join(2,"Victim",120,0,victim_box,0,{}, {},100,2);
    w.join(3,"Second victim",140,0,std::make_shared<world::Outbox>(),0,{}, {},100,3);
    w.set_vitals(1,{500,500,100,100},100);
    w.set_vitals(2,{500,500,100,100},100);
    w.set_vitals(3,{500,500,100,100},100);
    const auto attacker=*w.effect_scope(1),victim=*w.effect_scope(2),
        second=*w.effect_scope(3),npc=*w.effect_scope(100);
    check(w.elemental_armor(victim,60,60));
    check(w.elemental_armor(second,60,60));
    check(w.stage_npc_bonds_of_torment(npc,victim,plan,200));
    check(w.stage_npc_bonds_of_torment(npc,second,plan,250));
    const auto status=victim_box->drain();
    check(!status.empty());
    const auto hit=w.damage_npc(attacker,npc,{combat::Kind::armor_ignoring,100,20,1881},1248,300);
    check(hit.reason==combat::Reason::accepted&&hit.applied==100);
    std::vector<unsigned> damaged_targets;
    for(const auto& packet:victim_box->drain())if(packet.size()>=14&&packet[0]==162&&packet[1]==0)
        damaged_targets.push_back(unsigned(packet[6])|(unsigned(packet[7])<<8)|
                                  (unsigned(packet[8])<<16)|(unsigned(packet[9])<<24));
    check(damaged_targets==std::vector<unsigned>({100,2,3}));
    check(w.resources(2)->health==400);
    check(w.resources(3)->health==400);
    check(w.stage_npc_bonds_of_torment(npc,victim,plan,400));
    check(w.damage_npc(attacker,npc,{combat::Kind::armor_ignoring,50,20,1881},1248,500).reason==combat::Reason::accepted);
    check(w.resources(2)->health==350); // Reapplication is one live link.
    check(w.resources(3)->health==350);
    check(!w.stage_npc_bonds_of_torment({npc.instance,npc.actor,npc.generation+1},victim,plan,600));
    check(!w.stage_npc_bonds_of_torment(npc,{victim.instance,victim.actor,victim.generation+1},plan,600));
    check(w.damage_npc(attacker,npc,{combat::Kind::armor_ignoring,350,20,1881},1248,700).killed);
    check(w.resources(2)->health==0); // Lethal caster loss propagates before retirement.
    check(w.resources(3)->health==0);
    check(!w.stage_npc_bonds_of_torment(npc,victim,plan,800));
    {
        world::Instance expired;
        expired.configure_npcs({tormenter(100)});
        expired.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        expired.join(2,"Victim",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        expired.set_vitals(1,{500,500,100,100},100);
        expired.set_vitals(2,{500,500,100,100},100);
        const auto a=*expired.effect_scope(1),v=*expired.effect_scope(2),n=*expired.effect_scope(100);
        check(expired.elemental_armor(v,60,60));
        check(expired.stage_npc_bonds_of_torment(n,v,plan,200));
        check(expired.damage_npc(a,n,{combat::Kind::armor_ignoring,100,20,1881},1248,30200).reason==combat::Reason::accepted);
        check(expired.resources(2)->health==500);
    }
    {
        world::Instance replacement;
        replacement.configure_npcs({tormenter(100)});
        auto box=std::make_shared<world::Outbox>();
        replacement.join(1,"Attacker",0,0,box,0,{}, {},100,1);
        replacement.join(2,"Victim",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        replacement.set_vitals(1,{500,500,100,100},100);
        replacement.set_vitals(2,{500,500,100,100},100);
        const auto a=*replacement.effect_scope(1),v=*replacement.effect_scope(2),n=*replacement.effect_scope(100);
        check(replacement.elemental_armor(v,60,60));
        skills::Slots bar{};bar[0]={33,0};
        check(replacement.configure_adrenaline(a,bar,100));
        check(replacement.status_mutation(a,33,123,12,100,30100,true));
        combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};
        check(replacement.ordinary_weapon(a,sword,20));
        check(replacement.stage_npc_bonds_of_torment(n,v,plan,200));
        box->drain();
        check(replacement.ordinary_attack(a,n.actor,1000));
        replacement.tick(1500);
        replacement.cancel_ordinary_attack(a);
        const auto npc_after=replacement.npc_combat_snapshot(n);
        check(npc_after&&npc_after->health&&npc_after->health->current==466);
        check(replacement.resources(2)->health==466);
        const auto packets=box->drain();
        std::vector<unsigned> damaged_targets;
        for(const auto& packet:packets)if(packet.size()>=14&&packet[0]==162&&packet[1]==0)
            damaged_targets.push_back(unsigned(packet[6])|(unsigned(packet[7])<<8)|
                                      (unsigned(packet[8])<<16)|(unsigned(packet[9])<<24));
        check(damaged_targets==std::vector<unsigned>({100,2}));
    }
    {
        world::Instance ordinary;
        world::Instance::Npc caster;
        caster.id=caster.definition.id=100;caster.x=10;
        caster.definition.level=20;
        caster.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition combat_caster;
        combat_caster.binding.map=146;
        combat_caster.binding.actor=combat_caster.binding.definition=100;
        combat_caster.binding.level=20;
        combat_caster.relation=combat::npcs::Relation::hostile_to_players;
        combat_caster.spawn_health=combat::Health{500,500};
        combat_caster.damageable=true;
        combat_caster.physical_armor=500;
        combat_caster.physical_armor_evidence="test";
        combat_caster.fire_armor=combat_caster.cold_armor=
            combat_caster.lightning_armor=combat_caster.earth_armor=60;
        caster.combat_definition=combat_caster;
        ordinary.configure_npcs({caster});
        auto box=std::make_shared<world::Outbox>();
        ordinary.join(1,"Attacker",0,0,box,0,{}, {},100,1);
        ordinary.join(2,"Victim",20,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        ordinary.set_vitals(1,{500,500,100,100},100);
        ordinary.set_vitals(2,{500,500,100,100},100);
        const auto a=*ordinary.effect_scope(1),v=*ordinary.effect_scope(2),n=*ordinary.effect_scope(100);
        check(ordinary.elemental_armor(v,60,60));
        check(ordinary.stage_npc_bonds_of_torment(n,v,plan,100));
        check(ordinary.conjure_enchantment(a,*skills::effects::conjure(207,12),100));
        combat::weapon::Owned sword{12,1,{20,20,3},12,false,false,27};
        check(ordinary.ordinary_weapon(a,sword,20));
        check(ordinary.configure_projectiles({100,2}));
        box->drain();
        check(ordinary.ordinary_attack(a,n.actor,100));
        ordinary.tick(600);
        ordinary.cancel_ordinary_attack(a);
        const auto health=ordinary.npc_combat_snapshot(n)->health->current;
        check(health<500&&ordinary.resources(2)->health==health);
        unsigned propagation_packets=0;
        for(const auto& packet:box->drain())if(packet.size()>=14&&packet[0]==162&&packet[1]==0){
            const auto target=unsigned(packet[6])|(unsigned(packet[7])<<8)|
                (unsigned(packet[8])<<16)|(unsigned(packet[9])<<24);
            if(target==2)++propagation_packets;
        }
        check(propagation_packets==2); // Weapon and Conjure are distinct losses.
    }
    {
        world::Instance ignite;
        auto burning_caster=tormenter(100);
        burning_caster.combat_definition->fire_armor=60;
        ignite.configure_npcs({burning_caster});
        ignite.configure_attack_miss_draws([]{return .5;});
        check(ignite.configure_projectiles({100,2}));
        auto box=std::make_shared<world::Outbox>();
        ignite.join(1,"Ranger",0,0,box,0,{}, {},100,1);
        ignite.join(2,"Bonded",300,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        ignite.set_vitals(1,{500,500,100,100},100);
        ignite.set_vitals(2,{500,500,100,100},100);
        const auto a=*ignite.effect_scope(1),v=*ignite.effect_scope(2),n=*ignite.effect_scope(100);
        check(ignite.elemental_armor(v,60,60));
        combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};
        check(ignite.ordinary_weapon(a,bow,20));
        check(ignite.status_mutation(a,431,1,12,100,24100,true));
        check(ignite.stage_npc_bonds_of_torment(n,v,plan,200));
        box->drain();
        check(ignite.ordinary_attack(a,n.actor,250));
        ignite.tick(750);ignite.tick(850);ignite.cancel_ordinary_attack(a);
        const auto npc_health=ignite.npc_combat_snapshot(n)->health->current;
        check(npc_health==465&&ignite.resources(2)->health==npc_health);
        unsigned bond_packets=0;
        for(const auto& packet:box->drain())if(packet.size()>=14&&packet[0]==162&&packet[1]==0){
            const auto target=unsigned(packet[6])|(unsigned(packet[7])<<8)|
                (unsigned(packet[8])<<16)|(unsigned(packet[9])<<24);
            if(target==2)++bond_packets;
        }
        check(bond_packets==2); // Arrow and adjacent fire are distinct committed losses.
    }
    {
        world::Instance cast;
        cast.configure_npcs({tormenter(100)});
        auto box=std::make_shared<world::Outbox>();
        cast.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        cast.join(2,"Victim",120,0,box,0,{}, {},100,2);
        cast.set_vitals(1,{500,500,100,100},100);
        cast.set_vitals(2,{500,500,100,100},100);
        const auto a=*cast.effect_scope(1),v=*cast.effect_scope(2),n=*cast.effect_scope(100);
        check(cast.elemental_armor(v,60,60));
        check(cast.configure_npc_spellbook(n,{{{1881,0},1248,"fixed DoA cast checkpoint"}}));
        box->drain();
        const auto token=cast.begin_npc_bonds_of_torment(n,v,1000);
        check(bool(token)&&token->skill&&token->skill->skill==1881&&token->completes==2000);
        check(cast.npc_energy_snapshot(n)->current==10);
        check(!cast.complete_npc_action(*token,2000));
        check(!cast.complete_npc_bonds_of_torment(*token,1999));
        check(cast.complete_npc_bonds_of_torment(*token,2000));
        check(!cast.complete_npc_bonds_of_torment(*token,2000));
        const auto packets=box->drain();
        check(!skills::particles::targeted_body_2022(v.actor,n.actor,1881));
        check(std::count(packets.begin(),packets.end(),skills::casting::activation(n.actor,v.actor,1881))==1);
        check(std::count(packets.begin(),packets.end(),combat::detail::integer(58,n.actor,0))==1);
        check(std::count_if(packets.begin(),packets.end(),[](const auto& packet){return packet.size()>=2&&packet[0]==65&&packet[1]==0;})==1);
        check(!cast.begin_npc_bonds_of_torment(n,v,3000));
        check(cast.damage_npc(a,n,{combat::Kind::armor_ignoring,50,20,1881},1248,3100).reason==combat::Reason::accepted);
        check(cast.resources(2)->health==450);
        check(cast.begin_npc_bonds_of_torment(n,v,22000).has_value());
    }
    {
        world::Instance automatic;
        automatic.configure_npcs({tormenter(100)});
        automatic.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        automatic.join(2,"Victim",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        automatic.set_vitals(1,{500,500,100,100},100);
        automatic.set_vitals(2,{500,500,100,100},100);
        const auto a=*automatic.effect_scope(1),v=*automatic.effect_scope(2),n=*automatic.effect_scope(100);
        check(automatic.elemental_armor(v,60,60));
        check(automatic.configure_npc_spellbook(n,{{{1881,0},1248,"fixed DoA cast checkpoint"}}));
        const auto token=automatic.begin_npc_bonds_of_torment(n,v,1000);
        check(bool(token));
        automatic.tick(2500);
        check(!automatic.complete_npc_bonds_of_torment(*token,2500));
        check(automatic.damage_npc(a,n,{combat::Kind::armor_ignoring,75,20,1881},1248,2600).reason==combat::Reason::accepted);
        check(automatic.resources(2)->health==425);
    }
    {
        world::Instance autonomous;
        autonomous.configure_npcs({tormenter(100)});
        auto target_box=std::make_shared<world::Outbox>();
        autonomous.join(1,"Farther",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        autonomous.join(2,"Nearest",120,0,target_box,0,{}, {},100,2);
        autonomous.set_vitals(1,{500,500,100,100},100);
        autonomous.set_vitals(2,{500,500,100,100},100);
        const auto n=*autonomous.effect_scope(100);
        const auto farther=*autonomous.effect_scope(1);
        const auto nearest=*autonomous.effect_scope(2);
        check(autonomous.elemental_armor(nearest,60,60));
        check(!autonomous.configure_npc_bonds_ai(n)); // No reviewed loadout.
        check(autonomous.configure_npc_spellbook(n,{{{1881,0},1248,"fixed DoA cast checkpoint"}}));
        check(autonomous.configure_npc_bonds_ai(n));
        check(!autonomous.configure_npc_bonds_ai(n));
        target_box->drain();
        autonomous.tick(1000);
        check(autonomous.npc_energy_snapshot(n)->current==10);
        check(autonomous.resources(2)->health==500);
        autonomous.tick(2000);
        const auto emitted=target_box->drain();
        check(std::count(emitted.begin(),emitted.end(),
            skills::casting::activation(n.actor,nearest.actor,1881))==1);
        check(autonomous.damage_npc(farther,n,{combat::Kind::armor_ignoring,60,20,1881},1248,2100).reason==combat::Reason::accepted);
        check(autonomous.resources(2)->health==440);
        check(autonomous.resources(1)->health==500);
        autonomous.tick(2200);
        check(autonomous.npc_energy_snapshot(n)->current==10);
    }
    {
        world::Instance interrupted;
        interrupted.configure_npcs({tormenter(100)});
        interrupted.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        interrupted.join(2,"Victim",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        interrupted.set_vitals(1,{500,500,100,100},100);
        interrupted.set_vitals(2,{500,500,100,100},100);
        const auto a=*interrupted.effect_scope(1),v=*interrupted.effect_scope(2),n=*interrupted.effect_scope(100);
        check(interrupted.elemental_armor(v,60,60));
        check(interrupted.configure_npc_spellbook(n,{{{1881,0},1248,"fixed DoA cast checkpoint"}}));
        const auto token=interrupted.begin_npc_bonds_of_torment(n,v,1000);
        check(bool(token));
        check(interrupted.interrupt_skill(a,n,*skills::effects::interrupt_skill(23,0),20,1500));
        interrupted.tick(2500);
        check(!interrupted.complete_npc_bonds_of_torment(*token,2500));
        check(interrupted.damage_npc(a,n,{combat::Kind::armor_ignoring,75,20,1881},1248,2600).reason==combat::Reason::accepted);
        check(interrupted.resources(2)->health==500);
        check(interrupted.npc_energy_snapshot(n)->current==10); // Paid start is not refunded.
    }
    {
        world::Instance dead_caster;
        dead_caster.configure_npcs({tormenter(100,50)});
        dead_caster.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        dead_caster.join(2,"Victim",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        dead_caster.set_vitals(1,{500,500,100,100},100);
        dead_caster.set_vitals(2,{500,500,100,100},100);
        const auto a=*dead_caster.effect_scope(1),v=*dead_caster.effect_scope(2),n=*dead_caster.effect_scope(100);
        check(dead_caster.elemental_armor(v,60,60));
        check(dead_caster.configure_npc_spellbook(n,{{{1881,0},1248,"fixed DoA cast checkpoint"}}));
        const auto token=dead_caster.begin_npc_bonds_of_torment(n,v,1000);
        check(bool(token));
        check(dead_caster.damage_npc(a,n,{combat::Kind::armor_ignoring,50,20,1881},1248,1500).killed);
        dead_caster.tick(2500);
        check(!dead_caster.complete_npc_bonds_of_torment(*token,2500));
        check(dead_caster.resources(2)->health==500);
    }
    {
        world::Instance signet;
        signet.configure_npcs({tormenter(100)});
        auto box=std::make_shared<world::Outbox>();
        signet.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
        signet.join(2,"Bonded",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        signet.set_vitals(1,{500,500,100,100},100);
        signet.set_vitals(2,{500,500,100,100},100);
        const auto a=*signet.effect_scope(1),v=*signet.effect_scope(2),n=*signet.effect_scope(100);
        check(signet.elemental_armor(v,60,60));
        check(signet.stage_npc_bonds_of_torment(n,v,plan,200));
        box->drain();
        check(signet.unnatural_signet(a,n,*skills::effects::unnatural_signet(12),20,300));
        check(signet.npc_combat_snapshot(n)->health->current==437);
        check(signet.resources(2)->health==437);
        unsigned bond_packets=0;
        for(const auto& packet:box->drain())if(packet.size()>=14&&packet[0]==162&&packet[1]==0){
            const auto target=unsigned(packet[6])|(unsigned(packet[7])<<8)|
                (unsigned(packet[8])<<16)|(unsigned(packet[9])<<24);
            if(target==2)++bond_packets;
        }
        check(bond_packets==1);
    }
    {
        world::Instance shatter;
        shatter.configure_npcs({tormenter(100)});
        auto box=std::make_shared<world::Outbox>();
        shatter.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
        shatter.join(2,"Bonded",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        shatter.set_vitals(1,{500,500,100,100},100);
        shatter.set_vitals(2,{500,500,100,100},100);
        const auto a=*shatter.effect_scope(1),v=*shatter.effect_scope(2),n=*shatter.effect_scope(100);
        check(shatter.elemental_armor(v,60,60));
        check(shatter.stage_npc_bonds_of_torment(n,v,plan,200));
        check(shatter.stage_npc_shield_deflection(n,*skills::effects::shield_deflection(0),200));
        box->drain();
        check(shatter.enchantment_removal(a,n,*skills::effects::enchantment_removal(69,12),20,300));
        check(shatter.npc_combat_snapshot(n)->health->current==417);
        check(shatter.resources(2)->health==417);
        unsigned bond_packets=0;
        for(const auto& packet:box->drain())if(packet.size()>=14&&packet[0]==162&&packet[1]==0){
            const auto target=unsigned(packet[6])|(unsigned(packet[7])<<8)|
                (unsigned(packet[8])<<16)|(unsigned(packet[9])<<24);
            if(target==2)++bond_packets;
        }
        check(bond_packets==1);
    }
    {
        world::Instance interrupted_damage;
        interrupted_damage.configure_npcs({tormenter(100)});
        auto box=std::make_shared<world::Outbox>();
        interrupted_damage.join(1,"Interrupter",0,0,box,0,{}, {},100,1);
        interrupted_damage.join(2,"Bonded",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        interrupted_damage.set_vitals(1,{500,500,100,100},100);
        interrupted_damage.set_vitals(2,{500,500,100,100},100);
        const auto a=*interrupted_damage.effect_scope(1),v=*interrupted_damage.effect_scope(2),n=*interrupted_damage.effect_scope(100);
        check(interrupted_damage.elemental_armor(v,60,60));
        check(interrupted_damage.stage_npc_bonds_of_torment(n,v,plan,200));
        check(interrupted_damage.configure_npc_spellbook(n,{{{1881,0},1248,"fixed DoA cast checkpoint"}}));
        check(bool(interrupted_damage.begin_npc_bonds_of_torment(n,v,1000)));
        box->drain();
        const auto interrupt=*skills::effects::interrupt_skill(23,0);
        check(interrupted_damage.interrupt_skill(a,n,interrupt,20,1500));
        check(interrupted_damage.npc_combat_snapshot(n)->health->current==500-int(interrupt.damage));
        check(interrupted_damage.resources(2)->health==500-int(interrupt.damage));
        unsigned propagated=0;
        for(const auto& packet:box->drain())if(packet.size()>=14&&packet[0]==162&&packet[1]==0){
            const auto recipient=unsigned(packet[6])|(unsigned(packet[7])<<8)|
                (unsigned(packet[8])<<16)|(unsigned(packet[9])<<24);
            if(recipient==2)++propagated;
        }
        check(propagated==1);
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
