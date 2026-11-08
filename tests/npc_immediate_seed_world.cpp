// Detached: native non-spell effect/protection/Seed and publication ordering.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC immediate Seed line "+std::to_string(at.line()));
}
int main()try{
    const auto seed=*skills::effects::healing_seed(12);
    for(int allowed:{-1,0,1})for(bool protected_target:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();std::vector<world::Instance::Npc> population;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=2;n.definition.flags=4;
            n.x=id==100?0:120;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding={145,id,id,0,0,0,4,2};d.spawn_health=combat::Health{500,1000};d.damageable=true;
            d.relation=id==100?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
            if(id==100){d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;}
            else{d.healing_allowed=allowed<0?std::optional<bool>{}:std::optional(bool(allowed));
                if(allowed>=0)d.healing_evidence="synthetic Seed eligibility";}
            n.combat_definition=d;population.push_back(n);
        }
        w.configure_npcs(population);
        for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?100:140,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,1);
            w.set_vitals(id,{500,1000,40,40},100);}
        const auto source=*w.effect_scope(100),target=*w.effect_scope(1),monk=*w.effect_scope(2),ally=*w.effect_scope(101);
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        check(w.complete_healing_seed(monk,target,seed,0,100));
        if(protected_target)check(w.status_mutation(target,1399,800,12,100,6100,true));
        combat::ai::FlarePolicy policy;policy.source=source;policy.level=2;policy.speed=220;policy.acquire=200;policy.follow=1500;
        policy.leash=1000;policy.flight={1000,32};policy.evidence="synthetic NPC155 protection/Seed fixture";
        check(w.configure_npc_immediate_ai(policy,{{{155,0},*skills::effects::shadow_damage(155,0),policy.evidence}},1000));
        box->drain();w.tick(1000);const auto token=w.npc_flare_ai_snapshot(source)->cast;check(token.has_value());
        check(w.npc_energy_snapshot(source)->current==30);box->drain();w.tick(token->completes);
        const auto packets=box->drain();const auto finish=skills::casting::terminal(58,100),damage=combat::detail::integer(10,100,155);
        const auto* row=skills::find_skill(155);check(row&&row->caster_body_resource==279);
        world::Packet body;world::u16(body,158);world::u32(body,21);world::u32(body,100);world::u32(body,279);
        const auto heal=skills::particles::health_change_2022(1,2,seed.healing,1000);
        const auto at=[&](const auto& p){return std::find(packets.begin(),packets.end(),p);};
        if(allowed<0){
            check(w.npc_flare_ai_snapshot(source)->cast==token);
            check(w.resources(1)->health==500&&w.resources(2)->health==500&&w.npc_combat_snapshot(ally)->health->current==500);
            check(at(finish)==packets.end()&&at(body)==packets.end()&&at(damage)==packets.end());
            w.tick(token->completes+100);check(w.npc_energy_snapshot(source)->current==30);
            check(w.resources(1)->health==500&&w.npc_flare_ai_snapshot(source)->cast==token);
        }else{
            const unsigned loss=protected_target?15:20;
            check(w.resources(1)->health==500-loss+seed.healing&&w.resources(2)->health==500+seed.healing);
            check(w.npc_combat_snapshot(ally)->health->current==500+(allowed?seed.healing:0));
            check(!w.npc_flare_ai_snapshot(source)->cast&&w.npc_energy_snapshot(source)->current==30);
            check(at(finish)!=packets.end()&&at(body)!=packets.end()&&at(damage)!=packets.end()&&at(heal)!=packets.end());
            check(at(finish)<at(body)&&at(body)<at(damage)&&at(damage)<at(heal));
            check(std::count(packets.begin(),packets.end(),finish)==1&&std::count(packets.begin(),packets.end(),body)==1);
            w.tick(token->completes);check(box->drain().empty());
        }
    }
    std::cout<<"NPC155 Seed eligibility, protection, paid deferral and terminal/body/damage/heal order\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
