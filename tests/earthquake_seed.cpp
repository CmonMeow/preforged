#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earthquake Seed line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned offset){return unsigned(p[offset])|(unsigned(p[offset+1])<<8)|(unsigned(p[offset+2])<<16)|(unsigned(p[offset+3])<<24);}
struct Fixture{
 world::Instance w;effects::Scope source,early,later,monk,ally,enemy;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned hp){
  std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?910:700;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.spawn_health={500,1000};d.damageable=true;
   d.relation=id==100?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
   if(id==100){d.healing_allowed=allowed;if(allowed.has_value())d.healing_evidence="synthetic Earthquake Seed";}
   else{d.earth_armor=60;d.knockdown_allowed=true;d.knockdown_evidence="synthetic";}
   n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u,4u}){
   w.join(id,"Actor",id==1?0:id==2?600:id==3?760:920,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==3?hp:500u,1000,80,100},100);check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.combat_armor(*w.effect_scope(id),60));
  }
  source=*w.effect_scope(1);early=*w.effect_scope(2);later=*w.effect_scope(3);monk=*w.effect_scope(4);ally=*w.effect_scope(100);enemy=*w.effect_scope(101);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  for(auto target:{early,later}){check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));check(w.status_mutation(target,1399,800,12,100,6100,true));}
 }
};
int main()try{
 const auto seed=*effects::healing_seed(12);
 for(unsigned skill:{170u,1086u})for(int eligibility:{-1,0,1})for(bool paid:{false,true}){
  const auto plan=*effects::earthquake(skill,12);const double loss=plan.damage-5.;const unsigned low=unsigned(std::ceil(loss-seed.healing+1));
  Fixture f(eligibility<0?std::optional<bool>{}:std::optional(bool(eligibility)),low);f.box->drain();
  std::function<void()> copied;bool accepted=false;unsigned commits=0,payments=0;
  if(paid){
   skills::casting::Hooks h;h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& batch){f.box->enqueue(batch);};
   h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[](auto id,float range){return id==2&&range==1248;};
   h.consume_overcast_energy=[&](auto id,unsigned amount,auto event,auto at){++payments;check(id==skill&&amount==25);return f.w.activate_overcast_energy(f.source,id,amount,event,at);};
   h.before_spell_effect=[&](unsigned id,auto event,auto at){return f.w.before_spell_effect(f.source,id,event,at);};
   h.elemental_completed=[&](auto target,const auto& elemental,auto event,auto at)->std::optional<skills::casting::ConditionalElementalCompletion>{
    ++commits;effects::ElementalOutcome outcome;std::vector<world::Packet> visuals;std::function<void()> publication;
    accepted=f.w.elemental(f.source,target,elemental,20,at,&visuals,nullptr,&outcome,event,&publication);
    copied=publication;if(!accepted)return {};return skills::casting::ConditionalElementalCompletion{outcome,std::move(visuals),std::move(publication),true};
   };
   skills::Slots bar{};bar[0]={skill,0};skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[skill]=1;
   attributes::Ranks ranks{};ranks[9]=12;skills::casting::Runtime runtime(1);
   check(runtime.start(1000,{skill,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
   runtime.update(3999,h);check(!commits);runtime.update(4000,h);check(commits==1&&payments==1&&f.w.resources(1)->energy==55);
  }else{
   std::vector<world::Packet> visuals;
   accepted=f.w.earthquake(f.source,f.early,plan,20,1000,&visuals,{},&copied);
   if(accepted){check(visuals.empty()&&f.box->drain().empty());copied();}
  }
  check(accepted==(eligibility>=0));
  if(!accepted){
   check(f.w.resources(2)->health==500&&f.w.resources(3)->health==low&&f.w.npc_combat_snapshot(f.enemy)->health->current==500);
   check(!f.w.knocked_down(f.early,4000)&&!f.w.knocked_down(f.later,4000));
   for(const auto& packet:f.box->drain()){
    check(packet!=combat::detail::integer(10,1,skill));check(packet!=skills::casting::terminal(58,1));
    check(!(packet.size()==18&&packet[0]==159&&word(packet,2)==20&&word(packet,14)==304));
   }
   const auto burn=*effects::energy_drain(42,12);const unsigned now=paid?4000:1000;
   check(f.w.before_spell_effect(f.source,42,100,now));check(f.w.energy_drain(f.source,f.early,burn,20,now,nullptr,100));
   check(f.w.resources(2)->health==unsigned(std::ceil(500-(burn.amount*burn.damage_per_energy-5)+seed.healing)));
   continue;
  }
  check(f.w.resources(2)->health==unsigned(std::ceil(500-loss+2*seed.healing)));
  check(f.w.resources(3)->health==unsigned(std::ceil(low-loss+2*seed.healing)));
  check(f.w.resources(4)->health==500+seed.healing);
  check(f.w.npc_combat_snapshot(f.ally)->health->current==500+(eligibility==1?seed.healing:0));
  check(std::abs(f.w.npc_combat_snapshot(f.enemy)->health->current-(500-plan.damage))<1e-8);
  const unsigned now=paid?4000:1000;check(f.w.knocked_down(f.early,now)&&f.w.knocked_down(f.later,now)&&f.w.knocked_down(f.enemy,now));
  const auto hit=combat::detail::integer(10,1,skill),heal=skills::particles::health_change_2022(3,4,seed.healing,1000);
  std::vector<unsigned> order;
  for(const auto& p:f.box->drain()){
   if(paid&&p==skills::casting::terminal(58,1))order.push_back(8);
   else if(paid&&p==skills::casting::generic(21,1,303))order.push_back(9);
   else if(p.size()==18&&p[0]==159&&word(p,2)==20&&word(p,14)==304)order.push_back(word(p,6)==2?0:word(p,6)==3?3:4);
   else if(p==hit)order.push_back(1);else if(p==heal)order.push_back(2);
  }
  std::vector<unsigned> expected;if(paid)expected={8,9};for(auto n:{0u,1u,2u,3u,1u,2u,4u,1u})expected.push_back(n);
  check(order==expected);check(bool(copied));auto copy=copied;copied();copy();check(f.box->drain().empty());
 }
 std::cout<<"PASS Earthquake whole-cohort Seed admission, survival/control and paid publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
