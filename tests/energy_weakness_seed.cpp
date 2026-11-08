// Native World regression for59 Weakness -> Fragility -> Healing Seed.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy Weakness Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,target,monk,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 explicit Fixture(std::optional<bool> allowed,bool reversal=false){
  world::Instance::Npc body;body.id=body.definition.id=100;body.x=170;
  body.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition definition;
  definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health={500,1000};
  definition.damageable=true;definition.healing_allowed=allowed;
  if(allowed.has_value())definition.healing_evidence="synthetic condition Seed fixture";
  body.combat_definition=definition;w.configure_npcs({body});
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:id==2?160:323,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);
   check(w.combat_armor(*w.effect_scope(id),60));check(w.elemental_armor(*w.effect_scope(id),60,60));
  }
  source=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);npc=*w.effect_scope(100);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));
  // Single-target variant keeps the nearby monk unhexed.
  check(w.degeneration_hex(source,target,*effects::degeneration_hex(2998,12),100));
  check(w.status_mutation(target,1399,800,12,100,6100,true));
  if(reversal)check(w.status_mutation(target,307,801,12,100,8100,true));
  box->drain();
 }
 bool cast(){return w.energy_drain(source,target,*effects::energy_drain(59,12),20,100);}
 void remove_seed(){
  const auto status=*w.status_snapshot(target,100);
  const auto lease=std::find_if(status.begin(),status.end(),[](const auto& e){return e.skill==274;});
  check(lease!=status.end()&&w.status_mutation(target,274,lease->instance,lease->rank,100,{},false));
 }
};
int main()try{
 const auto plan=*effects::energy_drain(59,12);
 const auto seed=*effects::healing_seed(12);
 const auto fragility=*effects::fragility(2998,12);
 const double loss=std::max(0.,double(fragility.damage)-5.);
 check(plan.drain_each&&plan.radius>=163&&plan.weakness_ms>0&&plan.damage_per_energy==0&&loss>0);
 for(const auto allowed:{std::optional<bool>{},std::optional<bool>{false},std::optional<bool>{true}}){
  Fixture f(allowed);const bool accepted=f.cast();const auto packets=f.box->drain();
  if(!allowed.has_value()){
   check(!accepted);
   check(f.w.resources(1)->energy==100&&f.w.resources(2)->energy==100&&f.w.resources(3)->energy==100);
   check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.npc_combat_snapshot(f.npc)->health->current==500);
   check(!f.w.has_condition(f.target,combat::conditions::Kind::weakness,100)&&!f.w.has_condition(f.monk,combat::conditions::Kind::weakness,100));
   check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,2998))==packets.end());
   f.remove_seed();const auto aftershock=*effects::caster_earth_aoe(174,0);
   check(f.w.shockwave(f.source,aftershock,20,100));
   const auto first_loss=std::max(0.,double(aftershock.strikes[0].earth_damage)-5.);
   check(f.w.resources(2)->health==unsigned(500-first_loss));
  }else{
   check(accepted&&f.w.resources(2)->energy==100-plan.amount&&f.w.resources(3)->energy==100-plan.amount);
   check(f.w.resources(2)->health==unsigned(500-loss+seed.healing)&&f.w.resources(3)->health==500+seed.healing);
   check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(*allowed?seed.healing:0));
   check(f.w.has_condition(f.target,combat::conditions::Kind::weakness,100)&&f.w.has_condition(f.monk,combat::conditions::Kind::weakness,100));
   const auto damage=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,2998));
   const auto healing=std::find(packets.begin(),packets.end(),skills::particles::health_change_2022(3,3,seed.healing,1000));
   check(damage!=packets.end()&&healing!=packets.end()&&damage<healing);
  }
 }
 //59 itself deals no damage. Only its condition reaction consumes1399/307.
 // Full conversion therefore avoids Seed recipient admission despite unknownNPC.
 {
  Fixture f(std::nullopt,true);check(loss<=effects::reversal_fortune(12)->maximum_healing);
  check(f.cast());check(f.w.resources(2)->energy==100-plan.amount&&f.w.resources(3)->energy==100-plan.amount);
  check(f.w.resources(2)->health==unsigned(500+loss)&&f.w.resources(3)->health==500&&f.w.npc_combat_snapshot(f.npc)->health->current==500);
  check(f.w.has_condition(f.target,combat::conditions::Kind::weakness,100));
  const auto status=*f.w.status_snapshot(f.target,100);
  check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;}));
  check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==1399;}));
  const auto packets=f.box->drain();
  check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,2998))==packets.end());
  check(std::find(packets.begin(),packets.end(),skills::particles::health_change_2022(3,3,seed.healing,1000))==packets.end());
 }
 std::cout<<"PASS59 Weakness/Fragility/Seed atomic admission, healing immunity and full conversion\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
