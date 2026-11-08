#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace e=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("shadow NPC publication line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{133u,219u,155u,158u,1078u,102u,1067u}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();world::Instance::Npc n;
  n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={600,1000};d.damageable=true;
  n.combat_definition=d;w.configure_npcs({n});w.join(1,"Source",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);const auto plan=*e::shadow_damage(skill,12);box->drain();
  std::vector<world::Packet> visuals;std::function<void()> publication;
  check(w.shadow_damage(source,target,plan,20,100,&visuals,&publication));check(bool(publication));
  check(box->drain().empty());check(w.npc_combat_snapshot(target)->health->current==600-plan.damage-plan.steal_above_half);
  check(w.resources(1)->health==500+plan.steal_above_half);publication();const auto packets=box->drain();
  const auto hit=combat::detail::integer(10,1,skill);const auto damage=std::find(packets.begin(),packets.end(),hit);check(damage!=packets.end());
  if(const auto body=skills::particles::target_body_2022(100,skill)){
   const auto found=std::find(packets.begin(),packets.end(),*body);check(found!=packets.end()&&found<damage);
   // Preserve the established unary target-body resource. This migration is
   // not evidence for changing shadow effects to the routed GenericValue form.
   const auto routed=skills::particles::targeted_body_2022(100,1,skill);
   check(!routed||std::find(packets.begin(),packets.end(),*routed)==packets.end());
  }
  auto copied=publication;copied();publication();check(box->drain().empty());
  // Exercise the retained immediate-call API too, including its post-damage
  // exact-lifetime guard. Actual replacement during a death callback remains
  // a separate overlap fixture, not proved by this live-target case.
  check(w.shadow_damage(source,target,plan,20,200));const auto legacy=box->drain();
  const auto legacy_hit=std::find(legacy.begin(),legacy.end(),hit);check(legacy_hit!=legacy.end());
  if(const auto body=skills::particles::target_body_2022(100,skill)){
   const auto found=std::find(legacy.begin(),legacy.end(),*body);check(found!=legacy.end()&&legacy_hit<found);
  }
 }
 std::cout<<"PASS shadow NPC damage, conditional steal, established particle routing and one-shot publication\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}
