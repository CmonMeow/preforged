#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Condition recipient retirement line "+std::to_string(at.line()));
}
world::Instance::Npc make_body(unsigned id,float x,double health){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding={146,id,id,0,0,0,0,20};d.relation=combat::npcs::Relation::hostile_to_players;
 d.spawn_health=combat::Health{health,100};d.damageable=d.conditions_allowed=true;d.fire_armor=0;
 d.fleshy=true;d.fleshy_evidence="synthetic condition lifecycle fixture";n.combat_definition=d;return n;
}
int main()try{
 for(unsigned index=0;index<10;++index){
 world::Instance w;auto box=std::make_shared<world::Outbox>();
 w.join(1,"Condition caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
 const auto caster=*w.effect_scope(1);
 check(w.spawn_kill_encounter(caster,81,1,{make_body(101,100,1),make_body(102,500,100)},[]{return true;},false,{},{},false,0,true));
 const auto dying=*w.effect_scope(60000),selected=*w.effect_scope(60001);
 check(w.fire_storm(caster,dying,*skills::effects::fire_storm(0),20,100));
 box->drain();
 const auto kind=static_cast<combat::conditions::Kind>(index);
 // Completing kill-any changes objective/controller eligibility, not the
 // surviving body's lifetime. This remains a valid condition recipient.
 check(w.apply_condition(caster,selected,kind,1100,5000));
 check(w.npc_combat_snapshot(dying)->health->current==0);
 check(w.effect_scope(selected.actor)==selected&&w.has_condition(selected,kind,1100));
 check(w.clear_kill_objective(caster,81,1,true));
 check(!w.effect_scope(selected.actor)&&!w.npc_combat_snapshot(selected));
 check(!w.apply_condition(caster,selected,kind,1100,5000));
 check(!w.has_condition(selected,kind,1100));
 }
 std::cout<<"10 conditions: due quest completion preserves live survivor; actual retirement rejects stale target\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
