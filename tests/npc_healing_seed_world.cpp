#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC Healing Seed line "+std::to_string(at.line()));
}
static world::Instance::Npc seed_npc(unsigned id,combat::npcs::Relation relation,unsigned health,float x,bool allowed=true){
    world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};n.x=x;
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;d.relation=relation;
    d.spawn_health=combat::Health{double(health),1000};d.damageable=d.conditions_allowed=true;d.physical_armor=60;
    d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
    d.healing_allowed=allowed;d.healing_evidence="synthetic reviewed Seed fixture, not retail NPC statistics";
    n.combat_definition=d;return n;
}
int main()try{
    unsigned cases=0;
    // Health gain does not grant ordinary healing permission.
    {const auto d=*seed_npc(100,combat::npcs::Relation::allied_to_players,500,100,false).combat_definition;
     combat::npcs::LiveActor immune(d,{1,1,1,146,100});const auto life=immune.lifetime();
     check(!immune.heal_ally(life,25,100)&&immune.actor(life)->health->current==500);}
    for(unsigned rank:{0u,12u,20u})for(unsigned hp:{1u,500u})for(bool allowed:{false,true})for(bool expired:{false,true}){
        world::Instance w;auto out=std::make_shared<world::Outbox>();
        w.configure_npcs({seed_npc(100,combat::npcs::Relation::allied_to_players,hp,100,allowed),
            seed_npc(101,combat::npcs::Relation::hostile_to_players,500,110),
            seed_npc(102,combat::npcs::Relation::allied_to_players,500,120),
            seed_npc(103,combat::npcs::Relation::allied_to_players,500,300),
            seed_npc(104,combat::npcs::Relation::neutral,500,120)});
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0.f:110.f,0,id==1?out:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);
            w.set_vitals(id,{500,1000,40,40},100);}
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(100),enemy=*w.effect_scope(101);
        const auto plan=*skills::effects::healing_seed(rank);
        auto forged=plan;++forged.healing;check(!w.complete_healing_seed(caster,target,forged,0,100));
        auto stale=target;++stale.generation;check(!w.complete_healing_seed(caster,stale,plan,0,100));
        check(!w.complete_healing_seed(caster,enemy,plan,0,100));
        check(!w.complete_healing_seed(caster,*w.effect_scope(104),plan,0,100));
        check(w.complete_healing_seed(caster,target,plan,0,100));
        const auto cast=out->drain();check(std::count(cast.begin(),cast.end(),*skills::particles::targeted_body_2022(100,1,274))==1);
        // Recast replaces the owned lease, rather than multiplying reactions.
        check(w.complete_healing_seed(caster,target,plan,0,101));out->drain();
        const combat::ai::ordinary::Reviewed weapon{20,20,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},
            "synthetic20damage/no-critical fixture"};
        const auto start=expired?10101u:1000u;
        check(w.configure_npc_opposed_ordinary(enemy,weapon,start));
        const auto token=w.begin_npc_opposed_ordinary(enemy,target,start);check(token.has_value());
        w.tick(token->completes);
        combat::weapon::Strike strike;strike.level=20;strike.minimum=strike.maximum=20;strike.mastery=0;strike.base_roll=strike.miss_roll=.5;strike.location_armor=60;
        const auto damage=combat::weapon::resolve(strike);check(damage.has_value());
        const bool triggers=!expired;const double healing=triggers?plan.healing:0;
        const auto after=w.npc_combat_snapshot(target);check(after&&after->health);
        check(after->health->current==(hp<=damage->damage?0:hp-damage->damage+healing));
        check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==500+healing);
        check(w.npc_combat_snapshot(*w.effect_scope(103))->health->current==500);
        check(w.npc_combat_snapshot(*w.effect_scope(104))->health->current==500);
        check(w.npc_combat_snapshot(enemy)->health->current==500);
        check(w.resources(1)->health==500+healing&&w.resources(2)->health==500+healing&&w.resources(3)->health==500);
        const auto packets=out->drain();
        const auto heal=skills::particles::health_change_2022(102,1,plan.healing,1000);
        check(std::count(packets.begin(),packets.end(),heal)==(triggers?1:0));
        const auto before=w.resources(1)->health;w.tick(token->completes);check(w.resources(1)->health==before);
        ++cases;
    }
    // A recycled caster ID must not inherit the old caster's team or heal credit.
    for(unsigned rank:{0u,12u,20u}){
        world::Instance w;
        w.configure_npcs({seed_npc(100,combat::npcs::Relation::allied_to_players,500,100),
            seed_npc(101,combat::npcs::Relation::hostile_to_players,500,110)});
        for(unsigned id:{1u,2u}){w.join(id,"Ally",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,40,40},100);}
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(100),enemy=*w.effect_scope(101);
        check(w.complete_healing_seed(caster,target,*skills::effects::healing_seed(rank),0,100));
        w.leave(1);w.join(1,"Recycled enemy",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(1,{500,1000,40,40},100);
        check(*w.effect_scope(1)!=caster);
        const combat::ai::ordinary::Reviewed weapon{20,20,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"synthetic source"};
        check(w.configure_npc_opposed_ordinary(enemy,weapon,1000));
        const auto token=w.begin_npc_opposed_ordinary(enemy,target,1000);check(token.has_value());w.tick(token->completes);
        check(w.npc_combat_snapshot(target)->health->current<500);
        check(w.resources(1)->health==500&&w.resources(2)->health==500);++cases;
    }
    check(cases==27);
    std::cout<<"PASS "<<cases<<" NPC Seed World cases: generation/admission, replacement, direct damage, lethal recipient, immunity, range, allegiance, expiry and no duplicate tick\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
