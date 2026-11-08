#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Ward177 other NPC routes line "+std::to_string(at.line()));
}
world::Instance::Npc fixture_npc(bool hostile){
    world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;
    n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
    d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::neutral;
    d.spawn_health=combat::Health{1000,1000};d.spawn_energy=combat::npcs::Energy{100,100};
    d.damageable=d.conditions_allowed=true;d.creature_type="human";d.creature_type_evidence="authored fixture";
    n.combat_definition=d;return n;
}
int main()try{
    const auto plan=*skills::effects::ward_foes_plan(0);
    {
        world::Instance w;const auto guardian=fixture_npc(false);w.configure_npcs({guardian});
        w.join(2,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(2,{1000,1000,100,100},100);
        const auto caster=*w.effect_scope(2),guard=*w.effect_scope(100);
        auto enemy=fixture_npc(true);enemy.x=400;
        check(w.begin_wave_objective(caster,guard,73,1,3,1));
        check(w.spawn_quest_wave(caster,73,1,{enemy},[]{return true;}));
        const auto foe=*w.effect_scope(60000);
        check(w.apply_ward_foes(caster,plan,100));check(w.chase_quest_guardian(foe,50,100));
        const auto path=w.maintained_range_path(foe,100,9100);
        check(path&&!path->empty()&&std::abs(path->back().x-111)<.01);
        // Include the exact field-entry and expiry boundaries so maintained
        // range checks do not interpolate a chord across a speed change.
        check(std::any_of(path->begin(),path->end(),[](const auto& p){return p.at==1660&&std::abs(p.x-322)<.01;}));
        check(std::any_of(path->begin(),path->end(),[](const auto& p){return p.at==8100&&std::abs(p.x-161)<.01;}));
        w.tick(9100);
        check(!w.projectile_target_valid(caster,60000,110,9100));
        check(w.projectile_target_valid(caster,60000,112,9100));
        check(!w.maintained_range_path(foe,100,9100)); // The route has consumed that historical interval.
    }
    {
        world::Instance w;w.configure_npcs({fixture_npc(true)});
        w.join(1,"Ward caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        const auto caster=*w.effect_scope(1),foe=*w.effect_scope(100);
        check(w.apply_ward_foes(caster,plan,100));check(w.kill(caster,100));
        w.join(2,"Pursuit target",1490,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(2,{1000,1000,100,100},100);
        const auto target=*w.effect_scope(2);
        combat::ai::FlarePolicy policy;policy.source=foe;policy.level=20;policy.fire_magic=12;
        policy.speed=50;policy.acquire=2000;policy.follow=2500;policy.leash=2500;
        policy.flight={100,32};policy.evidence="authored Ward177 pursuit fixture";
        check(w.configure_npc_flare_ai(policy,100));w.tick(100);w.tick(9100);
        // Eight seconds at 25/s, followed by one at 50/s after expiry.
        check(!w.projectile_target_valid(target,100,1239,9100));
        check(w.projectile_target_valid(target,100,1241,9100));
    }
    std::cout<<"Ward177 staged NPC trajectory and Flare sparse expiry passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
