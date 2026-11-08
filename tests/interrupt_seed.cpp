// // Shared interrupt damage/Seed admission and paid publication.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
std::string case_context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Interrupt Seed line "+std::to_string(at.line())+" "+case_context);
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
 check(effects::is_spell(*skills::find_skill(82))&&!effects::is_spell(*skills::find_skill(155)));
 const auto seed=*effects::healing_seed(12);
 for(unsigned skill:{23u,860u})for(int allowed:{-1,0,1})for(unsigned protection:{0u,16u,17u})
 for(int reversal:{-1,0,20})for(unsigned activity:{0u,1u,2u,3u}){
  Fixture f(allowed<0?std::optional<bool>{}:std::optional(bool(allowed)),500,false);
  case_context="skill="+std::to_string(skill)+" healing="+std::to_string(allowed)+" protection="+std::to_string(protection)+
      " reversal="+std::to_string(reversal)+" activity="+std::to_string(activity);
  const auto plan=*effects::interrupt_skill(skill,12);const unsigned casting=activity==1?82:155;
  if(activity)check(f.w.cast_activity(f.later,casting,100,10000));
  if(activity==3)check(f.w.degeneration_hex(f.source,f.later,*effects::degeneration_hex(31,0),100));
  if(protection)check(f.w.status_mutation(f.later,protection,903,15,100,10000,true));
  if(reversal>=0)check(f.w.status_mutation(f.later,307,904,unsigned(reversal),100,8100,true));
  const bool matches=activity==1||(skill==860&&activity==3);
  const double incoming=plan.damage-5,converted=reversal<0?0:std::min(incoming,double(effects::reversal_fortune(reversal)->maximum_healing));
  const double loss=incoming-converted;const bool trigger=matches&&loss>0,accepted=allowed>=0||!trigger;
  const auto epoch=f.w.control_epoch(f.later);const auto energy=f.w.resources(3)->energy;
  check(f.w.interrupt_skill(f.source,f.later,plan,20,100)==accepted);
  check(f.w.resources(3)->health==(accepted&&matches?500+converted-loss+(trigger?seed.healing:0):500));
  check(f.w.resources(2)->health==500+(accepted&&trigger?seed.healing:0));
  check(f.w.resources(4)->health==500+(accepted&&trigger?seed.healing:0));
  check(f.w.npc_combat_snapshot(f.ally)->health->current==500+(accepted&&trigger&&allowed==1?seed.healing:0));
  check(f.w.resources(3)->energy==energy-(accepted&&matches&&protection==17?4:0));
  const bool retained=activity&&(!matches||!accepted||protection);
  check(bool(f.w.casting_skill(f.later,100))==retained);
  if(!accepted){check(f.w.control_epoch(f.later)==epoch);
   if(reversal<0&&activity==1&&!protection){
    // Move the Seed owner out of the unknown NPC's healing radius, then
    // independently probe the first absorption charge after refused control.
    f.w.move(3,600,0,1000);f.w.tick(1100);const auto burn=*effects::energy_drain(42,12);
    check(f.w.before_spell_effect(f.source,42,1,1100));check(f.w.energy_drain(f.source,f.later,burn,20,1100,nullptr,1));
    check(f.w.resources(3)->health==500-(burn.amount*burn.damage_per_energy-5)+seed.healing);
   }
  }
 }
 for(unsigned skill:{23u,860u})for(int allowed:{-1,1}){
  Fixture f(allowed<0?std::optional<bool>{}:std::optional(true),500,false);
  case_context="paid skill="+std::to_string(skill)+" healing="+std::to_string(allowed);
  check(f.w.cast_activity(f.later,82,100,10000));skills::casting::Hooks h;std::function<void()> copied;unsigned commits=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.events=[&](const auto& packets){f.box->enqueue(packets);};h.projectile_target_valid=[](auto id,float range){return id==3&&range==1248;};
  h.consume_energy_at=[&](unsigned amount,auto now){return f.w.consume_energy(1,amount,f.source,now);};
  h.before_spell_effect=[&](unsigned id,auto event,auto now){return f.w.before_spell_effect(f.source,id,event,now);};
  h.interrupt_skill=[&](auto target,const auto& p,auto now)->skills::casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool ok=f.w.interrupt_skill(f.source,target,p,20,now,&visuals,&publication);copied=publication;
   return {ok,std::move(visuals),{},std::move(publication),true};
  };
  const auto* row=skills::find_skill(skill);check(row);skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[skill]=1;
  skills::Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[row->attribute]=12;skills::casting::Runtime runtime(1);f.box->drain();
  const auto energy=f.w.resources(1)->energy;
  check(runtime.start(1000,{skill,0,3},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
  const unsigned end=1000+unsigned(row->activation*1000);runtime.update(end-1,h);check(!commits);runtime.update(end,h);check(commits==1);
  check(f.w.resources(1)->energy==energy-row->energy_cost());
  const auto packets=f.box->drain();const auto finish=skills::casting::terminal(58,1),hit=combat::detail::integer(10,1,skill);
  const auto body=skills::particles::targeted_body_2022(3,1,skill);const auto heal=skills::particles::health_change_2022(3,4,seed.healing,1000);check(body.has_value());
  const auto at=[&](const auto& p){return std::find(packets.begin(),packets.end(),p);};
  if(allowed<0){check(!copied&&at(finish)==packets.end()&&at(hit)==packets.end()&&at(*body)==packets.end());continue;}
  check(at(finish)!=packets.end()&&at(*body)!=packets.end()&&at(hit)!=packets.end()&&at(heal)!=packets.end());
  check(at(finish)<at(*body)&&at(*body)<at(hit)&&at(hit)<at(heal));check(std::count(packets.begin(),packets.end(),*body)==1);
  check(bool(copied));auto other=copied;copied();other();check(f.box->drain().empty());
 }
 std::cout<<"PASS interrupt damage Seed eligibility, protection, conversion, activity and paid publication\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}
