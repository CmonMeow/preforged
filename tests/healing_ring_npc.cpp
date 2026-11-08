#include "gw_healing_ring_npc.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool x){if(!x)throw std::runtime_error("Healing Ring NPC proposal");}
int main()try{using namespace skills::effects;using namespace combat::npcs;
 const Scope caster{1,42,7};
 for(unsigned rank:{0u,12u,20u})for(auto relation:{Relation::neutral,Relation::protected_ally,Relation::hostile_to_players}){
  Definition d;d.binding.map=146;d.binding.actor=100;d.relation=relation;d.spawn_health=combat::Health{100,500};d.damageable=relation!=Relation::protected_ally;d.conditions_allowed=d.damageable;d.healing_allowed=true;d.healing_evidence="test";
  LiveActor actor(d,{1,2,3,146,100});const auto life=actor.lifetime();const auto plan=*healing_ring(rank,12);RingRecipient recipient{actor.hex_scope(),plan.healing};
  const auto proposal=prepare_healing_ring_npc(plan,caster,recipient,RingCreature::creature,actor,life,100);
  check(proposal&&proposal->actor(life)->health->current==100+plan.healing&&actor.actor(life)->health->current==100);
  check(!prepare_healing_ring_npc(plan,caster,recipient,RingCreature::spirit,actor,life,100));
  check(!prepare_healing_ring_npc(plan,caster,recipient,RingCreature::unknown,actor,life,100));
  auto stale=life;++stale.actor_generation;check(!prepare_healing_ring_npc(plan,caster,recipient,RingCreature::creature,actor,stale,100));
  auto wrong=recipient;++wrong.healing;check(!prepare_healing_ring_npc(plan,caster,wrong,RingCreature::creature,actor,life,100));
  if(d.conditions_allowed){check(actor.apply_condition(life,combat::conditions::Kind::deep_wound,42,100,5000));
  const auto wounded=prepare_healing_ring_npc(plan,caster,recipient,RingCreature::creature,actor,life,100);
  check(wounded&&wounded->actor(life)->health->current<=400&&wounded->actor(life)->health->current<proposal->actor(life)->health->current);}
  auto immune=d;immune.healing_allowed=false;LiveActor spirit(immune,life);
  check(!prepare_healing_ring_npc(plan,caster,recipient,RingCreature::creature,spirit,life,100));
  auto unknown=d;unknown.healing_allowed.reset();unknown.healing_evidence="-";LiveActor unreviewed(unknown,life);
  check(!prepare_healing_ring_npc(plan,caster,recipient,RingCreature::creature,unreviewed,life,100));
  auto retired=life;++retired.actor_generation;check(!actor.healing_allowed(retired).has_value());
  auto capped=d;capped.spawn_health=combat::Health{499,500};LiveActor full(capped,life);
  const auto cap=prepare_healing_ring_npc(plan,caster,recipient,RingCreature::creature,full,life,100);check(cap&&cap->actor(life)->health->current==500);
 }
 std::cout<<"PASS Healing Ring staged NPC heal, all relations, immutable source, spirit/unknown/stale/forged rejection, Deep Wound and cap; World integration pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
