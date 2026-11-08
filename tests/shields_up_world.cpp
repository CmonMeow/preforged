#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Shields Up World line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){
        world::Instance w;
        for(unsigned id=1;id<=4;++id){
            w.join(id,"Actor",id==3?1100.f:float(id*30),0,
                std::make_shared<world::Outbox>(),0,{}, {},100,id==4?2:1);
            w.set_vitals(id,{1000,1000,100,100},100);
        }
        for(unsigned id:{2u,3u}){
            check(w.party_action(1,158,id,100));
            check(w.party_action(id,154,1,100));
        }
        const auto source=*w.effect_scope(1),ally=*w.effect_scope(2),
            distant=*w.effect_scope(3),foe=*w.effect_scope(4);
        const auto plan=*shields_up(rank);
        check(w.stage_shields_up_party(source,plan,100));
        for(const auto recipient:{source,ally}){
            const auto statuses=w.status_snapshot(recipient,100);
            check(statuses&&statuses->size()==1&&statuses->front().skill==367&&
                statuses->front().expires==100+plan.duration);
            check(w.shields_up_player_armor(recipient,100,
                ShieldsUpDamage::projectile_attack)==60);
            check(w.shields_up_player_armor(recipient,100,
                ShieldsUpDamage::projectile_spell)==0);
        }
        check(w.status_snapshot(distant,100)->empty()&&
            w.status_snapshot(foe,100)->empty());
        check(w.stage_shields_up_party(source,plan,200));
        check(w.status_snapshot(ally,200)->size()==1);
        w.tick(200+plan.duration);
        check(w.shields_up_player_armor(ally,200+plan.duration,
            ShieldsUpDamage::projectile_attack)==0);
    }
    {
        world::Instance w;for(unsigned id:{1u,2u}){
            w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            w.set_vitals(id,{1000,1000,100,100},100);
        }
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        const auto source=*w.effect_scope(1),ally=*w.effect_scope(2);
        check(w.settle_resources(ally,200));
        check(!w.stage_shields_up_party(source,*shields_up(0),100));
        check(w.status_snapshot(source,100)->empty());
        check(w.status_snapshot(ally,100)->empty());
        check(w.stage_shields_up_party(source,*shields_up(0),200));
        check(w.kill(ally,201));
        check(w.shields_up_player_armor(ally,201,
            ShieldsUpDamage::projectile_attack)==0);
    }
    const auto wand_impact=[](bool shielded){
        world::Instance w;w.configure_attack_miss_draws([]{return .5;});
        for(unsigned id:{1u,2u}){
            w.join(id,"Actor",float((id-1)*100),0,
                std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{500,500,20,20},100);
        }
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.combat_armor(target,60)&&w.physical_armor(target,60)&&
            w.configure_projectiles({100,32}));
        if(shielded)check(w.stage_shields_up_party(target,*shields_up(0),100));
        combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
        check(w.ordinary_weapon(source,wand,20)&&w.ordinary_attack(source,2,100));
        w.tick(600);w.tick(1600);
        return w.resources(2)->health;
    };
    const auto plain=wand_impact(false),protected_health=wand_impact(true);
    check(plain<protected_health&&protected_health<500);
    {
        world::Instance w;
        std::vector<world::Instance::Npc> npcs;
        for(unsigned id:{100u,101u,102u}){
            world::Instance::Npc npc;npc.id=npc.definition.id=id;
            npc.x=100+float(id-100)*20;npc.y=0;
            npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;
            d.binding.map=146;d.binding.actor=d.binding.definition=id;
            d.relation=combat::npcs::Relation::allied_to_players;
            d.spawn_health=combat::Health{500,500};
            d.physical_armor=d.earth_armor=60;
            d.damageable=d.conditions_allowed=true;
            npc.combat_definition=d;npcs.push_back(npc);
        }
        w.configure_npcs(npcs);
        auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u,3u}){
            w.join(id,"Actor",0,0,id==1?box:std::make_shared<world::Outbox>(),
                0,{}, {},100,id==3?2:1);
            w.set_vitals(id,{500,500,100,100},100);
        }
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        const auto source=*w.effect_scope(1),member=*w.effect_scope(2),
            outsider=*w.effect_scope(3),owned=*w.effect_scope(100),
            nearby=*w.effect_scope(101),other_party=*w.effect_scope(102);
        auto stale=member;++stale.generation;
        check(!w.set_npc_party_owner(owned,stale));
        check(w.set_npc_party_owner(owned,member));
        check(w.set_npc_party_owner(other_party,outsider));
        box->drain();std::function<void()> publish;
        check(w.stage_shields_up_party(source,*shields_up(12),200,&publish));
        check(box->drain().empty()&&bool(publish));publish();
        const auto owned_status=w.status_snapshot(owned,200);
        check(owned_status&&owned_status->size()==1&&owned_status->front().skill==367);
        check(w.status_snapshot(nearby,200)->empty());
        check(w.status_snapshot(other_party,200)->empty());
        const auto first_effect=owned_status->front().instance;
        const auto packets=box->drain();
        check(std::any_of(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>6&&packet[0]==65&&packet[1]==0&&packet[2]==100;
        }));
        check(w.kill(member,250));
        check(w.stage_shields_up_party(source,*shields_up(12),300));
        const auto renewed_effect=w.status_snapshot(owned,300)->front().instance;
        check(renewed_effect!=first_effect); // Death does not dissolve party membership.
        w.leave(2);
        check(w.stage_shields_up_party(source,*shields_up(12),400));
        check(w.status_snapshot(owned,400)->front().instance==renewed_effect);
        box->drain();
        w.tick(200+shields_up(12)->duration);
        check(w.status_snapshot(owned,200+shields_up(12)->duration)->front().instance==renewed_effect);
        box->drain();
        w.tick(300+shields_up(12)->duration);
        check(w.status_snapshot(owned,300+shields_up(12)->duration)->empty());
        const auto expired=box->drain();
        check(std::count_if(expired.begin(),expired.end(),[&](const auto& packet){
            return packet.size()==10&&packet[0]==67&&packet[1]==0&&
                packet[2]==100&&packet[6]==renewed_effect;
        })==1);
        w.tick(301+shields_up(12)->duration);
        const auto later=box->drain();
        check(std::none_of(later.begin(),later.end(),[&](const auto& packet){
            return packet.size()==10&&packet[0]==67&&packet[1]==0&&
                packet[2]==100&&packet[6]==renewed_effect;
        }));
        check(w.set_npc_party_owner(owned,std::nullopt));
    }
    {
        const auto opposed_health=[](bool protected_ally,bool ranged){
            world::Instance w;
            std::vector<world::Instance::Npc> npcs;
            for(unsigned id:{100u,101u}){
                world::Instance::Npc npc;npc.id=npc.definition.id=id;
                npc.x=id==100?0.f:100.f;npc.y=0;
                npc.definition.level=4;
                npc.definition.visual_adjustment={0,0,0,100};
                combat::npcs::Definition definition;
                definition.binding.map=146;
                definition.binding.actor=definition.binding.definition=id;
                definition.binding.level=4;
                definition.relation=id==100?combat::npcs::Relation::allied_to_players:
                    combat::npcs::Relation::hostile_to_players;
                definition.spawn_health=combat::Health{500,500};
                definition.damageable=true;
                definition.physical_armor=60;
                npc.combat_definition=definition;npcs.push_back(npc);
            }
            w.configure_npcs(npcs);
            w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            w.set_vitals(1,{500,500,100,100},100);
            const auto owner=*w.effect_scope(1),ally=*w.effect_scope(100),enemy=*w.effect_scope(101);
            check(w.set_npc_party_owner(ally,owner));
            if(protected_ally)check(w.stage_shields_up_party(owner,*shields_up(12),100));
            const combat::ai::ordinary::Reviewed weapon{80,80,0,0,false,
                ranged?combat::ai::ordinary::Delivery::ranged:
                    combat::ai::ordinary::Delivery::melee,
                750,2000,500,{1200,32},"synthetic reviewed delivery regression"};
            check(w.configure_npc_opposed_ordinary(enemy,weapon,1000));
            check(w.begin_npc_opposed_ordinary(enemy,ally,1000).has_value());
            w.tick(1750);w.tick(2500);
            return w.npc_combat_snapshot(ally)->health->current;
        };
        const auto plain=opposed_health(false,true),shielded=opposed_health(true,true);
        check(plain<shielded&&shielded<500);
        const auto melee_plain=opposed_health(false,false);
        check(melee_plain<500&&melee_plain==opposed_health(true,false));
    }
    std::cout<<"Shields Up player/NPC party snapshots, status/expiry and player/NPC projectile-attack armor pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
