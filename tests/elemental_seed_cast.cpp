// Detached paid elemental publication/Seed regression.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Elemental Seed cast line "+std::to_string(at.line()));
}
int main()try{
 const auto seed=*effects::healing_seed(12);
 for(unsigned skill:{189u,1068u})for(int eligibility:{-1,0,1})for(bool production:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=250;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;
  d.healing_allowed=eligibility<0?std::optional<bool>{}:std::optional(bool(eligibility));
  if(eligibility>=0)d.healing_evidence="synthetic paid elemental Seed";
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:id==2?100:260,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?100u:500u,1000,80,100},100);
   check(w.energy_capacity(id,skill==1068?4:6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),60,60));
  }
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3),ally=*w.effect_scope(100);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,seed,0,100));
  check(w.status_mutation(target,1399,800,12,100,6100,true));
  const auto* row=find_skill(skill);check(row);const auto plan=*effects::elemental(skill,12);
  casting::Hooks hooks;unsigned payments=0,commits=0;std::function<void()> copied;bool instant=false;
  hooks.resources=[&](auto id){return w.resources(id);};hooks.events=[&](const auto& packets){box->enqueue(packets);};
  hooks.target_scope=[&](auto id){return w.effect_scope(id);};hooks.projectile_target_valid=[](auto id,float range){return id==2&&range==1248;};
  hooks.consume_energy_at=[&](unsigned amount,auto at){++payments;check(amount==row->energy_cost());return w.consume_energy(1,amount,source,at);};
  hooks.elemental=[&](auto selected,const auto& effect,auto at)->casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool accepted=w.elemental(source,selected,effect,20,at,&visuals,nullptr,nullptr,{},&publication);
   copied=publication;return {accepted,std::move(visuals),{},std::move(publication),true};
  };
  hooks.conditional_elemental=[&](auto selected,const auto& effect,auto at)->std::optional<casting::ConditionalElementalCompletion>{
   ++commits;effects::ElementalOutcome outcome;std::vector<world::Packet> visuals;std::function<void()> publication;
   if(!w.elemental(source,selected,effect,20,at,&visuals,nullptr,&outcome,{},&publication))return {};
   instant=outcome.instant_recharge;copied=publication;
   return casting::ConditionalElementalCompletion{outcome,std::move(visuals),std::move(publication),true};
  };
  unsigned spell_events=0;
  if(production){
   hooks.before_spell_effect=[&](unsigned id,std::uint64_t event,auto at){
    ++spell_events;check(id==skill&&event);return w.before_spell_effect(source,id,event,at);
   };
   hooks.elemental_completed=[&](auto selected,const auto& effect,std::uint64_t event,auto at)->std::optional<casting::ConditionalElementalCompletion>{
    ++commits;effects::ElementalOutcome outcome;std::vector<world::Packet> visuals;std::function<void()> publication;
    if(!w.elemental(source,selected,effect,20,at,&visuals,nullptr,&outcome,event,&publication))return {};
    instant=outcome.instant_recharge;copied=publication;
    return casting::ConditionalElementalCompletion{outcome,std::move(visuals),std::move(publication),true};
   };
  }
  Slots bar{};bar[0]={skill,0};Policy policy;policy.primary=skill==1068?4:6;policy.campaigns=255;policy.learned[skill]=1;
  attributes::Ranks ranks{};ranks[row->attribute]=12;casting::Runtime runtime(1);box->drain();
  check(runtime.start(1000,{skill,0,2},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&payments==1);
  const unsigned completion=1000+unsigned(row->activation*1000);
  runtime.update(completion-1,hooks);check(commits==0);runtime.update(completion,hooks);check(commits==1);
  check(spell_events==unsigned(production));
  const bool accepted=eligibility>=0;
  check(w.resources(1)->energy==80-row->energy_cost());
  check(w.resources(2)->health==unsigned(500-(accepted?plan.damage-5:0)+(accepted?seed.healing:0)));
  //189's healer is also an area foe; it receives the target's Seed before
  // its own unprotected damage.1068 has only the selected recipient.
  check(w.resources(3)->health==unsigned(500+(accepted?seed.healing:0)-(accepted&&skill==189?plan.damage:0)));
  check(w.npc_combat_snapshot(ally)->health->current==500+(accepted&&eligibility==1?seed.healing:0));
  check(instant==(accepted&&skill==1068));
  const auto terminal=casting::terminal(58,1),body=*particles::targeted_body_2022(2,1,skill);
  const auto damage=combat::detail::integer(10,1,skill),healing=particles::health_change_2022(2,3,seed.healing,1000);
  std::vector<unsigned> order;
  for(const auto& p:box->drain()){if(p==terminal)order.push_back(0);else if(p==body)order.push_back(1);else if(p==damage)order.push_back(2);else if(p==healing)order.push_back(3);}
  std::vector<unsigned> expected;if(accepted){expected={0,1,2,3};if(skill==189)expected.push_back(2);}
  check(order==expected);
  if(accepted)check(bool(copied));else check(!copied);
  auto other=copied;if(copied)copied();if(other)other();runtime.update(completion,hooks);check(commits==1&&box->drain().empty());
 }
 std::cout<<"PASS paid elemental complete feedback capture, Seed refusal and conditional outcome\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
