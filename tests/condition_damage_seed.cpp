// Detached generic elemental cohort/Seed admission regressions.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Condition Seed line "+std::to_string(at.line()));
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
   if(id==100){d.healing_allowed=allowed;if(allowed.has_value())d.healing_evidence="synthetic elemental Seed";
    if(!allowed.has_value()){d.creature_type="spirit";d.creature_type_evidence="authored unsupported Seed gain recipient";d.fleshy=false;d.fleshy_evidence="authored non-fleshy spirit";}
    if(allowed==std::optional(false)){combat::npcs::LiveActor immune(d,{1,1,1,146,id});const auto life=immune.lifetime();
     check(!immune.heal_ally(life,25,100)&&immune.actor(life)->health->current==500);}
   }
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
 bool cast(unsigned skill,effects::Scope target,std::uint64_t now=1000){
  std::vector<world::Packet> visuals;box->drain();return w.condition_damage(source,target,*effects::condition_damage(skill,12),20,now,&visuals);
 }
};
int main()try{
 const auto seed=*effects::healing_seed(12);const auto fragility=*effects::fragility(2998,12);
 const auto plan=*effects::condition_damage(864,12);const double direct=plan.damage-5.,first=std::max(0.,double(fragility.damage)-10.),second=std::max(0.,double(fragility.damage)-15.);
 for(int eligibility:{-1,0,1}){
  Fixture f(eligibility<0?std::optional<bool>{}:std::optional(bool(eligibility)),1);
  for(auto target:{f.early,f.later,f.enemy})check(f.w.apply_condition(f.source,target,combat::conditions::Kind::blind,100,10000));
  check(f.w.degeneration_hex(f.source,f.later,*effects::degeneration_hex(2998,12),100));
  check(f.cast(864,f.early)==(eligibility>=0));
  if(eligibility<0){
   check(f.w.resources(2)->health==500&&f.w.resources(3)->health==1&&f.w.npc_combat_snapshot(f.enemy)->health->current==500);
   check(!f.w.has_condition(f.early,combat::conditions::Kind::poison,1000));
   const auto burn=*effects::energy_drain(42,12);check(f.w.before_spell_effect(f.source,42,1,1000));
   check(f.w.energy_drain(f.source,f.early,burn,20,1000,nullptr,1));
   check(f.w.resources(2)->health==500-(burn.amount*burn.damage_per_energy-5)+seed.healing);continue;
  }
  check(f.w.resources(2)->health==unsigned(std::ceil(500-direct+4*seed.healing)));
  check(f.w.resources(3)->health==unsigned(std::ceil(1-direct-first-second+4*seed.healing)));
  check(f.w.resources(4)->health==500+3*seed.healing);
  check(f.w.npc_combat_snapshot(f.ally)->health->current==500+3*seed.healing);
  check(f.w.npc_combat_snapshot(f.enemy)->health->current==500-plan.damage);
  for(auto target:{f.early,f.later,f.enemy})for(auto kind:{combat::conditions::Kind::poison,combat::conditions::Kind::weakness})check(f.w.has_condition(target,kind,1000));
 }
 for(unsigned skill:{828u,864u}){
  Fixture f(true,500,true);check(f.w.apply_condition(f.source,f.early,combat::conditions::Kind::blind,100,10000));
  skills::casting::Hooks h;std::function<void()> copied;unsigned commits=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.events=[&](const auto& packets){f.box->enqueue(packets);};h.projectile_target_valid=[](auto id,float range){return id==2&&range==1248;};
  h.consume_energy_at=[&](unsigned amount,auto now){return f.w.consume_energy(1,amount,f.source,now);};
  h.condition_damage=[&](auto target,const auto& plan,auto now)->skills::casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool ok=f.w.condition_damage(f.source,target,plan,20,now,&visuals,&publication);copied=publication;
   return {ok,std::move(visuals),{},std::move(publication),true};
  };
  const auto* row=skills::find_skill(skill);check(row);skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;
  skills::Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[row->attribute]=12;skills::casting::Runtime runtime(1);f.box->drain();
  check(runtime.start(1000,{skill,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
  const unsigned end=1000+unsigned(row->activation*1000);runtime.update(end-1,h);check(!commits);runtime.update(end,h);check(commits==1);
  const auto packets=f.box->drain();const auto finish=skills::casting::terminal(58,1),hit=combat::detail::integer(10,1,skill);
  const auto body=*skills::particles::target_body_2022(2,skill),heal=skills::particles::health_change_2022(2,4,seed.healing,1000);
  const auto at=[&](const auto& packet){return std::find(packets.begin(),packets.end(),packet);};
  check(at(finish)!=packets.end()&&at(body)!=packets.end()&&at(hit)!=packets.end()&&at(heal)!=packets.end());
  check(at(finish)<at(body)&&at(body)<at(hit)&&at(hit)<at(heal));check(bool(copied));auto other=copied;copied();other();check(f.box->drain().empty());
 }
 // A selected Hex Breaker retaliation is part of cast admission too.
 for(int eligibility:{-1,0,1}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();world::Instance::Npc n;n.id=n.definition.id=100;n.x=-100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;
  d.spawn_health={500,1000};d.damageable=true;d.healing_allowed=eligibility<0?std::optional<bool>{}:std::optional(bool(eligibility));
  if(eligibility>=0)d.healing_evidence="synthetic HexBreaker Seed";
  else{d.creature_type="spirit";d.creature_type_evidence="authored unsupported Seed gain recipient";d.fleshy=false;d.fleshy_evidence="authored non-fleshy spirit";}
  if(eligibility==0){combat::npcs::LiveActor immune(d,{1,1,1,146,100});const auto life=immune.lifetime();
   check(!immune.heal_ally(life,25,100)&&immune.actor(life)->health->current==500);}
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?100:-60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);
   w.set_vitals(id,{500,1000,80,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3),ally=*w.effect_scope(100);
  check(w.party_action(1,158,3,100)&&w.party_action(3,154,1,100));check(w.complete_healing_seed(monk,source,seed,0,100));
  check(w.apply_condition(source,target,combat::conditions::Kind::blind,100,10000));check(w.hex_breaker(target,*effects::hex_breaker(12),20,100));
  for(auto owner:{source,target})check(w.status_mutation(owner,1399,800,12,100,6100,true));
  check(w.condition_damage(source,target,*effects::condition_damage(828,12),20,1000)==(eligibility>=0));
  if(eligibility<0){check(w.resources(1)->health==500&&w.resources(2)->health==500);continue;}
  check(w.resources(1)->health==500-(effects::hex_breaker(12)->damage-5)+seed.healing);
  check(w.resources(2)->health==500-(effects::condition_damage(828,12)->damage-5));check(w.resources(3)->health==500+seed.healing);
  check(w.npc_combat_snapshot(ally)->health->current==500+seed.healing);
  const auto entries=*w.status_snapshot(target,1000);check(std::none_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==828||e.skill==10;}));
 }
 std::cout<<"PASS conditional damage Seed cohort, followup Fragility, paid order and HexBreaker admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
