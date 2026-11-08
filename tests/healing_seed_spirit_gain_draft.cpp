// DETACHED common consumer regression: Healing Seed explicitly grants Health
// to spirits (local wiki Healing_Seed Notes), independently of healing immunity.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Healing Seed spirit gain draft line "+std::to_string(at.line()));
}
static world::Instance::Npc body(unsigned id,bool hostile,float x,bool spirit=false){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
 d.spawn_health={500,1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;
 d.damageable=true;d.physical_armor=60;d.conditions_allowed=!spirit;
 d.creature_type=spirit?"spirit":"human";d.creature_type_evidence="authored Seed spirit gain fixture";
 d.healing_allowed=!spirit;d.healing_evidence="spirit cannot receive healing; Seed grants Health";
 n.combat_definition=d;return n;
}
int main()try{
 auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
 w->configure_npcs({body(100,false,100),body(101,true,110),body(102,false,120,true),body(103,false,500,true)});
 w->join(1,"Seed caster",100,0,box,0,{}, {},100,1);w->set_vitals(1,{500,1000,100,100},100);
 const auto target=*w->effect_scope(100),foe=*w->effect_scope(101),spirit=*w->effect_scope(102),far=*w->effect_scope(103),caster=*w->effect_scope(1);
 const auto seed=*healing_seed(12);check(w->complete_healing_seed(caster,target,seed,0,100));
 const combat::ai::ordinary::Reviewed weapon{20,20,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"authored20damage Seed gain"};
 check(w->configure_npc_opposed_ordinary(foe,weapon,1000));const auto token=w->begin_npc_opposed_ordinary(foe,target,1000);check(bool(token));box->drain();w->tick(token->completes);
 check(w->npc_combat_snapshot(target)->health->current==480+seed.healing);
 check(w->resources(1)->health==500+seed.healing);
 check(w->npc_combat_snapshot(spirit)->health->current==500+seed.healing);
 check(w->npc_combat_snapshot(far)->health->current==500);
 const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(102,1,seed.healing,1000))==1);
 std::cout<<"PASS detached Healing Seed real-hit nearby spirit Health gain, healing immunity and range\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
