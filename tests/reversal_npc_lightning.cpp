#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC lightning Reversal line "+std::to_string(at.line()));
}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{229u,1369u,1082u,224u,228u,865u,3396u,2059u,842u,1664u,232u})for(unsigned rr:{0u,12u})for(unsigned hp:{10u,500u,1000u}){
  ++cases;world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={double(hp),1000};
  d.damageable=true;d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
  d.lightning_armor=60;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);check(w.stage_npc_reversal(target,rr,100));box->drain();
  const auto plan=*skills::effects::lightning(skill,12);check(w.lightning(source,target,plan,20,100));if(plan.projectile)w.tick(1000);
  const double raw=plan.damage*std::exp2(60.*plan.penetration/100./40.),converted=std::min(raw,rr?67.:15.);
  const double expected=std::max(0.,std::min(1000.,hp+converted)-(raw-converted));
  const auto actor=*w.npc_combat_snapshot(target);
  if(std::abs(actor.health->current-expected)>1e-7)throw std::runtime_error("skill="+std::to_string(skill)+" HP="+std::to_string(actor.health->current)+" expected="+std::to_string(expected));
  for(auto [kind,duration]:std::array<std::pair<combat::conditions::Kind,std::uint64_t>,3>{{
   {combat::conditions::Kind::weakness,plan.weakness_ms},{combat::conditions::Kind::blind,plan.blind_ms},{combat::conditions::Kind::cracked_armor,plan.cracked_ms}}})
   check(w.has_condition(target,kind,plan.projectile?1000:100)==(expected>0&&duration>0));
  const auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67&&p[2]==100;})==1);
  check(w.take_npc_deaths(source).size()==unsigned(expected<=0));
 }
 unsigned chains=0;
 for(unsigned skill:{842u,1664u}){++chains;world::Instance w;std::vector<world::Instance::Npc> population;
  for(unsigned id=100;id<=103;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.x=60+(id-100)*20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.spawn_health={id==101?10.:id==102?1000.:500.,1000};
   d.damageable=true;d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
   if(id!=103)d.lightning_armor=60;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;population.push_back(n);}
  w.configure_npcs(population);auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  if(skill==842)check(w.activate_overcast_energy(source,866,10,1,100)); // Arc secondaries require Overcast.

  for(unsigned id=100;id<=103;++id)check(w.stage_npc_reversal(*w.effect_scope(id),id==101?0:12,100));box->drain();
  const auto plan=*skills::effects::lightning(skill,12);check(w.lightning(source,target,plan,20,100));
  for(unsigned id=100;id<=103;++id){const double hp=id==101?10:id==102?1000:500;
   const double raw=(id==100?plan.damage:plan.secondary_damage)*std::exp2(15./40.),converted=std::min(raw,id==101?15.:67.);
   const double expected=id==103?hp:std::max(0.,std::min(1000.,hp+converted)-(raw-converted));
   check(std::abs(w.npc_combat_snapshot(*w.effect_scope(id))->health->current-expected)<1e-7);
  }
  const auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67;})==3);
  check(w.take_npc_deaths(source).size()==1);
 }
 for(unsigned skill:{224u,228u}){world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={10,1000};
  d.damageable=true;d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
  d.lightning_armor=60;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  check(w.apply_condition(source,source,combat::conditions::Kind::deep_wound,100,10000));
  check(w.condition_transfer(source,target,*skills::effects::condition_transfer(132,12),100));check(w.stage_npc_reversal(target,12,100));box->drain();
  check(w.lightning(source,target,*skills::effects::lightning(skill,12),20,100));check(w.npc_combat_snapshot(target)->health->current==0);
  check(!w.has_condition(target,combat::conditions::Kind::weakness,100)&&!w.has_condition(target,combat::conditions::Kind::cracked_armor,100));
  const auto packets=box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p[0]==162;}));check(w.take_npc_deaths(source).size()==1);
 }
 check(chains==2);
 check(cases==66);std::cout<<"PASS66 NPC lightning Reversal cases across11 common/projectile plans;dedicated plans and paid307 remain pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
