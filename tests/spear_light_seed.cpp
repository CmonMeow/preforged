// Detached native World flight regression. No live client/capture dependency.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Spear Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,target,monk,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned hp,bool absorption=false){
  world::Instance::Npc body;body.id=body.definition.id=100;body.x=910;
  body.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition definition;
  definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health={500,1000};
  definition.damageable=true;definition.healing_allowed=allowed;
  if(allowed.has_value())definition.healing_evidence="synthetic Spear Seed fixture";
  body.combat_definition=definition;w.configure_npcs({body});
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:id==2?900:1000,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:id==2?hp:500u,1000,100,100},100);
  }
  source=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);npc=*w.effect_scope(100);
  check(w.player_holy_equipment(target,0)&&w.configure_projectiles({1800,32}));
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));
  if(absorption)check(w.status_mutation(target,1399,800,12,100,6100,true));
  box->drain();
 }
 void release(){
  std::vector<world::Packet> visuals;std::function<void()> publish;
  check(w.spear_light(source,target,*effects::spear_light(12),20,1000,&visuals,&publish));
  check(visuals.size()==1&&bool(publish));w.skill_event(1,visuals,1000);publish();
  check(w.attack_activity(target,2000,1100)); // Bonus is sampled at actual impact.
  w.tick(1400);box->drain();
 }
 void remove_seed(unsigned at){
  const auto status=*w.status_snapshot(target,at);
  const auto lease=std::find_if(status.begin(),status.end(),[](const auto& e){return e.skill==274;});
  check(lease!=status.end()&&w.status_mutation(target,274,lease->instance,lease->rank,at,{},false));
 }
};
int main()try{
 const auto plan=*effects::spear_light(12);
 const auto seed=*effects::healing_seed(12);
 // First hit survives. Its Seed heal keeps the victim alive through the
 // separate bonus; a preview that omits feedback predicts a lethal second hit.
 const unsigned hp=plan.damage+plan.attacking_bonus-seed.healing+1;
 check(hp>plan.damage&&hp<plan.damage+plan.attacking_bonus);
 for(bool allowed:{false,true}){
  Fixture f(allowed,hp);f.release();check(f.w.resources(2)->health==hp);
  f.w.tick(1500);
  const unsigned expected=hp+2*seed.healing-plan.damage-plan.attacking_bonus;
  check(f.w.resources(2)->health==expected&&f.w.resources(3)->health==500+2*seed.healing);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(allowed?2*seed.healing:0));
  const auto packets=f.box->drain();const auto damage=combat::detail::integer(10,1,1130);
  const auto healing=skills::particles::health_change_2022(3,3,seed.healing,1000);
  std::vector<unsigned> order;
  for(const auto& packet:packets){if(packet==damage)order.push_back(1);else if(packet==healing)order.push_back(2);}
  check(order==std::vector<unsigned>({1,2,1,2}));
  f.w.tick(1600);check(f.w.resources(2)->health==expected);
  const auto later=f.box->drain();check(std::find(later.begin(),later.end(),damage)==later.end());
 }
 // Unknown eligibility is discovered at impact, but must reject BOTH events
 // before spending the first hit's Health/protection contact.
 {
  Fixture f(std::nullopt,500,true);f.release();f.w.tick(1500);
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.npc_combat_snapshot(f.npc)->health->current==500);
  const auto packets=f.box->drain();const auto damage=combat::detail::integer(10,1,1130);
  check(std::find(packets.begin(),packets.end(),damage)==packets.end());
  f.w.tick(1600);check(f.w.resources(2)->health==500);
  f.remove_seed(1600);
  const auto burn=*effects::energy_drain(42,12);
  check(f.w.before_spell_effect(f.source,42,1,1600));
  check(f.w.energy_drain(f.source,f.target,burn,20,1600,nullptr,1));
  check(f.w.resources(2)->health==500-(burn.amount*burn.damage_per_energy-5));
 }
 std::cout<<"PASS Spear separate-hit Seed survival, eligibility admission, counter preservation and wire feedback order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
