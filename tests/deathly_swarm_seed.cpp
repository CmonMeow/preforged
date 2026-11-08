// Detached delayed-contact105 Healing Seed regression; no retail tie-policy claim.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Deathly Swarm Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,target,second,third,healer,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned hp){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=250;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;d.healing_allowed=allowed;
  if(allowed.has_value())d.healing_evidence="synthetic delayed105 Seed fixture";
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u,4u,9u}){
   const float x=id==1?0:id==2?100:id==3?400:id==4?700:-60;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==2?hp:500u,1000,100,100},100);
   check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),60,60));
  }
  source=*w.effect_scope(1);target=*w.effect_scope(2);second=*w.effect_scope(3);third=*w.effect_scope(4);healer=*w.effect_scope(9);npc=*w.effect_scope(100);
  check(w.party_action(2,158,9,100)&&w.party_action(9,154,2,100));
  check(w.complete_healing_seed(healer,target,*effects::healing_seed(12),0,100));
  check(w.status_mutation(target,1399,800,12,100,6100,true));
 }
 void launch(){check(w.deathly_swarm(source,target,*effects::deathly_swarm(12),20,100));box->drain();}
};
int main()try{
 const auto plan=*effects::deathly_swarm(12);const auto seed=*effects::healing_seed(12);
 const double loss=std::max(0.,plan.damage-5.);
 const unsigned low=unsigned(std::ceil(loss+1));
 check(plan.radius>=300&&plan.radius<460&&seed.adjacent>=160&&seed.adjacent<300);
 for(bool allowed:{false,true}){
  Fixture f(allowed,low);f.launch();f.w.tick(1000);
  check(f.w.resources(2)->health==low&&f.w.resources(3)->health==500);f.box->drain();
  f.w.tick(1100);
  if(f.w.resources(2)->health!=unsigned(std::ceil(low-loss+seed.healing)))
   throw std::runtime_error("First105 Health="+std::to_string(f.w.resources(2)->health)+" expected="+std::to_string(unsigned(std::ceil(low-loss+seed.healing)))+" raw="+std::to_string(plan.damage));
  check(f.w.resources(9)->health==500+seed.healing);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(allowed?seed.healing:0));
  check(f.w.resources(3)->health==500&&f.w.resources(4)->health==500);
  const auto packets=f.box->drain();std::vector<unsigned> order;
  const auto damage=combat::detail::integer(10,1,105),healing=skills::particles::health_change_2022(2,9,seed.healing,1000);
  for(const auto& p:packets){if(p==damage)order.push_back(1);else if(p==healing)order.push_back(2);}
  check(order==std::vector<unsigned>({1,2}));
  // Stable actor order selects3 then4. Healer9 is a legal foe but loses the
  // private-server tie; this does not assert retail's undocumented tie rule.
  f.w.tick(2100);check(f.w.resources(3)->health==unsigned(std::ceil(500-plan.damage))&&f.w.resources(4)->health==500);
  f.w.tick(3100);check(f.w.resources(4)->health==unsigned(std::ceil(500-plan.damage)));
  const auto after=f.w.resources(2)->health;f.w.tick(3200);check(f.w.resources(2)->health==after);
 }
 // Lock the once-only condition-armor transformation:100 ->80, not60.
 {
  Fixture f(true,500);check(f.w.elemental_armor(f.target,60,100));
  check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::cracked_armor,100,5000));
  f.launch();f.w.tick(1100);
  const double mitigated=plan.damage*std::exp2(-.5)-5.;
  check(f.w.resources(2)->health==unsigned(std::ceil(500-mitigated+seed.healing)));
 }
 {
  Fixture f(std::nullopt,500);f.launch();f.w.tick(1100);
  check(f.w.resources(2)->health==500&&f.w.resources(9)->health==500&&f.w.resources(3)->health==500);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500);
  const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,105))==packets.end());
  // Cancel remaining delayed work by removing its exact source. This fixture
  // intentionally does not prescribe retry versus consumed-contact policy.
  f.w.leave(1);f.w.move(2,-100,0,1200);f.w.tick(2000);
  check(std::abs(f.w.position(2,2000).first+100)<.5);
  // Target is350 from unknownNPC250 but40 from healer-60: retain Seed,
  // its ordinary eligible heal, and the original1399 application.
  f.w.join(1,"New caster",0,0,f.box,0,{}, {},2000,1);
  f.w.set_vitals(1,{500,1000,100,100},100);const auto source=*f.w.effect_scope(1);
  const auto burn=*effects::energy_drain(42,12);check(f.w.before_spell_effect(source,42,1,2000));
  check(f.w.energy_drain(source,f.target,burn,20,2000,nullptr,1));
  check(f.w.resources(2)->health==500-(burn.amount*burn.damage_per_energy-5)+seed.healing);
  // The first absorption step remains available: rejected105 spent no1399.
 }
 std::cout<<"PASS delayed105 Seed contact ordering, known/immune/refusal and refreshed chain selection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
