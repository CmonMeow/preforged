#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Grawl melee World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope owner,source;world::Instance::Npc sample;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(bool arm=true){
        sample.id=sample.definition.id=100;sample.definition.level=1;sample.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=160;d.binding.actor=d.binding.definition=100;d.binding.level=1;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{80,80};
        d.damageable=true;d.earth_armor=0;d.physical_armor=0;d.conditions_allowed=true;sample.combat_definition=d;
        join();check(w.spawn_kill_encounter(owner,75,1,{sample},[]{return true;}));source=*w.effect_scope(60000);
        if(arm){check(w.arm_grawl_melee(owner,1,1000));check(w.arm_grawl_melee(owner,1,1000));}
        check(!w.npc_energy_snapshot(source));
    }
    void join(){w.join(1,"Quest owner",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
        owner=*w.effect_scope(1);check(w.physical_armor(owner,0));}
    void hurt(std::uint64_t now){check(w.damage_npc(owner,source,{combat::Kind::armor_ignoring,1,20,194},1248,now).reason==combat::Reason::accepted);}
};
int main()try{
    {Fixture f;f.w.tick(1000);f.hurt(1100);
        check(f.w.grenths_balance(f.owner,f.source,1101));
        check(f.w.npc_combat_snapshot(f.source)->health->current==80);
        f.w.tick(1101);check(f.w.npc_ordinary_pending(f.source).has_value());}
    {Fixture f;f.hurt(1100);f.w.tick(1100);f.w.move(1,3000,0,1101);
        // move() authors a real 288-unit/s player route, not a teleport.
        // Allow the outbound route and the NPC's own return to finish.
        for(unsigned at=1200;at<=40000;at+=100)f.w.tick(at);
        check(std::abs(f.w.position(1,40000).first-3000)<.01f);
        check(!f.w.npc_flare_ai_snapshot(f.source)->engaged&&!f.w.npc_flare_ai_snapshot(f.source)->returning);
        f.w.move(1,100,0,41000);
        for(unsigned at=41000;at<=53000;at+=100)f.w.tick(at);
        check(std::abs(f.w.position(1,53000).first-100)<.01f);
        check(!f.w.npc_flare_ai_snapshot(f.source)->engaged&&!f.w.npc_ordinary_pending(f.source));
        f.hurt(54000);f.w.tick(54000);check(f.w.npc_ordinary_pending(f.source).has_value());}
    {Fixture f;f.w.tick(1000);f.w.tick(1050);check(!f.w.npc_ordinary_pending(f.source));
        check(!f.w.npc_flare_ai_snapshot(f.source)->engaged); //no proximity aggro
        f.hurt(1100);f.w.tick(1100);check(f.w.npc_ordinary_pending(f.source).has_value());
        check(f.w.resources(1)->health==1000);f.w.tick(1849);check(f.w.resources(1)->health==1000);
        check(f.w.npc_ordinary_pending(f.source)->token.completes==1850);
        // The1849 poll schedules World's next100ms update at1949. The
        // nominal1850 windup endpoint cannot bypass that scheduler gate.
        f.w.tick(1949);check(f.w.resources(1)->health<1000);check(!f.w.npc_ordinary_pending(f.source));
        const auto outcomes=f.w.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::hit);
        const auto health=f.w.resources(1)->health;f.w.tick(3000);check(f.w.resources(1)->health==health);
        check(!f.w.arm_grawl_melee(f.owner,2,3000));}
    {Fixture f;f.hurt(1100);f.w.tick(1100);check(f.w.npc_ordinary_pending(f.source).has_value());
        check(f.w.damage_npc(f.owner,f.source,{combat::Kind::armor_ignoring,100,20,194},1248,1200).killed);
        f.w.tick(2000);check(f.w.resources(1)->health==1000);check(!f.w.npc_ordinary_pending(f.source));}
    {Fixture f;f.hurt(1100);f.w.tick(1100);check(f.w.clear_kill_objective(f.owner,75,1,true));
        f.w.tick(2000);check(!f.w.effect_scope(f.source.actor));check(f.w.resources(1)->health==1000);
        check(!f.w.npc_flare_ai_snapshot(f.source));
        check(f.w.spawn_kill_encounter(f.owner,75,2,{f.sample},[]{return true;}));
        const auto fresh=*f.w.effect_scope(60001);check(fresh!=f.source);check(f.w.arm_grawl_melee(f.owner,2,2100));
        f.w.tick(2100);check(!f.w.npc_ordinary_pending(fresh));}
    {Fixture f;f.hurt(1100);f.w.tick(1100);const auto old=f.owner;f.w.leave(1);f.join();
        check(f.owner!=old);f.w.tick(2000);check(f.w.resources(1)->health==1000);check(!f.w.npc_flare_ai_snapshot(f.source));
        check(!f.w.arm_grawl_melee(old,1,2000));
        check(f.w.spawn_kill_encounter(f.owner,75,1,{f.sample},[]{return true;}));
        check(f.w.arm_grawl_melee(f.owner,1,2100));f.w.tick(2100);check(f.w.resources(1)->health==1000);}
    {Fixture f(false);combat::weapon::Owned sword{3,1,{1,1,2},0,false,false,27};
        check(f.w.ordinary_weapon(f.owner,sword,20));check(f.w.melee_target(f.owner,f.source.actor,1000));
        check(f.w.melee_attack(f.owner,f.source,sword,*combat::weapon::attack_skill(382,0),1000));
        check(f.w.melee_attack(f.owner,f.source,sword,*combat::weapon::attack_skill(384,0),1001));
        check(f.w.has_condition(f.source,combat::conditions::Kind::deep_wound,1001));
        check(f.w.npc_combat_snapshot(f.source)->health->maximum==64);
        check(f.w.arm_grawl_melee(f.owner,1,1100));f.w.tick(1100);
        check(f.w.npc_ordinary_pending(f.source).has_value());}
    std::cout<<"Grawl owner melee activation and lifecycle passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
