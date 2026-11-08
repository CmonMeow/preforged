#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>

using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Mend Body and Soul World line "+std::to_string(at.line()));
}

static world::Instance::Npc spirit(unsigned id,float x,bool typed=true){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.y=0;
    n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition body;
    body.binding.map=146;body.binding.actor=body.binding.definition=id;
    body.binding.level=20;body.relation=combat::npcs::Relation::allied_to_players;
    body.spawn_health=combat::Health{40,40};body.damageable=true;
    if(typed){body.creature_type="spirit";body.creature_type_evidence="fixture";}
    n.combat_definition=body;return n;
}
static world::Instance::Npc human(unsigned id,bool typed=true){
    auto n=spirit(id,40,typed);
    n.combat_definition->spawn_health=combat::Health{20,100};
    n.combat_definition->healing_allowed=true;
    n.combat_definition->healing_evidence="fixture";
    if(typed)n.combat_definition->creature_type="human";
    return n;
}

int main()try{
    const auto plan=*skills::effects::mend_body_soul(0);
    for(unsigned mode=0;mode<3;++mode){
        world::Instance w;
        if(mode==1)w.configure_npcs({spirit(100,0),spirit(101,100)});
        if(mode==2)w.configure_npcs({spirit(100,0,false)});
        auto caster_box=std::make_shared<world::Outbox>();
        auto ally_box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,caster_box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,ally_box,0,{}, {},100,1);
        w.set_vitals(1,{100,100,50,50},100);
        w.set_vitals(2,{20,100,50,50},100);
        const auto caster=*w.effect_scope(1),ally=*w.effect_scope(2);
        using combat::conditions::Kind;
        check(w.apply_condition(caster,ally,Kind::bleeding,100,10000));
        check(w.apply_condition(caster,ally,Kind::poison,101,10000));
        caster_box->drain();ally_box->drain();
        auto stale=ally;++stale.generation;
        check(!w.stage_mend_body_soul_player(caster,stale,plan,200));
        check(w.resources(2)->health==20);
        std::function<void()> publish;
        check(w.stage_mend_body_soul_player(caster,ally,plan,200,&publish)==(mode!=2));
        check(mode==2?w.resources(2)->health<=20:
                      w.resources(2)->health>=39&&w.resources(2)->health<=40);
        check(w.has_condition(ally,Kind::bleeding,200)==(mode!=1));
        check(w.has_condition(ally,Kind::poison,200)==(mode!=1));
        const auto prior_caster=caster_box->drain(),prior_ally=ally_box->drain();
        const auto premature=[](const auto& packet){return !packet.empty()&&packet[0]==67;};
        check(std::none_of(prior_caster.begin(),prior_caster.end(),premature));
        check(std::none_of(prior_ally.begin(),prior_ally.end(),premature));
        if(mode!=2){
            check(bool(publish));publish();
            const auto packets=ally_box->drain();
            check(std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(2,1,20,100))!=packets.end());
            check(std::count_if(packets.begin(),packets.end(),[](const auto& packet){
                return !packet.empty()&&packet[0]==67;
            })==(mode==1?2:0));
        }
    }
    for(unsigned mode=0;mode<3;++mode){
        world::Instance w;
        auto target_npc=human(100,mode!=1);
        if(mode==2)target_npc.combat_definition->relation=
            combat::npcs::Relation::hostile_to_players;
        w.configure_npcs({target_npc,spirit(101,80)});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{100,100,50,50},100);
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(100);
        box->drain();
        auto stale=target;++stale.generation;
        check(!w.stage_mend_body_soul_npc(caster,stale,plan,200));
        std::function<void()> publish;
        check(w.stage_mend_body_soul_npc(caster,target,plan,200,&publish)==(mode==0));
        check(w.npc_combat_snapshot(target)->health->current==(mode==0?40:20));
        const auto early=box->drain();
        check(std::find(early.begin(),early.end(),
            skills::particles::health_change_2022(100,1,20,100))==early.end());
        if(mode==0){
            check(bool(publish));publish();
            const auto packets=box->drain();
            check(std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(100,1,20,100))!=packets.end());
        }
    }
    std::cout<<"PASS Mend Body and Soul player heal/cleanse and typed NPC heal staging; no normal cast binding\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
