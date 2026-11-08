#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Simple enchantment deadline line "+std::to_string(at.line()));
}
int main()try{
 for(bool npc_target:{false,true}){
  auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.definition.visual_adjustment={0,0,0,100};n.x=60;
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={500,500};d.damageable=true;n.combat_definition=d;w->configure_npcs({n});
  w->join(1,"Caster",0,0,box,0,{}, {},100,1);w->join(2,"Other",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w->set_vitals(1,{500,500,50,50},100);w->set_vitals(2,{500,500,50,50},100);
  const auto source=*w->effect_scope(1),other=*w->effect_scope(2),target=*w->effect_scope(npc_target?100:1);
  const auto short_plan=*skills::effects::shield_deflection(0),long_plan=*skills::effects::shield_deflection(15);
  check(npc_target?w->stage_npc_shield_deflection(target,short_plan,500):w->complete_shield_deflection(source,target,short_plan,0,500));
  check(w->complete_shield_deflection(source,other,long_plan,0,500));
  check(w->status_mutation(other,258,9000,0,500,{},true)); // Unbounded unrelated enchantment.
  const auto active=w->status_snapshot(target,500);check(bool(active));
  const auto entry=std::find_if(active->begin(),active->end(),[](const auto& e){return e.skill==259;});check(entry!=active->end()&&entry->expires);
  const auto end=*entry->expires;const auto removed=skills::casting::effect_remove(target.actor,entry->instance);
  box->drain();w->tick(end-1);auto packets=box->drain();check(std::count(packets.begin(),packets.end(),removed)==0);
  w->tick(end);packets=box->drain();check(std::count(packets.begin(),packets.end(),removed)==1);
  w->tick(end);w->tick(end+100);packets=box->drain();check(std::count(packets.begin(),packets.end(),removed)==0);
  const auto remaining=w->status_snapshot(other,end+100);check(bool(remaining));
  check(std::any_of(remaining->begin(),remaining->end(),[](const auto& e){return e.skill==259;}));
  check(std::any_of(remaining->begin(),remaining->end(),[](const auto& e){return e.instance==9000&&!e.expires;}));
  check(w->resources(1)->health==500&&w->resources(2)->health==500&&w->npc_combat_snapshot(*w->effect_scope(100))->health->current==500);
 }
 std::cout<<"PASS exact player/NPC enchantment expiry despite due-1 tick; replay and unrelated future/unbounded effects preserved\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
