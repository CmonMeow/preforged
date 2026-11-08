// Detached shared holy-family Healing Seed admission regression.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Holy Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,target,monk,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned health=500){
  world::Instance::Npc body;body.id=body.definition.id=100;body.x=250;body.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;d.healing_allowed=allowed;
  if(allowed.has_value())d.healing_evidence="synthetic holy Seed fixture";body.combat_definition=d;w.configure_npcs({body});
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:id==2?100:260,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==2?health:500u,1000,id==1?50u:100u,100},100);
   check(w.energy_capacity(id,3,0,100,{80,-2}));
  }
  source=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);npc=*w.effect_scope(100);
  check(w.player_holy_equipment(target,0));
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));
  check(w.status_mutation(target,1399,800,12,100,6100,true));
 }
 bool cast(unsigned skill){
  box->drain();std::vector<world::Packet> visuals;std::function<void()> publication;
  const bool ok=w.holy_damage(source,target,*effects::holy_damage(skill,12),20,1000,&visuals,&publication);
  if(ok){check(bool(publication));check(box->drain().empty());box->enqueue(visuals);auto copy=publication;publication();const auto sent=box->drain();copy();check(box->drain().empty());box->enqueue(sent);}
  else check(!publication&&visuals.empty());
  return ok;
 }
};
int main()try{
 const auto seed=*effects::healing_seed(12);
 const auto strike=*effects::holy_damage(312,12);
 const double first=strike.damage-5.,second=strike.knocked_down_bonus-10.;
 const unsigned low=unsigned(first+second-seed.healing+1);
 check(low>first&&low<first+second);
 for(bool allowed:{false,true}){
  Fixture f(allowed,low);check(f.w.gale(f.source,f.target,12,{},100));check(f.cast(312));
  check(f.w.resources(2)->health==unsigned(low-first-second+2*seed.healing));
  check(f.w.resources(3)->health==500+2*seed.healing);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(allowed?2*seed.healing:0));
  const auto packets=f.box->drain();std::vector<unsigned> order;
  const auto body=*skills::particles::targeted_body_2022(2,1,312),damage=combat::detail::integer(10,1,312);
  const auto healing=skills::particles::health_change_2022(2,3,seed.healing,1000);
  for(const auto& p:packets){if(p==body)order.push_back(0);else if(p==damage)order.push_back(1);else if(p==healing)order.push_back(2);}
  check(order==std::vector<unsigned>({0,1,2,1,2}));
 }
 for(unsigned skill:{312u,296u,2006u}){
  Fixture f(std::nullopt);if(skill==312)check(f.w.gale(f.source,f.target,12,{},100));
  if(skill!=312)check(f.w.attack_activity(f.target,3000,200));
  check(!f.cast(skill));check(f.w.resources(2)->health==500&&f.w.resources(1)->energy==50);
  check(f.w.knocked_down(f.target,1000)==(skill==312));
 }
 for(bool attacking:{false,true})for(bool allowed:{false,true}){
  Fixture f(allowed);if(attacking)check(f.w.attack_activity(f.target,3000,200));check(f.cast(2006));
  const auto plan=*effects::holy_damage(2006,12);
  check(f.w.resources(1)->energy==50+(attacking?plan.energy_if_attacking:0));
  check(f.w.resources(2)->health==500-(plan.damage-5)+seed.healing);
 }
 for(bool allowed:{false,true}){
  Fixture f(allowed);check(f.w.attack_activity(f.target,3000,200));check(f.cast(296));
  check(f.w.knocked_down(f.target,1000));
  check(f.w.resources(2)->health==500-(effects::holy_damage(296,12)->damage-5)+seed.healing);
 }
 for(unsigned skill:{294u,2887u}){Fixture f(true);check(f.cast(skill));check(f.w.resources(2)->health==500-(effects::holy_damage(skill,12)->damage-5)+seed.healing);}
 {
  Fixture f(std::nullopt);check(f.cast(1689));check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500);
  // Zero damage must not require Seed recipient eligibility.1399 still
  // receives the contact; do not assume zero contacts vanish.
 }
 {
  Fixture f(true);check(f.w.status_mutation(f.source,1399,900,12,100,6100,true));
  check(f.cast(1689));const auto plan=*effects::holy_damage(1689,12,1);
  check(f.w.resources(2)->health==500-(plan.damage-5)+seed.healing);
 }
 // Exercise the real paid Runtime envelope, not just World callbacks.
 {
  Fixture f(true,low);check(f.w.gale(f.source,f.target,12,{},100));
  skills::casting::Hooks h;unsigned pays=0,commits=0;std::function<void()> copied;
  h.resources=[&](auto id){return f.w.resources(id);};
  h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.projectile_target_valid=[](auto id,float range){return id==2&&range==144;};
  h.consume_energy_at=[&](unsigned amount,auto at){++pays;check(amount==5);return f.w.consume_energy(1,amount,f.source,at);};
  h.holy_damage=[&](auto target,const auto& plan,auto at)->skills::casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool ok=f.w.holy_damage(f.source,target,plan,20,at,&visuals,&publication);
   copied=publication;return {ok,std::move(visuals),{},std::move(publication),true};
  };
  skills::Slots bar{};bar[0]={312,0};skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[312]=1;
  attributes::Ranks ranks{};ranks[14]=12;skills::casting::Runtime runtime(1);f.box->drain();
  check(runtime.start(1000,{312,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
  runtime.update(1749,h);check(commits==0&&pays==1);runtime.update(1750,h);check(commits==1);
  check(f.w.resources(1)->energy==45&&f.w.resources(2)->health==unsigned(low-first-second+2*seed.healing));
  check(f.w.resources(3)->health==500+2*seed.healing);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500+2*seed.healing);
  const auto terminal=skills::casting::terminal(58,1),body=*skills::particles::targeted_body_2022(2,1,312);
  const auto damage=combat::detail::integer(10,1,312),heal=skills::particles::health_change_2022(2,3,seed.healing,1000);
  std::vector<unsigned> order;
  for(const auto& packet:f.box->drain()){
   if(packet==terminal)order.push_back(0);else if(packet==body)order.push_back(1);
   else if(packet==damage)order.push_back(2);else if(packet==heal)order.push_back(3);
  }
  check(order==std::vector<unsigned>({0,1,2,3,2,3}));
  check(bool(copied));auto second_copy=copied;copied();second_copy();runtime.update(1750,h);
  check(commits==1&&f.box->drain().empty());
 }
 std::cout<<"PASS holy family Seed ordered hits, atomic refusal, control and conditional Energy\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
