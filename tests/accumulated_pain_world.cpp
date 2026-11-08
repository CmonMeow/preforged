#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Accumulated Pain World line "+std::to_string(at.line()));
}
int main()try{
 world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
 n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
 d.binding.map=146;d.binding.actor=d.binding.definition=100;
 d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{480,480};
 d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
 w.join(1,"Wound caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
 w.set_vitals(1,{500,1000,50,100},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
 check(w.degeneration_hex(source,target,*skills::effects::degeneration_hex(129,0),100));
 check(w.degeneration_hex(source,target,*skills::effects::degeneration_hex(785,0),100));
 check(w.wound_spell(source,target,*skills::effects::wound_spell(1052,12),20,100));
 const auto actor=w.npc_combat_snapshot(target);check(actor&&actor->health);
 check(actor->health->current==321&&actor->health->maximum==384);
 std::cout<<"Accumulated Pain damages a two-hex NPC then applies Deep Wound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
