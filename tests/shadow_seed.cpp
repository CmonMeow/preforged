#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace e=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("shadow Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();e::Scope source,target,monk,ally;
 Fixture(int allowed,unsigned health){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=110;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};
  d.healing_allowed=allowed<0?std::optional<bool>{}:std::optional(bool(allowed));if(allowed>=0)d.healing_evidence="synthetic shadow Seed";
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==2?health:500u,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(2);monk=*w.effect_scope(3);ally=*w.effect_scope(100);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));check(w.complete_healing_seed(monk,target,*e::healing_seed(12),0,100));
 }
};
int main()try{
 const auto seed=*e::healing_seed(12);
 for(unsigned skill:{133u,219u,155u,158u,1078u,102u,1067u})for(int allowed:{-1,0,1})
 for(unsigned hp:{500u,600u})for(bool absorption:{false,true})for(bool reversal:{false,true}){
  Fixture f(allowed,hp);const auto plan=*e::shadow_damage(skill,12);
  if(absorption)check(f.w.status_mutation(f.target,1399,800,12,100,6100,true));
  if(reversal)check(f.w.status_mutation(f.target,307,801,20,100,8100,true));
  const double incoming=std::max(0.,double(plan.damage)-(absorption?5:0));
  const double converted=reversal?std::min(incoming,double(e::reversal_fortune(20)->maximum_healing)):0;
  const double loss=incoming-converted;const bool trigger=loss>0;const bool accepted=allowed>=0||!trigger;
  const double steal=hp>500?plan.steal_above_half:0;
  f.box->drain();check(f.w.shadow_damage(f.source,f.target,plan,20,100)==accepted);
  check(f.w.resources(2)->health==(accepted?hp+converted-loss+(trigger?seed.healing:0)-steal:hp));
  check(f.w.resources(1)->health==500+(accepted?steal:0));
  check(f.w.resources(3)->health==500+(accepted&&trigger?seed.healing:0));
  check(f.w.npc_combat_snapshot(f.ally)->health->current==500+(accepted&&trigger&&allowed==1?seed.healing:0));
  if(!accepted){const auto entries=*f.w.status_snapshot(f.target,100);
   check(!absorption||std::any_of(entries.begin(),entries.end(),[](const auto& x){return x.skill==1399;}));}
 }
 for(unsigned skill:{102u,155u}){
  Fixture f(1,600);skills::casting::Hooks h;unsigned commits=0;std::function<void()> copied;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.events=[&](const auto& packets){f.box->enqueue(packets);};h.projectile_target_valid=[](auto id,float){return id==2;};
  h.consume_energy_at=[&](unsigned amount,auto now){return f.w.consume_energy(1,amount,f.source,now);};
  h.shadow_damage=[&](auto target,const auto& plan,auto now)->skills::casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool ok=f.w.shadow_damage(f.source,target,plan,20,now,&visuals,&publication);copied=publication;
   return {ok,std::move(visuals),{},std::move(publication),true};
  };
  const auto* row=skills::find_skill(skill);check(row);skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;
  skills::Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[row->attribute]=12;skills::casting::Runtime runtime(1);f.box->drain();
  check(runtime.start(1000,{skill,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
  const unsigned end=1000+unsigned(row->activation*1000);runtime.update(end-1,h);check(!commits);runtime.update(end,h);check(commits==1);
  const auto packets=f.box->drain();const auto finish=skills::casting::terminal(58,1),hit=combat::detail::integer(10,1,skill);
  // Vile Touch has caster-body279 and no target-body resource in the pinned
  // client. Shadow Strike has target-body192 and no caster-body resource.
  const auto body_effect=skill==155?skills::casting::success_body_effect(1,skill):skills::particles::target_body_2022(2,skill);
  check(body_effect.has_value());const auto body=*body_effect,heal=skills::particles::health_change_2022(2,3,seed.healing,1000);
  const auto at=[&](const auto& packet){return std::find(packets.begin(),packets.end(),packet);};
  check(at(finish)!=packets.end()&&at(body)!=packets.end()&&at(hit)!=packets.end()&&at(heal)!=packets.end());
  check(at(finish)<at(body)&&at(body)<at(hit)&&at(hit)<at(heal));check(bool(copied));auto other=copied;copied();other();check(f.box->drain().empty());
 }
 std::cout<<"PASS shadow direct damage/Seed/conversion/conditional life steal and paid publication\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}
