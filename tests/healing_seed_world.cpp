#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>

using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Healing Seed World line "+std::to_string(at.line()));
}
int main()try{
    world::Instance w;
    auto box=std::make_shared<world::Outbox>();
    for(unsigned id:{1u,2u,3u}){
        w.join(id,"Actor",float(id*50),0,id==1?box:std::make_shared<world::Outbox>(),
            0,{}, {},100,id==3?2:1);
        w.set_vitals(id,{500,1000,100,100},100);
    }
    check(w.party_action(1,158,2,100));
    check(w.party_action(2,154,1,100));
    const auto source=*w.effect_scope(1),target=*w.effect_scope(2),enemy=*w.effect_scope(3);
    const auto plan=*skills::effects::healing_seed(12);
    box->drain();
    check(!w.complete_healing_seed(source,source,plan,0,1000));
    check(!w.complete_healing_seed(source,enemy,plan,0,1000));
    auto stale=target;++stale.generation;
    check(!w.complete_healing_seed(source,stale,plan,0,1000));
    auto forged=plan;++forged.healing;
    check(!w.complete_healing_seed(source,target,forged,0,1000));
    check(w.complete_healing_seed(source,target,plan,0,1000));
    auto status=w.status_snapshot(target,1000);
    check(status&&status->size()==1&&status->front().skill==274&&
        status->front().cause==1&&status->front().expires==11000);
    auto wire=box->drain();
    check(wire.size()>=2&&wire[0]==combat::detail::integer(58,1,0)&&
        wire[1]==*skills::particles::targeted_body_2022(2,1,274));
    check(std::count(wire.begin(),wire.end(),combat::detail::integer(58,1,0))==1);
    check(std::count(wire.begin(),wire.end(),
        *skills::particles::targeted_body_2022(2,1,274))==1);
    check(w.complete_healing_seed(source,target,plan,0,1001));
    status=w.status_snapshot(target,1001);
    check(status&&status->size()==1&&status->front().expires==11001);
    check(w.status_snapshot(target,11001)->empty());
    {
        world::Instance fight;auto observer=std::make_shared<world::Outbox>();
        world::Instance::Npc friendly;
        friendly.id=friendly.definition.id=100;
        friendly.x=110;friendly.y=0;
        friendly.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition npc;
        npc.binding.map=146;npc.binding.actor=npc.binding.definition=100;
        npc.relation=combat::npcs::Relation::allied_to_players;
        npc.spawn_health=combat::Health{500,1000};npc.damageable=true;
        npc.healing_allowed=true;npc.healing_evidence="authored Healing Seed fixture";
        friendly.combat_definition=npc;
        fight.configure_npcs({friendly});
        for(unsigned id:{1u,2u,3u,4u}){
            fight.join(id,"Actor",id==1?0.f:id==2?50.f:id==3?90.f:60.f,
                0,id==1?observer:std::make_shared<world::Outbox>(),0,{}, {},100,
                id==4?2:1);
            fight.set_vitals(id,{500,1000,100,100},100);
        }
        check(fight.party_action(1,158,2,100));
        check(fight.party_action(2,154,1,100));
        const auto monk=*fight.effect_scope(1),owner=*fight.effect_scope(2),
            attacker=*fight.effect_scope(4);
        check(fight.complete_healing_seed(monk,owner,plan,0,100));
        fight.join(5,"Distant ally",2000,0,std::make_shared<world::Outbox>(),
            0,{}, {},100,1);
        fight.set_vitals(5,{500,1000,100,100},100);
        check(fight.settle_resources(*fight.effect_scope(5),2000));
        check(fight.physical_armor(owner,60));
        combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};
        check(fight.ordinary_weapon(attacker,sword,20));
        fight.configure_attack_miss_draws([]{return .5;});
        observer->drain();
        check(fight.ordinary_attack(attacker,2,1000));
        fight.tick(1500);fight.cancel_ordinary_attack(attacker);
        check(fight.resources(2)->health==505);
        check(fight.resources(1)->health==525);
        check(fight.resources(3)->health==525);
        check(fight.resources(4)->health==500);
        check(fight.resources(5)->health==500);
        check(fight.npc_combat_snapshot(*fight.effect_scope(100))->health->current==525);
        const auto post=observer->drain();
        const auto heal=skills::particles::health_change_2022(3,1,25,1000);
        check(std::count(post.begin(),post.end(),heal)==1);
        fight.tick(1600);
        check(fight.resources(3)->health==525);
        fight.tick(10100);observer->drain();
        check(fight.status_snapshot(owner,10100)->empty());
        check(fight.ordinary_attack(attacker,2,10101));
        fight.tick(10601);fight.cancel_ordinary_attack(attacker);
        const auto after_expiry=observer->drain();
        check(std::count(after_expiry.begin(),after_expiry.end(),heal)==0);
    }
    {
        world::Instance lethal;
        for(unsigned id:{1u,2u,3u}){
            lethal.join(id,"Actor",float(id*40),0,std::make_shared<world::Outbox>(),
                0,{}, {},100,id==3?2:1);
            lethal.set_vitals(id,{id==2?10u:500u,1000,100,100},100);
        }
        check(lethal.party_action(1,158,2,100));
        check(lethal.party_action(2,154,1,100));
        const auto monk=*lethal.effect_scope(1),owner=*lethal.effect_scope(2),
            attacker=*lethal.effect_scope(3);
        check(lethal.complete_healing_seed(monk,owner,plan,0,100));
        check(lethal.physical_armor(owner,60));
        combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};
        check(lethal.ordinary_weapon(attacker,sword,20));
        lethal.configure_attack_miss_draws([]{return .5;});
        check(lethal.ordinary_attack(attacker,2,1000));
        lethal.tick(1500);lethal.cancel_ordinary_attack(attacker);
        check(lethal.resources(2)->health==0);
        check(lethal.resources(1)->health==525);
    }
    {
        world::Instance skill_hit;
        for(unsigned id:{1u,2u,3u}){
            skill_hit.join(id,"Actor",float(id*40),0,
                std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);
            skill_hit.set_vitals(id,{500,1000,100,100},100);
        }
        check(skill_hit.party_action(1,158,2,100));
        check(skill_hit.party_action(2,154,1,100));
        const auto monk=*skill_hit.effect_scope(1),owner=*skill_hit.effect_scope(2),
            attacker=*skill_hit.effect_scope(3);
        check(skill_hit.complete_healing_seed(monk,owner,plan,0,100));
        check(skill_hit.physical_armor(owner,60));
        combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};
        check(skill_hit.ordinary_weapon(attacker,sword,20));
        skill_hit.configure_attack_miss_draws([]{return .5;});
        check(skill_hit.melee_attack(attacker,owner,sword,
            *combat::weapon::attack_skill(322,0),1000));
        check(skill_hit.resources(1)->health==525);
        check(skill_hit.resources(2)->health==495);
    }
    for(const bool illusionary:{false,true}){
        world::Instance bonus;auto observer=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u,3u}){
            bonus.join(id,"Actor",float(id*40),0,
                id==1?observer:std::make_shared<world::Outbox>(),0,{}, {},100,
                id==3?2:1);
            bonus.set_vitals(id,{500,1000,100,100},100);
        }
        check(bonus.party_action(1,158,2,100));
        check(bonus.party_action(2,154,1,100));
        const auto monk=*bonus.effect_scope(1),owner=*bonus.effect_scope(2),
            attacker=*bonus.effect_scope(3);
        check(bonus.complete_healing_seed(monk,owner,plan,0,100));
        check(bonus.elemental_armor(owner,60,60));
        combat::weapon::Owned weapon{1,1,{20,20,5},12,false,false,27};
        if(illusionary){
            weapon={1,1,{20,20,2},12,false,false,27};
            skills::Slots bar{};bar[0]={33,0};
            check(bonus.configure_adrenaline(attacker,bar,100));
            check(bonus.status_mutation(attacker,33,123,12,100,30100,true));
            check(bonus.physical_armor(owner,60));
        }else check(bonus.conjure_enchantment(attacker,
            *skills::effects::conjure(182,12),100));
        check(bonus.ordinary_weapon(attacker,weapon,20));
        bonus.configure_attack_miss_draws([]{return .5;});
        observer->drain();
        check(bonus.ordinary_attack(attacker,2,1000));
        bonus.tick(1500);
        bonus.cancel_ordinary_attack(attacker);
        if(illusionary){
            check(bonus.resources(1)->health==525);
            check(bonus.resources(2)->health==491);
        }else{
            check(bonus.resources(1)->health==550);
            check(bonus.resources(2)->health==513);
        }
    }
    for(const bool dancing:{false,true}){
        world::Instance spell;
        for(unsigned id:{1u,2u,3u,4u}){
            spell.join(id,"Actor",id==4?0.f:id==1?540.f:id==2?600.f:620.f,
                0,std::make_shared<world::Outbox>(),0,{}, {},100,id==4?2:1);
            spell.set_vitals(id,{500,1000,100,100},100);
        }
        check(spell.party_action(1,158,2,100));
        check(spell.party_action(2,154,1,100));
        const auto monk=*spell.effect_scope(1),owner=*spell.effect_scope(2),
            attacker=*spell.effect_scope(4);
        check(spell.complete_healing_seed(monk,owner,plan,0,100));
        check(spell.combat_armor(owner,60));
        if(dancing){
            check(spell.dancing_daggers(attacker,owner,
                *skills::effects::dancing_daggers(15),20,1000));
            spell.tick(1334);spell.tick(1500);spell.tick(1660);
            spell.tick(1800);spell.tick(2500);
            check(spell.resources(1)->health==575);
            check(spell.resources(2)->health==470);
            check(spell.resources(3)->health==575);
        }else{
            check(spell.stone_daggers(attacker,owner,
                *skills::effects::stone_daggers(15),20,1000));
            spell.tick(1334);spell.tick(1500);spell.tick(1868);
            check(spell.resources(1)->health==550);
            check(spell.resources(2)->health==484);
            check(spell.resources(3)->health==550);
        }
    }
    {
        world::Instance miss;
        for(unsigned id:{1u,2u,3u}){
            miss.join(id,"Actor",id==3?0.f:id==1?540.f:600.f,0,
                std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);
            miss.set_vitals(id,{500,1000,100,100},100);
        }
        check(miss.party_action(1,158,2,100));
        check(miss.party_action(2,154,1,100));
        const auto monk=*miss.effect_scope(1),owner=*miss.effect_scope(2),
            attacker=*miss.effect_scope(3);
        check(miss.complete_healing_seed(monk,owner,plan,0,100));
        check(miss.combat_armor(owner,60));
        check(miss.dancing_daggers(attacker,owner,
            *skills::effects::dancing_daggers(15),20,1000));
        miss.move(2,600,500,1050);miss.tick(3000);
        check(miss.resources(1)->health==500);
        check(miss.resources(2)->health==500);
    }
    for(const unsigned skill:{222u,1369u}){
        world::Instance lightning;
        for(unsigned id:{1u,2u,3u,4u}){
            lightning.join(id,"Actor",id==4?0.f:id==1?160.f:id==2?200.f:230.f,
                0,std::make_shared<world::Outbox>(),0,{}, {},100,id==4?2:1);
            lightning.set_vitals(id,{500,1000,100,100},100);
        }
        check(lightning.party_action(1,158,2,100));
        check(lightning.party_action(2,154,1,100));
        const auto monk=*lightning.effect_scope(1),owner=*lightning.effect_scope(2),
            attacker=*lightning.effect_scope(4);
        check(lightning.complete_healing_seed(monk,owner,plan,0,100));
        check(lightning.lightning_armor(owner,80));
        if(skill==1369)check(lightning.configure_projectiles({100,32}));
        check(lightning.lightning(attacker,owner,
            *skills::effects::lightning(skill,12),20,1000));
        if(skill==1369)lightning.tick(1100);
        check(lightning.resources(1)->health==525);
        check(lightning.resources(2)->health==484);
        check(lightning.resources(3)->health==525);
    }
    {
        world::Instance chain;
        for(unsigned id:{1u,2u,3u,4u}){
            chain.join(id,"Actor",id==1?0.f:id==2?100.f:id==3?110.f:400.f,
                0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?2:1);
            chain.set_vitals(id,{500,1000,100,100},100);
        }
        check(chain.party_action(4,158,2,100));
        check(chain.party_action(2,154,4,100));
        const auto monk=*chain.effect_scope(4),owner=*chain.effect_scope(2),
            attacker=*chain.effect_scope(1);
        check(chain.complete_healing_seed(monk,owner,plan,0,100));
        check(chain.lightning_armor(owner,80));
        check(chain.lightning_armor(*chain.effect_scope(3),80));
        check(chain.activate_overcast_energy(attacker,866,10,1,200));
        check(chain.lightning(attacker,owner,*skills::effects::lightning(842,12),20,1000));
        check(chain.resources(2)->health==492);
        check(chain.resources(3)->health==454);
        check(chain.resources(4)->health==500);
    }
    std::cout<<"PASS trusted Healing Seed application and ordinary direct-hit group heal\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
