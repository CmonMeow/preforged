// Detached regression: ordered Mind Shock hits, Seed feedback and real survival.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mind Shock Seed line "+std::to_string(at.line()));
}
double damage_at(const effects::MindShockPlan& plan,int armor){
 combat::Actor actor{2,1,combat::Health{1000,1000},{},0,true};actor.lightning_armor=armor;
 const auto hit=combat::apply(actor,{combat::Kind::lightning,plan.damage,20,plan.skill,plan.penetration},true);
 check(hit.reason==combat::Reason::accepted);return hit.damage;
}
struct Fixture{
 world::Instance w;effects::Scope source,target,monk,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned hp,bool absorption=false){
  world::Instance::Npc body;body.id=body.definition.id=100;body.x=110;
  body.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition definition;
  definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health={500,1000};
  definition.damageable=true;definition.healing_allowed=allowed;
  if(allowed.has_value())definition.healing_evidence="synthetic Mind Shock Seed fixture";
  body.combat_definition=definition;w.configure_npcs({body});
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:id==2?hp:500u,1000,id==1?80u:20u,100},100);
   check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.lightning_armor(*w.effect_scope(id),80));
  }
  source=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);npc=*w.effect_scope(100);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));
  if(absorption)check(w.status_mutation(target,1399,800,12,100,6100,true));box->drain();
 }
 void remove_seed(){const auto status=*w.status_snapshot(target,1000);
  const auto lease=std::find_if(status.begin(),status.end(),[](const auto& e){return e.skill==274;});
  check(lease!=status.end()&&w.status_mutation(target,274,lease->instance,lease->rank,1000,{},false));}
};
std::vector<unsigned> feedback(const std::vector<world::Packet>& packets,unsigned skill,unsigned healing){
 std::vector<unsigned> order;const auto damage=combat::detail::integer(10,1,skill);
 const auto heal=skills::particles::health_change_2022(3,3,healing,1000);
 for(const auto& packet:packets){if(packet==damage)order.push_back(1);else if(packet==heal)order.push_back(2);}
 return order;
}
int main()try{
 const auto seed=*effects::healing_seed(12);
 for(unsigned skill:{226u,2804u}){
  const auto plan=*effects::mind_shock(skill,12);const double raw=damage_at(plan,80);
  const unsigned hp=unsigned(std::ceil(2*raw-seed.healing+1));check(hp>raw&&hp<2*raw);
  for(bool allowed:{false,true}){
   Fixture f(allowed,hp);std::vector<world::Packet> cosmetics;
   check(f.w.mind_shock(f.source,f.target,plan,20,1000,&cosmetics));
   check(std::abs(double(f.w.resources(2)->health)-(hp-2*raw+2*seed.healing))<1.01);
   check(f.w.resources(3)->health==500+2*seed.healing&&f.w.knocked_down(f.target,1000));
   check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(allowed?2*seed.healing:0));
   check(feedback(f.box->drain(),skill,seed.healing)==std::vector<unsigned>({1,2,1,2}));
   check(feedback(cosmetics,skill,seed.healing).empty());
  }
  {
   Fixture f(std::nullopt,500,true);check(!f.w.mind_shock(f.source,f.target,plan,20,1000));
   check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&!f.w.knocked_down(f.target,1000));
   check(feedback(f.box->drain(),skill,seed.healing).empty());
   f.remove_seed();const auto burn=*effects::energy_drain(42,12);
   check(f.w.before_spell_effect(f.source,42,1,1000));
   check(f.w.energy_drain(f.source,f.target,burn,20,1000,nullptr,1));
   check(f.w.resources(2)->health==500-(burn.amount*burn.damage_per_energy-5));
  }
  // One Watch Yourself charge: full conversion preserves armor for hit2;
  // partial conversion spends it on hit1. Derive penetration through combat.
  for(int reversal:{-1,0,20}){
   Fixture f(false,500);const auto watch=*effects::watch_yourself(0,true);
   check(watch.charges==1&&f.w.stage_watch_yourself_recipient(f.monk,f.target,watch,100));
   if(reversal>=0)check(f.w.status_mutation(f.target,307,801,unsigned(reversal),100,8100,true));
   f.box->drain();const double first=damage_at(plan,80+int(watch.armor));
   const double converted=reversal<0?0:std::min(first,double(effects::reversal_fortune(unsigned(reversal))->maximum_healing));
   const double second=converted==first?first:raw;const unsigned triggers=converted==first?1:2;
   std::vector<world::Packet> cosmetics;check(f.w.mind_shock(f.source,f.target,plan,20,1000,&cosmetics));
   const double expected=500+converted-(first-converted)-second+triggers*seed.healing;
   check(std::abs(double(f.w.resources(2)->health)-expected)<1.01);
   check(f.w.resources(3)->health==500+triggers*seed.healing&&f.w.knocked_down(f.target,1000));
   check(f.w.watch_yourself_armor(f.target,1000)==0);
   check(feedback(f.box->drain(),skill,seed.healing)==(triggers==1?std::vector<unsigned>({1,2}):std::vector<unsigned>({1,2,1,2})));
   check(feedback(cosmetics,skill,seed.healing).empty());
  }
 }
 std::cout<<"PASS Mind Shock Seed survival/KD, atomic refusal, penetration, Watch/conversion and feedback order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
