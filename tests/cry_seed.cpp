// Cry whole-cohort protected damage/Seed and paid publication regressions.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Cry Seed line "+std::to_string(at.line()));
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
   if(id==100){d.healing_allowed=allowed;if(allowed.has_value())d.healing_evidence="synthetic elemental Seed";}
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
};
int main()try{
 const auto seed=*effects::healing_seed(12);const auto plan=*effects::cry_frustration(12);
 for(int allowed:{-1,0,1})for(unsigned protection:{0u,16u,17u}){
  Fixture f(allowed<0?std::optional<bool>{}:std::optional(bool(allowed)),40);
  check(f.w.cast_activity(f.early,1,100,10000)&&f.w.cast_activity(f.later,28,100,10000));
  if(protection)for(auto target:{f.early,f.later})check(f.w.status_mutation(target,protection,900+target.actor,15,100,10000,true));
  const auto early_epoch=f.w.control_epoch(f.early),later_epoch=f.w.control_epoch(f.later);
  const auto early_energy=f.w.resources(2)->energy,later_energy=f.w.resources(3)->energy;
  check(f.w.cry_frustration(f.source,f.early,plan,20,1000)==(allowed>=0));
  if(allowed<0){
   check(f.w.resources(2)->health==500&&f.w.resources(3)->health==40&&f.w.resources(4)->health==500);
   check(f.w.resources(2)->energy==early_energy&&f.w.resources(3)->energy==later_energy);
   check(f.w.control_epoch(f.early)==early_epoch&&f.w.control_epoch(f.later)==later_epoch);
   check(f.w.casting_skill(f.early,1000)==1&&f.w.casting_skill(f.later,1000)==28);
   // An isolated later hit proves the refused whole cohort did not spend the
   // first recipient's Shield of Absorption hit counter.
   const auto burn=*effects::energy_drain(42,12);check(f.w.before_spell_effect(f.source,42,1,1000));
   check(f.w.energy_drain(f.source,f.early,burn,20,1000,nullptr,1));
   check(f.w.resources(2)->health==500-(burn.amount*burn.damage_per_energy-5)+seed.healing);continue;
  }
  const unsigned damage=plan.damage-5;
  check(f.w.resources(2)->health==500-damage+2*seed.healing);
  check(f.w.resources(3)->health==40+2*seed.healing-damage);
  check(f.w.resources(4)->health==500-plan.damage+seed.healing);
  check(f.w.npc_combat_snapshot(f.ally)->health->current==500+(allowed==1?seed.healing:0));
  check(f.w.npc_combat_snapshot(f.enemy)->health->current==500-plan.damage);
  check(bool(f.w.casting_skill(f.early,1000))==bool(protection));
  check(bool(f.w.casting_skill(f.later,1000))==bool(protection));
  check(f.w.resources(2)->energy==early_energy-(protection==17?4u:0u));
 }
 for(int allowed:{-1,1}){
  Fixture f(allowed<0?std::optional<bool>{}:std::optional(true),500);
  check(f.w.cast_activity(f.early,1,100,10000));skills::casting::Hooks h;std::function<void()> copied;unsigned commits=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.events=[&](const auto& packets){f.box->enqueue(packets);};h.projectile_target_valid=[](auto id,float range){return id==2&&range==1248;};
  h.consume_energy_at=[&](unsigned amount,auto now){return f.w.consume_energy(1,amount,f.source,now);};
  h.before_spell_effect=[&](unsigned id,auto event,auto now){return f.w.before_spell_effect(f.source,id,event,now);};
  h.cry_frustration=[&](auto target,const auto& p,auto now)->skills::casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool ok=f.w.cry_frustration(f.source,target,p,20,now,&visuals,&publication);copied=publication;
   return {ok,std::move(visuals),{},std::move(publication),true};
  };
  skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[57]=1;
  skills::Slots bar{};bar[0]={57,0};attributes::Ranks ranks{};ranks[2]=12;skills::casting::Runtime runtime(1);f.box->drain();
  const auto starting_energy=f.w.resources(1)->energy;
  check(runtime.start(1000,{57,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
  runtime.update(1249,h);check(!commits);runtime.update(1250,h);check(commits==1&&f.w.resources(1)->energy==starting_energy-10);
  const auto packets=f.box->drain();const auto finish=skills::casting::terminal(58,1),hit=combat::detail::integer(10,1,57);
  const auto body=*skills::particles::targeted_body_2022(2,1,57),heal=skills::particles::health_change_2022(2,4,seed.healing,1000);
  const auto at=[&](const auto& p){return std::find(packets.begin(),packets.end(),p);};
  if(allowed<0){check(!copied&&at(finish)==packets.end()&&at(hit)==packets.end()&&at(body)==packets.end());continue;}
  check(at(finish)!=packets.end()&&at(body)!=packets.end()&&at(hit)!=packets.end()&&at(heal)!=packets.end());
  check(std::count(packets.begin(),packets.end(),body)==1&&std::count(packets.begin(),packets.end(),finish)==1);
  check(at(finish)<at(body)&&at(body)<at(hit)&&at(hit)<at(heal));check(bool(copied));auto other=copied;copied();other();check(f.box->drain().empty());
 }
 std::cout<<"PASS Cry Seed whole cohort, control/protection refusal, survival and paid publication\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}
