// Detached generic elemental cohort/Seed admission regressions.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Elemental Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,early,later,monk,ally,enemy;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned later_hp,bool early_seed=true){
  std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?910:700;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.spawn_health={500,1000};d.damageable=true;
   d.relation=id==100?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
   if(id==100){d.healing_allowed=allowed;if(allowed.has_value())d.healing_evidence="synthetic elemental Seed";
    if(!allowed.has_value()){d.creature_type="spirit";d.creature_type_evidence="authored unsupported Seed gain recipient";d.fleshy=false;d.fleshy_evidence="authored non-fleshy spirit";}
    if(allowed==std::optional(false)){combat::npcs::LiveActor immune(d,{1,1,1,146,id});const auto life=immune.lifetime();
     check(!immune.heal_ally(life,25,100)&&immune.actor(life)->health->current==500);}
   }
   else{d.conditions_allowed=true;d.fire_armor=d.cold_armor=60;d.fire_armor_evidence=d.cold_armor_evidence="synthetic elemental armor";}
   n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u,4u}){
   w.join(id,"Actor",id==1?0:id==2?600:id==3?760:920,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==3?later_hp:500u,1000,100,100},100);check(w.energy_capacity(id,6,0,100,{80,-2}));
   check(w.elemental_armor(*w.effect_scope(id),60,60));
  }
  source=*w.effect_scope(1);early=*w.effect_scope(2);later=*w.effect_scope(3);monk=*w.effect_scope(4);ally=*w.effect_scope(100);enemy=*w.effect_scope(101);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  check(w.complete_healing_seed(monk,later,*effects::healing_seed(12),0,100));
  if(early_seed)check(w.complete_healing_seed(monk,early,*effects::healing_seed(12),0,100));
  for(auto scope:{early,later})check(w.status_mutation(scope,1399,800,12,100,6100,true));
 }
 bool cast(unsigned skill,effects::Scope target,std::uint64_t now=1000){
  std::vector<world::Packet> visuals;box->drain();return w.elemental(source,target,*effects::elemental(skill,12),20,now,&visuals);
 }
};
int main()try{
 // Lightning must apply condition armor once, matching fire/cold and the
 // fresh World actual-hit path.100 ->80 with Cracked Armor.
 {
  combat::Actor actor;actor.id=2;actor.generation=1;actor.health=combat::Health{1000,1000};actor.lightning_armor=100;actor.damageable=true;
  combat::conditions::State conditions;check(conditions.apply(combat::conditions::Kind::cracked_armor,1,100,5000));
  const auto plan=*effects::elemental(1227,12);
  const auto result=effects::elemental_target(plan,actor,conditions,20,1,1000);
  check(result.accepted&&std::abs(result.actor.health->current-(1000-plan.damage*std::exp2(-.5)))<1e-8);
 }
 const auto seed=*effects::healing_seed(12);const auto plan=*effects::elemental(189,12);
 const auto fragility=*effects::fragility(2998,12);
 const double direct=plan.damage-5.,reaction=std::max(0.,double(fragility.damage)-10.);
 const unsigned low=unsigned(std::ceil(direct-seed.healing+1));
 check(plan.radius>=160&&plan.radius<310&&low<direct&&reaction>0);
 {
  Fixture f(std::nullopt,500,false);check(!f.cast(189,f.early));
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.npc_combat_snapshot(f.enemy)->health->current==500);
  check(!f.w.has_condition(f.early,combat::conditions::Kind::burning,1000)&&!f.w.has_condition(f.enemy,combat::conditions::Kind::burning,1000));
  // EarlierSeed cannot see unknownNPC910 from600, so this followup proves
  // whole-cohort refusal left the earlier1399 first step unconsumed.
  check(f.w.complete_healing_seed(f.monk,f.early,seed,0,1000));
  const auto burn=*effects::energy_drain(42,12);check(f.w.before_spell_effect(f.source,42,1,1000));
  check(f.w.energy_drain(f.source,f.early,burn,20,1000,nullptr,1));
  check(f.w.resources(2)->health==500-(burn.amount*burn.damage_per_energy-5)+seed.healing);
 }
 for(bool allowed:{false,true}){
  Fixture f(allowed,low);check(f.w.degeneration_hex(f.source,f.later,*effects::degeneration_hex(2998,12),100));
  check(f.cast(189,f.early));
  check(f.w.resources(2)->health==unsigned(std::ceil(500-direct+3*seed.healing)));
  check(f.w.resources(3)->health==unsigned(std::ceil(low-direct-reaction+3*seed.healing)));
  check(f.w.resources(4)->health==500+2*seed.healing);
  check(f.w.npc_combat_snapshot(f.ally)->health->current==500+2*seed.healing);
  check(f.w.npc_combat_snapshot(f.enemy)->health->current==500-plan.damage);
  check(f.w.has_condition(f.early,combat::conditions::Kind::burning,1000)&&f.w.has_condition(f.later,combat::conditions::Kind::burning,1000));
  const auto packets=f.box->drain();std::vector<unsigned> order;
  const auto damage=combat::detail::integer(10,1,189),reactive=combat::detail::integer(10,1,2998),healing=skills::particles::health_change_2022(3,4,seed.healing,1000);
  for(const auto& p:packets){if(p==damage)order.push_back(1);else if(p==reactive)order.push_back(3);else if(p==healing)order.push_back(2);}
  // Last189 attribution is the hostileNPC contact after player reactions.
  check(order==std::vector<unsigned>({1,2,1,2,3,2,1}));
 }
 //884 first applies Burning without direct damage; without Fragility no
 // Seed fact is needed even though the nearby explicit spirit lacks reviewed gain-immunity facts.
 {
  Fixture f(std::nullopt,500,false);check(f.cast(884,f.early));
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.npc_combat_snapshot(f.enemy)->health->current==500);
  check(f.w.has_condition(f.early,combat::conditions::Kind::burning,1000)&&f.w.has_condition(f.later,combat::conditions::Kind::burning,1000));
  // Repeating while Burning switches884 to damage, which now requires Seed.
  check(!f.cast(884,f.early,1000));check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500);
 }
 //89's above-half shadow event is a distinct protected hit. This also
 // exercises the single-recipient generic route if it remains separate.
 for(bool allowed:{false,true}){
  Fixture f(allowed,750,false);const auto two=*effects::elemental(89,12);check(f.cast(89,f.later));
  check(f.w.resources(3)->health==unsigned(std::ceil(750-(two.damage-5)-(two.shadow_above_half-10)+2*seed.healing)));
  check(f.w.resources(4)->health==500+2*seed.healing);
 }
 std::cout<<"PASS elemental mixed-cohort Seed admission, survival/Fragility, burn-only and two-hit routes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
