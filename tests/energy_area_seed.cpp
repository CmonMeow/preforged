// Native regression: Energy Burn/Surge use shared protected damage and Seed.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy area Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,target,monk;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 explicit Fixture(bool unknown=false){
  if(unknown){world::Instance::Npc n;n.id=n.definition.id=100;n.x=170;
   n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
   d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};
   d.damageable=true;d.healing_allowed=std::nullopt;n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u,3u}){
   // Monk is163 from victim (inside Seed166),323 from caster (outside174).
   w.join(id,"Actor",id==1?0:id==2?160:323,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);
   check(w.combat_armor(*w.effect_scope(id),60));check(w.elemental_armor(*w.effect_scope(id),60,60));
  }
  source=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));
  check(w.status_mutation(target,1399,800,12,100,6100,true));box->drain();
 }
 bool cast(){check(w.before_spell_effect(source,42,1,100));
  return w.energy_drain(source,target,*effects::energy_drain(42,12),20,100,nullptr,1);}
};
struct SurgeFixture{
 world::Instance w;effects::Scope source,primary,later,monk,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 SurgeFixture(std::optional<bool> allowed,unsigned primary_energy=100,bool reversal=false){
  world::Instance::Npc body;body.id=body.definition.id=100;body.x=110;
  body.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition definition;
  definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health={500,1000};
  definition.damageable=true;definition.healing_allowed=allowed;
  if(allowed.has_value())definition.healing_evidence="synthetic Seed admission";
  body.combat_definition=definition;w.configure_npcs({body});
  for(unsigned id:{1u,2u,3u,4u}){
   // Later is200 from primary (inside canonical252 Surge radius).
   // Monk is163 from later,363 from primary, outside Surge collateral.
   const float x=id==1?0:id==2?-100:id==3?100:263;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:500u,1000,id==2?primary_energy:id==3?37u:100u,100},100);
   check(w.combat_armor(*w.effect_scope(id),60));check(w.elemental_armor(*w.effect_scope(id),60,60));
  }
  source=*w.effect_scope(1);primary=*w.effect_scope(2);later=*w.effect_scope(3);
  monk=*w.effect_scope(4);npc=*w.effect_scope(100);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  check(w.complete_healing_seed(monk,later,*effects::healing_seed(12),0,100));
  for(const auto scope:{primary,later}){
   check(w.status_mutation(scope,1399,800,12,100,6100,true));
   if(reversal)check(w.status_mutation(scope,307,801,12,100,8100,true));
  }
  box->drain();
 }
 bool cast(){check(w.before_spell_effect(source,39,1,100));
  return w.energy_drain(source,primary,*effects::energy_drain(39,12),20,100,nullptr,1);}
 void remove_seed(){
  const auto status=*w.status_snapshot(later,100);
  const auto lease=std::find_if(status.begin(),status.end(),[](const auto& e){return e.skill==274;});
  check(lease!=status.end()&&w.status_mutation(later,274,lease->instance,lease->rank,100,{},false));
 }
};
int main()try{
 const auto seed=*effects::healing_seed(12);
 const auto burn=*effects::energy_drain(42,12);
 for(int reversal:{-1,0,12}){
  Fixture f;if(reversal>=0)check(f.w.status_mutation(f.target,307,801,unsigned(reversal),100,8100,true));
  f.box->drain();check(f.cast());
  const double incoming=double(burn.amount*burn.damage_per_energy)-5;
  const double converted=reversal<0?0:std::min(incoming,double(effects::reversal_fortune(unsigned(reversal))->maximum_healing));
  const double loss=incoming-converted;const unsigned triggers=loss>0?1u:0u;
  check(f.w.resources(2)->energy==100-burn.amount);
  check(f.w.resources(2)->health==unsigned(500+converted-loss+triggers*seed.healing));
  check(f.w.resources(3)->health==500+triggers*seed.healing);
  const auto packets=f.box->drain();
  const auto damage=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,42));
  const auto healing=std::find(packets.begin(),packets.end(),skills::particles::health_change_2022(3,3,seed.healing,1000));
  if(triggers)check(damage!=packets.end()&&healing!=packets.end()&&damage<healing);
  else check(damage==packets.end()&&healing==packets.end());
 }
 {
  Fixture f(true);check(!f.cast());
  check(f.w.resources(1)->health==1000&&f.w.resources(1)->energy==100);
  check(f.w.resources(2)->health==500&&f.w.resources(2)->energy==100);
  check(f.w.resources(3)->health==500&&f.w.resources(3)->energy==100);
  const auto packets=f.box->drain();
  check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,42))==packets.end());
  // Remove Seed by exact effect identity, retaining the unknown NPC and1399.
  // Independent corrected Earth consumer proves refusal did not spend1399.
  const auto status=*f.w.status_snapshot(f.target,100);
  const auto lease=std::find_if(status.begin(),status.end(),[](const auto& e){return e.skill==274;});
  check(lease!=status.end());
  check(f.w.status_mutation(f.target,274,lease->instance,lease->rank,100,{},false));
  const auto aftershock=*effects::caster_earth_aoe(174,0);
  check(f.w.shockwave(f.source,aftershock,20,100));
  const auto loss=std::max(0.,double(aftershock.strikes[0].earth_damage)-5.);
  check(f.w.resources(2)->health==unsigned(500-loss)&&f.w.resources(3)->health==500);
 }
 const auto surge=*effects::energy_drain(39,12);
 check(surge.radius>=200&&surge.radius<363&&!surge.drain_each);
 const auto surge_loss=double(surge.amount*surge.damage_per_energy)-5;
 for(const auto allowed:{std::optional<bool>{},std::optional<bool>{false},std::optional<bool>{true}}){
  SurgeFixture f(allowed);const bool accepted=f.cast();const auto packets=f.box->drain();
  if(!allowed.has_value()){
   check(!accepted&&f.w.resources(2)->energy==100&&f.w.resources(3)->energy==37);
   check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.resources(4)->health==500);
   check(f.w.npc_combat_snapshot(f.npc)->health->current==500);
   check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,39))==packets.end());
   f.remove_seed();const auto aftershock=*effects::caster_earth_aoe(174,0);
   check(f.w.shockwave(f.source,aftershock,20,100));
   const auto loss=std::max(0.,double(aftershock.strikes[0].earth_damage)-5.);
   check(f.w.resources(2)->health==unsigned(500-loss)&&f.w.resources(3)->health==unsigned(500-loss));
  }else{
   check(accepted&&f.w.resources(2)->energy==100-surge.amount&&f.w.resources(3)->energy==37);
   check(f.w.resources(2)->health==unsigned(500-surge_loss));
   check(f.w.resources(3)->health==unsigned(500-surge_loss+seed.healing)&&f.w.resources(4)->health==500+seed.healing);
   check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(*allowed?seed.healing:0));
  }
 }
 // No primary Energy means no damage, so the unknown Seed recipient is not
 // queried and no protection contact is consumed, even on collateral players.
 {
  SurgeFixture f(std::nullopt,0);check(f.cast());
  check(f.w.resources(2)->energy==0&&f.w.resources(3)->energy==37);
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.resources(4)->health==500);
  f.remove_seed();const auto aftershock=*effects::caster_earth_aoe(174,0);
  check(f.w.shockwave(f.source,aftershock,20,100));
  const auto loss=std::max(0.,double(aftershock.strikes[0].earth_damage)-5.);
  check(f.w.resources(2)->health==unsigned(500-loss)&&f.w.resources(3)->health==unsigned(500-loss));
 }
 // Fully converted damage spends307/1399 and primary Energy, but produces no
 // positive loss and therefore must not reject on unknown Seed healing facts.
 {
  SurgeFixture f(std::nullopt,100,true);check(f.cast());
  check(f.w.resources(2)->energy==100-surge.amount&&f.w.resources(3)->energy==37);
  check(f.w.resources(2)->health==unsigned(500+surge_loss)&&f.w.resources(3)->health==unsigned(500+surge_loss));
  check(f.w.resources(4)->health==500&&f.w.npc_combat_snapshot(f.npc)->health->current==500);
  for(const auto scope:{f.primary,f.later}){const auto status=*f.w.status_snapshot(scope,100);
   check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;}));
   check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==1399;}));}
 }
 std::cout<<"PASS Energy Burn/Surge Seed, conversion, empty Energy and atomic unknown-healing admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
