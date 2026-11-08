#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool ok,std::source_location where=std::source_location::current()){
 if(!ok)throw std::runtime_error("Save Yourselves line "+std::to_string(where.line()));
}
int main()try{
 for(unsigned skill:{1954u,2097u})for(unsigned rank=0;rank<=12;++rank){
  const auto plan=save_yourselves(skill,rank);
  check(plan&&plan->armor==100&&plan->adrenaline==200&&
        plan->duration==1000ull*(rank==0||rank==1?4u:rank<5?5u:6u));
  Scope source{1,1,1},target{1,2,1};
  check(!prepare_save_yourselves(*plan,source,source,1,100,true,true,0));
  check(!prepare_save_yourselves(*plan,source,target,1,100,false,true,10));
  check(!prepare_save_yourselves(*plan,source,target,1,100,true,true,1013));
  auto entry=prepare_save_yourselves(*plan,source,target,1,100,true,true,1012);
  check(entry&&entry->skill==skill&&entry->expires==100+plan->duration);
  Registry registry(target);check(registry.apply(target,*entry,Replace::same_skill).accepted);
  check(save_yourselves_armor(registry,target,100)==100);
  check(save_yourselves_armor(registry,target,100+plan->duration)==0);
 }
 for(unsigned skill:{1954u,2097u}){
  world::Instance world;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u,4u}){
   world.join(id,"Actor",id==3?1100.f:float(id*30),0,
       id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==4?2:1);
   world.set_vitals(id,{1000,1000,100,100},100);
  }
  for(unsigned id:{2u,3u})check(world.party_action(1,158,id,100)&&world.party_action(id,154,1,100));
  const auto owner=*world.effect_scope(1),ally=*world.effect_scope(2);
  Slots bar{};bar[0]={skill,0};check(world.configure_adrenaline(owner,bar,100));
  for(unsigned i=0;i<8;++i)check(world.complete_to_limit(owner,*to_limit(20),100));
  casting::Runtime runtime(1);casting::Hooks hooks;unsigned spends=0,completions=0;
  hooks.resources=[&](auto id){return world.resources(id);};
  hooks.events=[&](const auto& packets){box->enqueue(packets);};
  hooks.consume_energy_at=[&](auto energy,auto at){return world.consume_energy(1,energy,owner,at);};
  hooks.target_scope=[&](auto id){return world.effect_scope(id);};
  hooks.target_valid=[](auto id,float,bool){return id==1;};
  hooks.consume_adrenaline=[&](auto slot,auto at){++spends;return world.consume_adrenaline(owner,slot,at);};
  hooks.reconcile_adrenaline=[&](auto at){world.reconcile_adrenaline(owner,at);};
  hooks.allegiance_rank=[](unsigned){return std::optional<unsigned>{6};};
  hooks.save_yourselves=[&](auto source,const auto& plan,auto at){++completions;
   std::function<void()> publish;const auto ok=world.stage_save_yourselves_party(source,plan,at,&publish);
   return casting::EffectCompletion{ok,{}, {},std::move(publish)};};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[skill]=1;
  attributes::Ranks ranks{};box->drain();
  check(runtime.start(100,{skill,0,4},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
  check(spends==1&&completions==1&&world.adrenaline(owner)->front().units==0);
  const auto owner_status=world.status_snapshot(owner,100);
  check(owner_status&&std::none_of(owner_status->begin(),owner_status->end(),
      [&](const auto& entry){return entry.skill==skill;}));
  const auto ally_status=world.status_snapshot(ally,100);
  check(ally_status&&ally_status->size()==1&&ally_status->front().skill==skill);
  check(world.status_snapshot(*world.effect_scope(3),100)->empty());
  check(world.status_snapshot(*world.effect_scope(4),100)->empty());
  auto packets=box->drain();
  check(std::count(packets.begin(),packets.end(),casting::generic(48,1,skill))==1);
  check(std::count(packets.begin(),packets.end(),*casting::skill_success_body_effect(1,skill))==1);
  check(runtime.start(101,{skill,0,4},bar,policy,ranks,true,hooks)==casting::Reason::adrenaline);
  world.tick(6100);check(world.status_snapshot(ally,6100)->empty());
  auto missing_rank=hooks;missing_rank.allegiance_rank={};
  casting::Runtime absent(1);check(absent.start(200,{skill,0,4},bar,policy,ranks,true,missing_rank)==casting::Reason::unsupported);
  auto pvp=policy;pvp.pvp_match=true;
  casting::Runtime arena(1);check(arena.start(200,{skill,0,4},bar,pvp,ranks,true,hooks)==casting::Reason::pve_only);
 }
 const auto spell_loss=[](bool protected_ally){
  world::Instance w;
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),
       0,{}, {},100,id==3?2:1);
   w.set_vitals(id,{500,500,100,100},100);
  }
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.lightning_armor(ally,60));
  if(protected_ally)check(w.stage_save_yourselves_party(source,*save_yourselves(1954,6),100));
  check(w.lightning(foe,ally,*lightning(224,12),20,100));
  return 500u-w.resources(2)->health;
 };
 check(spell_loss(true)<spell_loss(false));
 {
  world::Instance w;world::Instance::Npc npc;npc.id=npc.definition.id=100;
  npc.x=100;npc.y=0;npc.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition definition;definition.binding.map=146;
  definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;
  definition.spawn_health=combat::Health{500,500};
  definition.physical_armor=definition.earth_armor=60;
  definition.damageable=definition.conditions_allowed=true;
  npc.combat_definition=definition;w.configure_npcs({npc});
  auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Actor",0,0,id==1?box:std::make_shared<world::Outbox>(),
      0,{}, {},100,1);w.set_vitals(id,{500,500,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1),member=*w.effect_scope(2),owned=*w.effect_scope(100);
  check(w.set_npc_party_owner(owned,member));box->drain();
  std::function<void()> publish;
  check(w.stage_save_yourselves_party(source,*save_yourselves(2097,6),200,&publish));
  check(box->drain().empty()&&bool(publish));publish();
  const auto status=w.status_snapshot(owned,200);
  check(status&&status->size()==1&&status->front().skill==2097);
  const auto effect=status->front().instance;box->drain();
  w.tick(6200);check(w.status_snapshot(owned,6200)->empty());
  const auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[&](const auto& packet){
   return packet.size()==10&&packet[0]==67&&packet[1]==0&&packet[2]==100&&packet[6]==effect;
  })==1);
 }
 const auto npc_health=[](bool protected_ally,bool ranged){
  world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc npc;npc.id=npc.definition.id=id;
   npc.x=id==100?0.f:100.f;npc.y=0;
   npc.definition.level=4;npc.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition definition;definition.binding.map=146;
   definition.binding.actor=definition.binding.definition=id;
   definition.binding.level=4;
   definition.relation=id==100?combat::npcs::Relation::allied_to_players:
       combat::npcs::Relation::hostile_to_players;
   definition.spawn_health=combat::Health{500,500};
   definition.damageable=true;definition.physical_armor=60;
   npc.combat_definition=definition;npcs.push_back(npc);
  }
  w.configure_npcs(npcs);
  w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(1,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(100),enemy=*w.effect_scope(101);
  check(w.set_npc_party_owner(ally,owner));
  if(protected_ally)check(w.stage_save_yourselves_party(owner,*save_yourselves(1954,6),100));
  const combat::ai::ordinary::Reviewed weapon{80,80,0,0,false,
      ranged?combat::ai::ordinary::Delivery::ranged:combat::ai::ordinary::Delivery::melee,
      750,2000,500,{1200,32},"synthetic reviewed delivery regression"};
  check(w.configure_npc_opposed_ordinary(enemy,weapon,1000));
  check(w.begin_npc_opposed_ordinary(enemy,ally,1000).has_value());
  w.tick(1750);w.tick(2500);
  return w.npc_combat_snapshot(ally)->health->current;
 };
 for(bool ranged:{false,true}){
  const auto plain=npc_health(false,ranged),guarded=npc_health(true,ranged);
  check(plain<guarded&&guarded<500);
 }
 check(!save_yourselves(1953,6)&&!save_yourselves(1954,13));
 std::cout<<"PASS Save Yourselves paired rank/party/adrenaline/animation/status/expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
