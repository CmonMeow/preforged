#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
using namespace skills::effects;

static void check(bool ok, std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Selfless NPC payment line "+std::to_string(at.line()));
}

struct Fixture {
    world::Instance w;
    Scope owner,ally,hostile,stale;
    explicit Fixture(unsigned alias){
        std::vector<world::Instance::Npc> population;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc npc;
            npc.id=npc.definition.id=id;
            npc.definition.level=4;
            npc.definition.visual_adjustment={0,0,0,100};
            npc.x=float(id);
            combat::npcs::Definition definition;
            definition.binding.map=146;
            definition.binding.actor=definition.binding.definition=id;
            definition.binding.level=4;
            definition.relation=id==100?combat::npcs::Relation::allied_to_players
                                       :combat::npcs::Relation::hostile_to_players;
            definition.spawn_health=combat::Health{200,200};
            definition.damageable=true;
            definition.healing_allowed=id==100;
            definition.healing_evidence="synthetic Selfless semantic target fixture";
            npc.combat_definition=definition;
            population.push_back(npc);
        }
        // Population replacement is only permitted before any player joins.
        // Retain an actual retired NPC lifetime, rather than inventing a scope.
        w.configure_npcs(population);
        stale=*w.effect_scope(100);
        w.configure_npcs(population);
        ally=*w.effect_scope(100);
        hostile=*w.effect_scope(101);
        check(stale.actor==ally.actor&&stale!=ally);
        w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        owner=*w.effect_scope(1);
        check(w.complete_selfless_spirit(owner,*selfless_spirit(alias),0,1000));
    }
};

int main()try{
    for(unsigned alias:{1952u,2095u})for(unsigned route=0;route<3;++route){
        for(unsigned target_kind=0;target_kind<3;++target_kind){
            Fixture f(alias);
            const auto target=target_kind==0?f.ally:target_kind==1?f.hostile:f.stale;
            const bool valid=target_kind!=2;
            const unsigned reduction=target_kind==0?3:0;
            if(route==0){
                check(f.w.consume_selfless_energy(f.owner,{194,0},target,10,1100)==valid);
                check(f.w.resources(1)->energy==(valid?90+reduction:100));
            }else if(route==1){
                check(f.w.activate_selfless_overcast_energy(f.owner,219,target,10,1,1100)==valid);
                check(f.w.resources(1)->energy==(valid?90+reduction:100));
                if(valid)check(f.w.overcast_energy_snapshot(f.owner)->curve.burden_units()==500000);
            }else{
                check(f.w.apply_elemental_glyph(f.owner,50,*elemental_glyph(198),1001));
                const auto state=f.w.elemental_glyph_snapshot(f.owner,1100);
                check(bool(state));
                const auto paid=f.w.pay_selfless_elemental_glyph_use(f.owner,{194,0},target,20,1,1100,state);
                check(bool(paid)==valid);
                check(f.w.resources(1)->energy==(valid?80+reduction:100));
                if(valid){
                    check(paid->energy_cost==20-reduction);
                    check(f.w.settle_elemental_glyph_use(f.owner,1,GlyphOutcome::completed,1200).accepted);
                }else{
                    // Rejection must leave both the energy and glyph lease available.
                    const auto retry=f.w.pay_selfless_elemental_glyph_use(f.owner,{194,0},f.ally,20,1,1100,state);
                    check(retry&&retry->energy_cost==17);
                    check(f.w.resources(1)->energy==83);
                }
            }
        }
    }
    std::cout<<"PASS Selfless NPC allied/hostile/retired lifetime ordinary/Overcast/glyph payments for both aliases\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
