#pragma once
#include "gw_world_simulation.h"
#include <stdexcept>
namespace warrior_test {
using namespace fw::gw;
struct Fixture {
 world::Instance world;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 skills::effects::Scope source,target;
 combat::weapon::Owned weapon;
 Fixture(unsigned skill,bool npc,unsigned health=1000,std::optional<bool> susceptible=true,bool conditions=true,unsigned fire_multiplier=1,std::optional<combat::npcs::Energy> energy={},std::optional<bool> flesh=true){
  world.configure_attack_miss_draws([]{return .5;});
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),1000};
   if(energy){d.spawn_energy=energy;d.energy_pips=0;d.energy_evidence="test";d.energy_pips_evidence="test";}d.damageable=true;d.conditions_allowed=conditions;d.fire_multiplier=fire_multiplier;d.fire_multiplier_evidence="test";d.physical_armor=60;d.physical_armor_evidence="test";
   d.fleshy=flesh;if(flesh)d.fleshy_evidence="synthetic attack fixture body trait";
   d.knockdown_allowed=susceptible;d.knockdown_evidence=susceptible.has_value()?"test":"";n.combat_definition=d;world.configure_npcs({n});}
  world.join(1,"Warrior",0,0,box,0,{}, {},100,1);world.set_vitals(1,{1000,1000,30,30},100);
  if(!npc){world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);world.set_vitals(2,{health,1000,30,30},100);}
  source=*world.effect_scope(1);target=*world.effect_scope(npc?100:2);
  if(!world.energy_capacity(1,1,0,100,{10,-2}))throw std::runtime_error("energy fixture");
  if(!npc)world.physical_armor(target,60);
  weapon={12,1,{20,20,2},12,false,false,combat::weapon::attack_skill(skill,12)->required_type};
  if(!world.ordinary_weapon(source,weapon,20))throw std::runtime_error("weapon fixture");
 }
 double health()const{return target.actor==100?world.npc_combat_snapshot(target)->health->current:double(world.resources(target.actor)->health);}
 bool condition(combat::conditions::Kind kind,std::uint64_t at,std::uint64_t duration){
  if(target.actor!=100)return world.apply_condition(source,target,kind,at,duration);
  // Use the existing owned transfer path: generic apply_condition deliberately
  // has only player recipients and must not be weakened for a test fixture.
  return world.apply_condition(source,source,kind,at,duration)&&
   world.condition_transfer(source,target,*skills::effects::condition_transfer(154),at);
 }
 bool prerequisite(unsigned skill,bool enabled,std::uint64_t at=100,std::uint64_t duration=5000){
  if(!enabled)return true;
  using enum combat::conditions::Kind;
  if(skill==334||skill==342)return condition(deep_wound,at,duration);
  if(skill==850)return condition(weakness,at,duration);
  if(skill==352||skill==849){
   combat::weapon::Owned hammer=weapon;hammer.item_type=15;
   if(!world.ordinary_weapon(source,hammer,20)||!world.melee_attack(source,target,hammer,*combat::weapon::attack_skill(331,0),at)||!world.ordinary_weapon(source,weapon,20))return false;
   return world.knocked_down(target,at);
  }
  return true;
 }
};
}
