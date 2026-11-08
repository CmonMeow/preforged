#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Shard Storm World line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills;using namespace skills::effects;
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){
   world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
   if(mode!=1){d.cold_armor=60;d.cold_armor_evidence="test";}n.combat_definition=d;w.configure_npcs({n});
  }
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Player",id==1?0:600,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{1000,1000,100,100},100);
   check(w.elemental_armor(*w.effect_scope(id),{},id==2&&mode==1?std::nullopt:std::optional<int>{60}));
  }
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);const auto plan=*water_projectile(213,15);
  auto health=[&]{return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);};
  std::vector<world::Packet> visuals{{1,2}};box->drain();
  check(w.water_projectile(caster,target,plan,20,1000,&visuals)==(mode!=1));
  check(visuals.size()==(mode==1?0:1));check(health()==1000);
  if(mode==1)continue;
  check(visuals[0][0]==163);
  if(mode==2)w.leave(1);
  if(mode==3&&!npc)check(w.hex_breaker(target,*hex_breaker(12),20,1100));
  if(mode==4)check(w.kill(caster,1100));
  w.tick(1234);check(health()==1000);
  w.tick(1334);check(health()==(mode==2?1000:915));
  const bool hex=npc?bool(w.npc_combat_snapshot(target)->effects&0x800):!w.status_snapshot(target,1334)->empty();
  check(hex==(mode!=2&&(mode!=3||npc)));
  check(w.resources(3)->health==1000); // Colocated foes are not Shard collateral.
  w.tick(1400);check(health()==(mode==2?1000:915));
  w.tick(7334);
  check(npc?!(w.npc_combat_snapshot(target)->effects&0x800):w.status_snapshot(target,7334)->empty());
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){
   world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
   d.cold_armor=60;d.cold_armor_evidence="test";n.combat_definition=d;w.configure_npcs({n});
  }
  for(unsigned id:{1u,2u}){
   w.join(id,"Player",id==1?0:600,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),{},60));
  }
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);unsigned commits=0,payments=0;bool stale=false;
  skills::casting::Hooks h;
  h.resources=[&](unsigned id){return w.resources(id);};h.events=[&](const auto& packets){w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){++payments;return w.consume_energy(1,cost,source,at);};
  h.target_scope=[&](unsigned id){return stale&&id==target.actor?std::optional<Scope>{}:w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==target.actor&&range==1248;};
  if(mode!=4)h.movement_hex=[&](auto recipient,const auto& plan,auto at)->skills::casting::EffectCompletion{
   ++commits;std::vector<world::Packet> out;
   if(mode==5)return {false,{{163,0}}};
   const auto projectile=water_projectile(plan.skill,plan.rank);check(bool(projectile));
   const bool ok=w.water_projectile(source,recipient,*projectile,20,at,&out);return {ok,std::move(out)};
  };
  skills::Slots bar{};bar[0]={213,0};skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[213]=1;
  attributes::Ranks ranks{};ranks[11]=12;skills::casting::Runtime cast(1);
  check(cast.start(1000,{213,0,target.actor},bar,policy,ranks,true,h)==(mode==4?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
  check(payments==unsigned(mode!=4)&&w.resources(1)->energy==(mode==4?100:90));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==unsigned(mode==0||mode==5)&&payments==1);
  const auto packets=box->drain();
  auto launch=std::find_if(packets.begin(),packets.end(),[](const auto& p){return !p.empty()&&p[0]==163;});
  check((launch!=packets.end())==(mode==0));
  if(mode==0)check(std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1))<launch);
  auto health=[&]{return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);};
  check(health()==1000);w.tick(2334);check(health()==(mode==0?930:1000));
 }
 std::cout<<"PASS Shard Storm World delayed player/NPC damage,hex,expiry,death,departure and selected-only targeting\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
