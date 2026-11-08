#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC Reversal World line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope caster,target;
 Fixture(unsigned hp=10,std::optional<bool> permission=true){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.spawn_health=combat::Health{double(hp),1000};d.damageable=true;d.conditions_allowed=true;
  d.relation=combat::npcs::Relation::hostile_to_players;d.healing_allowed=permission;if(permission.has_value())d.healing_evidence="synthetic";
  d.spawn_energy=combat::npcs::Energy{20,40};d.energy_pips=0;
  n.combat_definition=d;w.configure_npcs({n});w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  caster=*w.effect_scope(1);target=*w.effect_scope(100);box->drain();
 }
 double health(){return w.npc_combat_snapshot(target)->health->current;}
 bool hit(unsigned amount=72,unsigned at=200){return w.npc_reversal_damage(caster,target,{combat::Kind::armor_ignoring,double(amount),20,42},at);}
};
int main()try{
 unsigned cases=0;
 for(unsigned hp:{1u,10u,500u,1000u})for(unsigned rank:{0u,12u,20u}){
  ++cases;Fixture f(hp);check(f.w.stage_npc_reversal(f.target,rank,100));f.box->drain();
  check(f.hit());const auto converted=std::min(72.,double(skills::effects::reversal_fortune(rank)->maximum_healing));
  check(f.health()==std::max(0.,std::min(1000.,hp+converted)-(72-converted)));
  const auto packets=f.box->drain();check(!packets.empty()&&packets.front()[0]==67);
  check(!f.hit()&&f.box->drain().empty());
  check(f.w.take_npc_deaths(f.caster).size()==unsigned(f.health()<=0));check(f.w.take_npc_deaths(f.caster).empty());
 }
 check(cases==12);
 unsigned drain_cases=0;
 for(unsigned skill:{39u,42u})for(unsigned hp:{1u,500u,1000u})for(unsigned rr:{0u,12u})for(unsigned er:{0u,181u,2860u}){
  ++drain_cases;Fixture f(hp);f.w.set_vitals(1,{100,1000,50,100},100);
  if(er){const auto r=*skills::effects::ether_renewal(er,15);check(f.w.status_mutation(f.caster,er,80,15,100,100+r.duration,true));}
  check(f.w.stage_npc_reversal(f.target,rr,100));const auto plan=*skills::effects::energy_drain(skill,12);
  check(f.w.before_spell_effect(f.caster,skill,1,100));
  check(f.w.energy_drain(f.caster,f.target,plan,20,100,nullptr,1));
  const double raw=std::min(20u,plan.amount)*plan.damage_per_energy,converted=std::min(raw,double(skills::effects::reversal_fortune(rr)->maximum_healing));
  const double expected=std::max(0.,std::min(1000.,hp+converted)-(raw-converted));
  check(f.health()==expected&&f.w.resources(1)->health==100+(er?20:0));
  check(f.w.resources(1)->energy==50+(er?skills::effects::ether_renewal(er,15)->energy_per_enchantment:0));
  check(f.w.npc_energy_snapshot(f.target)->current==20-std::min(20u,plan.amount));
  check(!f.w.energy_drain(f.caster,f.target,plan,20,100,nullptr,1)&&f.health()==expected);
 }
 check(drain_cases==36);
 for(unsigned skill:{39u,42u}){Fixture f(10);
  check(f.w.apply_condition(f.caster,f.caster,combat::conditions::Kind::deep_wound,100,10000));
  check(f.w.condition_transfer(f.caster,f.target,*skills::effects::condition_transfer(132,12),100));
  check(f.w.stage_npc_reversal(f.target,12,100));f.box->drain();
  const auto plan=*skills::effects::energy_drain(skill,12);
  check(f.w.energy_drain(f.caster,f.target,plan,20,100)&&f.health()==0);
  const auto packets=f.box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p[0]==162;}));
  check(f.w.take_npc_deaths(f.caster).size()==1);
 }


 {Fixture f(10);check(f.w.apply_condition(f.caster,f.caster,combat::conditions::Kind::deep_wound,100,10000));
  check(f.w.condition_transfer(f.caster,f.target,*skills::effects::condition_transfer(132,12),100));
  check(f.w.stage_npc_reversal(f.target,12,100));f.box->drain();
  std::optional<combat::npcs::LiveActor::ReversalDamage> receipt;
  check(f.w.npc_reversal_damage(f.caster,f.target,{combat::Kind::armor_ignoring,42,20,42},200,&receipt));
  check(receipt&&!receipt->damage&&f.health()==0);const auto packets=f.box->drain();
  check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p[0]==162;}));
  check(f.w.take_npc_deaths(f.caster).size()==1&&!f.hit());
 }

 for(auto permission:{std::optional<bool>{},std::optional<bool>{false}}){Fixture f(500,permission);check(!f.w.stage_npc_reversal(f.target,12,100)&&f.health()==500);}
 {Fixture f;check(f.w.stage_npc_reversal(f.target,12,100));f.box->drain();
  auto stale=f.caster;++stale.generation;check(!f.w.npc_reversal_damage(stale,f.target,{combat::Kind::armor_ignoring,72,20,42},200));
  check(!f.hit(0)&&f.health()==10);check(f.hit()&&f.health()==72);
 }
 {Fixture f;check(f.w.stage_npc_reversal(f.target,12,100));check(!f.hit(72,8100)&&f.health()==10);}
 {Fixture f;check(f.w.stage_npc_reversal(f.target,12,100));check(f.w.stage_npc_reversal(f.target,0,150));check(f.hit()&&f.health()==0);}
 {Fixture f(1000);check(f.w.stage_npc_reversal(f.target,12,100));f.w.tick(8200);check(!f.hit(72,8300)&&f.health()==1000);}
 unsigned area_cases=0;
 for(bool npc_target:{false,true})for(unsigned rr:{0u,12u})for(unsigned er:{0u,181u,2860u}){
  ++area_cases;world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id=100;id<=105;++id){world::Instance::Npc n;n.id=n.definition.id=id;
   n.x=id==101?70:id==102?312:id==103?313:60;n.plane=id==104?1:0;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.spawn_health={id==101?10.:id==102?1000.:500.,1000};d.damageable=true;d.conditions_allowed=true;
   d.relation=id==105?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
   d.healing_allowed=true;d.healing_evidence="synthetic mixed recipients";
   if(id==100){d.spawn_energy=combat::npcs::Energy{20,40};d.energy_pips=0;}
   n.combat_definition=d;npcs.push_back(n);
  }
  w.configure_npcs(npcs);
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?100u:500u,1000,id==1?50u:20u,100},100);}
  const auto source=*w.effect_scope(1),selected=*w.effect_scope(npc_target?100:2);
  for(unsigned id=100;id<=105;++id)check(w.stage_npc_reversal(*w.effect_scope(id),rr,100));
  std::map<unsigned,unsigned> before_effects;for(unsigned id=100;id<=105;++id)before_effects[id]=w.npc_combat_snapshot(*w.effect_scope(id))->effects;
  if(er){const auto p=*skills::effects::ether_renewal(er,15);check(w.status_mutation(source,er,80,15,100,100+p.duration,true));}
  const auto plan=*skills::effects::energy_drain(39,12);const double raw=std::min(20u,plan.amount)*9.,converted=std::min(raw,double(skills::effects::reversal_fortune(rr)->maximum_healing));
  check(w.before_spell_effect(source,39,1,100)&&w.energy_drain(source,selected,plan,20,100,nullptr,1));
  check(w.resources(1)->health==100+(er?20:0));
  check(w.resources(1)->energy==50+(er?skills::effects::ether_renewal(er,15)->energy_per_enchantment:0));
  check(w.resources(2)->health==500-raw&&w.resources(2)->energy==(npc_target?20:20-plan.amount));
  for(unsigned id=100;id<=105;++id){const auto target=*w.effect_scope(id);const auto a=*w.npc_combat_snapshot(target);
   const double initial=id==101?10:id==102?1000:500;
   const auto expected=id<=102?std::max(0.,std::min(1000.,initial+converted)-(raw-converted)):initial;
   check(a.health->current==expected);
   if(id<=102)check(a.effects!=before_effects[id]);else check(a.effects==before_effects[id]);
   if(id==100)check(w.npc_energy_snapshot(target)->current==(npc_target?20-plan.amount:20));
   else check(!w.npc_energy_snapshot(target));
  }
  check(!w.energy_drain(source,selected,plan,20,100,nullptr,1));
 }
 check(area_cases==12);
 std::cout<<"PASS12 NPC Reversal World combinations,one-use publication/death,permissions,stale scope,zero,expiry,replacement;paid307 remains unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
