// Detached source-cost/condition/reaction admission for Signet of Agony.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Agony Seed line "+std::to_string(at.line()));
}
int main()try{
 const auto seed=*effects::healing_seed(12);const auto fragility=*effects::fragility(2998,12);
 const auto plan=*effects::elemental(145,12);
 const double reaction=std::max(0.,double(fragility.damage)-5.);
 const unsigned hp=unsigned(std::ceil(100+reaction+1));check(reaction>0&&hp<200);
 for(int eligibility:{-1,0,1}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=-100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;
  d.healing_allowed=eligibility<0?std::optional<bool>{}:std::optional(bool(eligibility));
  if(eligibility>=0)d.healing_evidence="synthetic source Agony Seed";
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:id==2?200:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);
   w.set_vitals(id,{id==1?hp:500u,1000,100,100},100);check(w.energy_capacity(id,4,0,100,{80,-2}));
  }
  const auto source=*w.effect_scope(1),enemy=*w.effect_scope(2),monk=*w.effect_scope(3),ally=*w.effect_scope(100);
  check(w.party_action(1,158,3,100)&&w.party_action(3,154,1,100));
  check(w.complete_healing_seed(monk,source,seed,0,100));
  check(w.degeneration_hex(enemy,source,*effects::degeneration_hex(2998,12),100));
  for(const auto owner:{source,enemy})check(w.status_mutation(owner,1399,800,12,100,6100,true));
  std::optional<skills::sacrifice::Loss> paid;std::vector<world::Packet> visuals;std::function<void()> publication;box->drain();
  const bool accepted=w.elemental(source,source,plan,20,100,&visuals,&paid,nullptr,{},&publication);
  check(accepted==(eligibility>=0));
  if(eligibility<0){
   check(!paid&&!publication&&visuals.empty());
   check(w.resources(1)->health==hp&&w.resources(2)->health==500&&w.resources(3)->health==500);
   check(!w.has_condition(source,combat::conditions::Kind::bleeding,100));
   check(w.npc_combat_snapshot(ally)->health->current==500);
   const auto packets=box->drain();check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,2,2998))==packets.end());
   check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,145))==packets.end());
   // Enemy has noSeed; this independent damage probe does not require the
   // rejected source's nearby unknown NPC and proves victim1399 unspent.
   const auto burn=*effects::energy_drain(42,12);check(w.before_spell_effect(source,42,1,100));
   check(w.energy_drain(source,enemy,burn,20,100,nullptr,1));
   check(w.resources(2)->health==500-(burn.amount*burn.damage_per_energy-5));
   continue;
  }
  check(paid&&paid->requested==100&&paid->remaining==hp-100&&!paid->killed);
  check(w.resources(1)->health==unsigned(std::ceil(hp-100-reaction+seed.healing)));
  check(w.resources(2)->health==unsigned(std::ceil(500-(plan.damage-5))));
  check(w.resources(3)->health==500+seed.healing);
  check(w.npc_combat_snapshot(ally)->health->current==500+(eligibility==1?seed.healing:0));
  check(w.has_condition(source,combat::conditions::Kind::bleeding,100));
  check(publication&&box->drain().empty());box->enqueue(visuals);auto copy=publication;publication();const auto packets=box->drain();copy();check(box->drain().empty());
  const auto reactive=combat::detail::integer(10,2,2998),heal=skills::particles::health_change_2022(1,3,seed.healing,1000),damage=combat::detail::integer(10,1,145);
  std::vector<unsigned> order;for(const auto& p:packets){if(p==reactive)order.push_back(1);else if(p==heal)order.push_back(2);else if(p==damage)order.push_back(3);}
  check(order==std::vector<unsigned>({1,2,3}));
 }
 std::cout<<"PASS Agony source sacrifice/Bleeding/Fragility/Seed admission before victim damage\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
