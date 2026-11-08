// Detached paid Runtime regression for direct Necrosis/Energy Blast feedback.
#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Direct skill Seed cast line "+std::to_string(at.line()));
}
int main()try{
 const auto seed=*effects::healing_seed(12);
 for(unsigned skill:{2103u,2193u})for(int eligibility:{-1,0,1})for(bool damaging:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=110;
  n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition definition;
  definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health={500,1000};
  definition.damageable=true;
  definition.healing_allowed=eligibility<0?std::optional<bool>{}:std::optional(bool(eligibility));
  if(eligibility>=0)definition.healing_evidence="synthetic paid direct Seed fixture";
  n.combat_definition=definition;w.configure_npcs({n});
  const auto* row=find_skill(skill);check(row);
  const unsigned starting_energy=skill==2193?(damaging?30u:10u):100u;
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:500u,1000,id==1?starting_energy:100u,100},100);
  }
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3),npc=*w.effect_scope(100);
  check(w.energy_capacity(1,6,0,100,{80,-2})); // Fixed zero natural Energy pips.
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,seed,0,100));
  if(skill==2103&&damaging)check(w.movement_hex(source,target,*effects::movement_hex(210,12),500));
  casting::Hooks hooks;unsigned pays=0,commits=0;std::function<void()> copied_publication;
  hooks.resources=[&](auto id){return w.resources(id);};
  hooks.events=[&](const auto& packets){box->enqueue(packets);};
  hooks.consume_energy_at=[&](unsigned amount,auto at){++pays;check(amount==row->energy_cost());return w.consume_energy(1,amount,source,at);};
  hooks.target_scope=[&](auto id){return w.effect_scope(id);};
  hooks.projectile_target_valid=[](auto id,float range){return id==2&&range==1248;};
  hooks.sunspear_rank=[](){return std::optional<unsigned>{5};};
  hooks.necrosis=[&](auto selected,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool accepted=w.necrosis(source,selected,plan,20,at,&visuals,&publication);
   copied_publication=publication;
   return {accepted,std::move(visuals),{},std::move(publication),true};
  };
  hooks.energy_blast=[&](auto selected,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool accepted=w.energy_blast(source,selected,plan,20,at,&visuals,&publication);
   copied_publication=publication;
   return {accepted,std::move(visuals),{},std::move(publication),true};
  };
  Slots bar{};bar[0]={skill,0};Policy policy;policy.primary=skill==2103?4:6;
  policy.campaigns=255;policy.learned[skill]=1;attributes::Ranks ranks{};ranks[12]=12;
  casting::Runtime runtime(1);box->drain();
  check(runtime.start(1000,{skill,0,2},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
  check(pays==1&&w.resources(1)->energy==starting_energy-row->energy_cost());
  const unsigned completion=1000+unsigned(row->activation*1000);
  runtime.update(completion-1,hooks);check(commits==0);runtime.update(completion,hooks);check(commits==1);
  const bool accepted=!damaging||eligibility>=0;
  const unsigned raw=skill==2103?(damaging?effects::necrosis(5)->damage:0u):
   std::min(effects::energy_blast(12)->maximum_damage,w.resources(1)->energy*effects::energy_blast(12)->damage_per_energy);
  const unsigned applied=accepted?raw:0;
  check(w.resources(2)->health==500-applied+(applied?seed.healing:0));
  check(w.resources(3)->health==500+(applied?seed.healing:0));
  check(w.npc_combat_snapshot(npc)->health->current==500+(applied&&eligibility==1?seed.healing:0));
  const auto packets=box->drain();
  const auto finish=casting::terminal(58,1),body=*particles::targeted_body_2022(2,1,skill);
  const auto damage=combat::detail::integer(10,1,skill);
  const auto healing=particles::health_change_2022(3,3,seed.healing,1000);
  std::vector<unsigned> order;
  for(const auto& packet:packets){
   if(packet==finish)order.push_back(1);else if(packet==body)order.push_back(2);
   else if(packet==damage)order.push_back(3);else if(packet==healing)order.push_back(4);
  }
  check(order==(accepted?(applied?std::vector<unsigned>({1,2,3,4}):std::vector<unsigned>({1,2})):std::vector<unsigned>{}));
  if(applied)check(bool(copied_publication));
  // Copy BEFORE first invocation occurs inside Runtime; all copies must share
  // one publication guard rather than each holding its own replayable queue.
  auto second_copy=copied_publication;
  if(copied_publication)copied_publication();if(second_copy)second_copy();
  runtime.update(completion,hooks);check(commits==1&&box->drain().empty());
 }
 std::cout<<"PASS paid Necrosis/Energy Blast Seed, no-effect/refusal ordering and copied publication replay guards\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
