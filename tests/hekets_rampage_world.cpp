#include "warrior_condition_fixture.h"
#include "gw_hekets_rampage.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Heket World line "+std::to_string(at.line()));
}
int main()try{
    for(bool npc:{false,true}){
        warrior_test::Fixture f(341,npc);
        auto& world=f.world;
        const auto owner=f.source;
        const auto plan=*skills::effects::hekets_rampage(0);
        check(world.status_mutation(owner,1728,200,0,100,100+plan.duration,true));
        check(world.attack_duration(owner,1000,1000,false)==670);
        auto status=world.status_snapshot(owner,1000);
        check(status&&std::any_of(status->begin(),status->end(),[](const auto& entry){return entry.skill==1728;}));
        f.box->drain();
        auto forged=*combat::weapon::attack_skill(341,12);++forged.bonus;
        check(!world.melee_attack(owner,f.target,f.weapon,forged,1000));
        check(world.attack_duration(owner,1000,1000,false)==670);
        check(world.melee_attack(owner,f.target,f.weapon,*combat::weapon::attack_skill(341,12),1000));
        const auto packets=f.box->drain();
        const auto remove=skills::casting::effect_remove(owner.actor,200);
        check(std::count(packets.begin(),packets.end(),remove)==1);
        check(world.attack_duration(owner,1000,1001,false)==1000);
        status=world.status_snapshot(owner,1001);
        check(status&&std::none_of(status->begin(),status->end(),[](const auto& entry){return entry.skill==1728;}));
    }
    // The frozen capture's Triple Shot is a three-arrow release. It must
    // retain the stance through activation, then end it exactly once when
    // the first arrow is accepted, before any later arrow can use it.
    for(bool npc:{false,true}){
        warrior_test::Fixture f(1953,npc);auto& w=f.world;
        const auto owner=f.source;const auto plan=*skills::effects::hekets_rampage(0);
        check(w.status_mutation(owner,1728,201,0,100,100+plan.duration,true));
        check(w.attack_duration(owner,1000,1000,true)==670);
        f.box->drain();
        auto forged=*combat::weapon::attack_skill(1953,0);++forged.arrows;
        check(!w.melee_attack(owner,f.target,f.weapon,forged,1000));
        check(w.status_snapshot(owner,1000)->size()==1);
        check(w.melee_attack(owner,f.target,f.weapon,*combat::weapon::attack_skill(1953,0),1000));
        const auto wire=f.box->drain();const auto remove=skills::casting::effect_remove(owner.actor,201);
        check(std::count(wire.begin(),wire.end(),remove)==1);
        check(w.status_snapshot(owner,1000)->empty());
        check(w.attack_duration(owner,1000,1001,true)==1000);
    }
    // A cancelled or interrupted activation never reaches the release boundary.
    // Exercise World capabilities rather than merely rejecting a forged plan.
    for(bool npc:{false,true})for(bool interrupted:{false,true}){
        warrior_test::Fixture f(359,npc);auto& w=f.world;
        check(w.status_mutation(f.source,1728,202,0,100,5100,true));
        const auto plan=*combat::weapon::attack_skill(359,12);
        const auto permit=w.prepare_melee_attack(f.source,f.target,f.weapon,plan,1000);
        check(permit&&permit->release_time()==1335);
        check(!w.melee_attack(f.source,f.target,f.weapon,plan,1334,permit));
        check(w.attack_duration(f.source,1000,1334)==670);
        if(interrupted){
            check(w.attack_activity(f.source,1335,1000));
            check(w.interrupt_attack(f.source,1100));
        }else permit->cancel();
        f.box->drain();const auto health=f.health();
        check(!w.melee_attack(f.source,f.target,f.weapon,plan,1335,permit));
        const auto wire=f.box->drain();
        check(std::count(wire.begin(),wire.end(),skills::casting::effect_remove(f.source.actor,202))==0);
        check(w.attack_duration(f.source,1000,1335)==670&&f.health()==health);
    }
    // Barrage and Volley validate their entire recipient set before release.
    // An unknown secondary armor must retain Heket; a later valid batch ends
    // it once, before any projectile becomes visible, regardless of target count.
    for(unsigned skill:{395u,2144u}){
        warrior_test::Fixture f(skill,false);auto& w=f.world;
        w.join(3,"Secondary",70,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(3,{1000,1000,30,30},100);
        const auto secondary=*w.effect_scope(3);
        check(w.status_mutation(f.source,1728,203,0,100,5100,true));
        check(w.prepare_diversion_skill_use(f.source,{skill,0},1,1000));
        f.box->drain();std::vector<world::Packet> visuals;
        check(!w.completed_bow_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(skill,12),1,1000,1000,0,&visuals));
        check(visuals.empty()&&w.attack_duration(f.source,1000,1000)==670);
        check(w.physical_armor(secondary,60));
        check(w.completed_bow_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(skill,12),1,1000,1000,0,&visuals).has_value());
        check(visuals.size()==2);
        const auto remove=skills::casting::effect_remove(f.source.actor,203);
        const auto before_launch=f.box->drain();
        check(std::count(before_launch.begin(),before_launch.end(),remove)==1);
        check(std::none_of(before_launch.begin(),before_launch.end(),[](const auto& p){return p.size()==29&&p[0]==163&&p[1]==128;}));
        check(w.attack_duration(f.source,1000,1000)==1000);
        w.skill_event(1,visuals,1000);w.tick(1100);
        check(w.take_attack_outcomes(f.source).size()==2);
        const auto after_launch=f.box->drain();
        check(std::count(after_launch.begin(),after_launch.end(),remove)==0);
    }
    {
        world::Instance w;world::Instance::Npc n;
        n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={500,500};
        d.spawn_energy=combat::npcs::Energy{20,20};d.energy_pips=0;
        d.energy_evidence="test";d.energy_pips_evidence="test";n.combat_definition=d;w.configure_npcs({n});
        auto box=std::make_shared<world::Outbox>();w.join(1,"Observer",0,0,box,0,{}, {},100,1);
        const auto actor=*w.effect_scope(100);
        check(!w.npc_hekets_rampage(actor,0,100,""));
        check(w.npc_hekets_rampage(actor,0,100,"reviewed fixture"));
        check(w.npc_energy_snapshot(actor)->current==15);
        check(w.attack_duration(actor,1000,100)==670);
        check(!w.npc_hekets_rampage(actor,0,101,"reviewed fixture"));
        const auto ordinary=w.begin_npc_action(actor,combat::npcs::control::Action::attack,200,300);
        check(ordinary&&w.complete_npc_action(*ordinary,300));
        check(w.attack_duration(actor,1000,300)==670);
        const auto token=w.begin_npc_action(actor,combat::npcs::control::Action::attack,500,1000,skills::Slot{1953,0});
        check(token&&w.attack_duration(actor,1000,999)==670);
        check(!w.complete_npc_action(*token,999));
        check(w.attack_duration(actor,1000,999)==670);
        box->drain();check(w.complete_npc_action(*token,1000));
        check(w.attack_duration(actor,1000,1000)==1000);
        const auto wire=box->drain();
        check(std::count_if(wire.begin(),wire.end(),[](const auto& p){return p.size()==10&&p[0]==67&&p[1]==0;})==1);
    }
    std::cout<<"PASS Heket player/NPC IAS, payment, recharge and attack-skill release ends\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
