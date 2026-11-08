// DETACHED, unregistered: acceptance regression for retained AI motion intent.
// Expected to expose today's missing AI history; not a passing checkpoint.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("AI death frontier line "+std::to_string(at.line()));
}
world::Instance::Npc body(unsigned id,float x,unsigned hp){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=1;
    n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding={145,id,id,0,0,0,0,1};
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(hp),hp};
    d.damageable=d.conditions_allowed=true;
    if(id==100){d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;}
    n.combat_definition=d;return n;
}
struct Result {float x,y;unsigned near_health,far_health;};
Result run(bool elemental,bool dense){
    world::Instance w;w.configure_npcs({body(100,0,2),body(101,300,200),body(102,-200,200)});
    w.join(1,"Static target",1400,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.set_vitals(1,{1000,1000,100,100},100);
    const auto owner=*w.effect_scope(1),source=*w.effect_scope(100);
    check(w.physical_armor(owner,0)&&w.elemental_armor(owner,0,0));
    combat::ai::FlarePolicy policy;policy.source=source;policy.target=owner;policy.level=1;
    policy.speed=100;policy.acquire=1500;policy.follow=1800;policy.leash=2000;
    policy.flight={1000,32};policy.evidence="synthetic authored static-target timeline fixture";
    if(elemental){
        // The target starts beyond the canonical1248 range and stays beyond
        // it until death. No invented skill range or ordinary attack stats.
        check(w.configure_npc_elemental_ai(policy,{{{214,0},*skills::effects::water_projectile(214,0),policy.evidence}},4,100));
        check(w.observe_npc_overcast(source,0,100,policy.evidence));
    }else check(w.configure_npc_immediate_ai(policy,{{{155,0},*skills::effects::shadow_damage(155,0),policy.evidence}},100));
    // Observe target and choose chase at100 in BOTH schedules; no future target
    // positions are used to justify this initial forward intent.
    w.tick(100);
    const auto initial=w.npc_pursuit_movement_snapshot(source);
    check(initial&&initial->at==100&&initial->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
    check(elemental?w.npc_elemental_ai_snapshot(source)->engaged:w.npc_flare_ai_snapshot(source)->engaged);
    check(elemental?!w.npc_elemental_ai_snapshot(source)->pending:!w.npc_flare_ai_snapshot(source)->cast);
    const auto retained=[&]{
        check(elemental?w.npc_elemental_ai_snapshot(source)->target==owner:w.npc_flare_ai_snapshot(source)->policy.target==owner);
        check(elemental?!w.npc_elemental_ai_snapshot(source)->pending:!w.npc_flare_ai_snapshot(source)->cast);
    };
    retained();
    // Join the legal-range hex caster only after the original chase decision.
    // Existing engagement must retain player1 rather than switch to this peer.
    w.join(2,"Bile caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.set_vitals(2,{1000,1000,100,100},100);const auto caster=*w.effect_scope(2);
    check(w.degeneration_hex(caster,source,*skills::effects::degeneration_hex(2058,0),100,20));
    retained();
    if(dense)for(unsigned at=200;at<=5000;at+=100){w.tick(at);if(at<1100)retained();}
    w.tick(5100);
    const auto deaths=w.take_npc_deaths(caster);
    const auto death=std::find_if(deaths.begin(),deaths.end(),[&](const auto& entry){return entry.target==source;});
    check(death!=deaths.end()&&death->time==1100&&w.pending_putrid_deaths()==0);
    Result result{death->position.x,death->position.y,
        unsigned(w.npc_combat_snapshot(*w.effect_scope(101))->health->current),
        unsigned(w.npc_combat_snapshot(*w.effect_scope(102))->health->current)};
    std::cout<<(elemental?"elemental":"immediate")<<" dense="<<dense<<" death=("<<result.x<<","<<result.y
        <<") recipients="<<result.near_health<<","<<result.far_health<<'\n';
    w.tick(6100);check(w.take_npc_deaths(caster).empty());check(w.take_npc_deaths(owner).empty());return result;
}
int main()try{
    bool passed=true;
    for(bool elemental:{false,true}){
        const auto coarse=run(elemental,false),fine=run(elemental,true);
        //100units/s from the explicit100ms decision until death1100ms.
        for(const auto result:{coarse,fine})passed=passed&&std::abs(result.x-100.f)<.01f&&std::abs(result.y)<.01f&&
            result.near_health==175&&result.far_health==200;
        passed=passed&&std::hypot(coarse.x-fine.x,coarse.y-fine.y)<.01f;
    }
    check(passed);std::cout<<"AI retained chase intent has coarse/fine death-origin agreement\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
