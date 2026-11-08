#include "gw_world_simulation.h"
#include <stdexcept>
using namespace fw::gw;
void check(bool ok){if(!ok)throw std::runtime_error("Aegis World");}
int main(){
 for(unsigned rank:{0u,12u,20u}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=4;++id){
   w.join(id,"Member",id==1?0:id==3?1100:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,1);
   w.set_vitals(id,{500,1000,30,30},100);
  }
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  check(w.party_action(1,158,3,100)&&w.party_action(3,154,1,100));
  const auto owner=*w.effect_scope(1);const auto plan=*skills::effects::aegis(rank);box->drain();
  check(w.complete_aegis(owner,plan,0,200));
  for(unsigned id:{1u,2u}){const auto active=w.status_snapshot(*w.effect_scope(id),200);
   check(active&&active->size()==1&&active->front().skill==257&&active->front().expires==200+plan.duration);
  }
  check(w.status_snapshot(*w.effect_scope(3),200)->empty());
  check(w.status_snapshot(*w.effect_scope(4),200)->empty()); // Same team, separate party.
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,469))==1);
  check(w.complete_aegis(owner,plan,0,300));check(w.status_snapshot(owner,300)->size()==1);
  w.tick(300+plan.duration);check(w.status_snapshot(owner,300+plan.duration)->empty());
  auto stale=owner;++stale.generation;check(!w.complete_aegis(stale,plan,0,400+plan.duration));
 }
 {
  world::Instance w;
  world::Instance::Npc corpse;corpse.id=corpse.definition.id=100;corpse.x=80;
  corpse.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition definition;
  definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::hostile_to_players;
  definition.spawn_health={100,100};definition.damageable=true;
  definition.fleshy=true;definition.fleshy_evidence="test authored";
  corpse.combat_definition=definition;w.configure_npcs({corpse});
  auto box=std::make_shared<world::Outbox>();
  w.join(2,"Profane source",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,7);
  w.join(3,"Aegis caster inside",100,0,box,0,{}, {},100,8);
  w.join(4,"Aegis ally outside",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,8);
  for(unsigned id:{2u,3u,4u})w.set_vitals(id,{100,100,20,100},100);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  const auto source=*w.effect_scope(2),inside=*w.effect_scope(3),outside=*w.effect_scope(4);
  check(w.damage_npc(source,*w.effect_scope(100),
      {combat::Kind::armor_ignoring,1000,20,194},1248,101).killed);
  check(w.claim_well_corpse(source,*skills::effects::well_cast_plan(94,12),1,102,252,
      [](const auto&){return true;}));
  const auto plan=*skills::effects::aegis(12);box->drain();
  check(w.complete_aegis(inside,plan,0,103));
  check(w.status_snapshot(inside,103)->empty());
  const auto ally_status=w.status_snapshot(outside,103);
  check(ally_status&&ally_status->size()==1&&ally_status->front().skill==257);
  const auto packets=box->drain();
  check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,3,469))==1);
  check(w.complete_aegis(inside,plan,0,100000));
  check(w.status_snapshot(inside,100000)->size()==1);
  check(w.status_snapshot(outside,100000)->size()==1);
 }
}
