#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC elemental cast line "+std::to_string(at.line())+context);
}
int main()try{
 for(unsigned skill:{191u,89u,1379u,2192u,846u,884u,1095u})for(unsigned mode=0;mode<7;++mode){
  context=" skill="+std::to_string(skill)+" mode="+std::to_string(mode);
  world::Instance w;world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=60;npc.definition.level=20;
  npc.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
  d.fire_armor=d.cold_armor=60;npc.combat_definition=d;w.configure_npcs({npc});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  check(w.energy_capacity(1,1,0,100,{80,-2}));const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);
  Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};Policy policy;policy.primary=6;policy.secondary=4;policy.campaigns=255;policy.learned[skill]=1;
  attributes::Ranks ranks{};casting::Hooks h;unsigned commits=0;bool stale=false;std::vector<world::Packet> packets;
  h.resources=[&](unsigned id){return w.resources(id);};h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,owner,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==100)return std::optional<effects::Scope>{};return w.effect_scope(id);};
  h.projectile_target_valid=[](unsigned id,float range){return id==100&&(range==1248||range==144);};
  if(mode!=4)h.elemental=[&](auto scope,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;if(mode==5)return {false,{casting::generic(21,100,123)}};
   box->drain();const bool accepted=w.elemental(owner,scope,plan,20,at,&visuals);const auto early=box->drain();
   for(const auto& visual:visuals)check(std::find(early.begin(),early.end(),visual)==early.end());
   return {accepted,std::move(visuals)};
  };
  casting::Runtime runtime(1);check(runtime.start(1000,{skill,0,100},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  const auto* row=find_skill(skill);const unsigned cost=skill==884?15:skill==89||skill==191?10:5;
  check(w.resources(1)->energy==100-(mode==4?0:cost));if(mode==4)continue;
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1100,h);if(mode==3)stale=true;
  if(mode==6)check(w.elemental(owner,target,*effects::elemental(191,12),20,1000));
  const auto end=1000+std::uint64_t(std::llround(row->activation*1000));runtime.update(end-1,h);check(!commits);runtime.update(end,h);
  const bool success=mode==0||mode==6;check(commits==unsigned(mode==0||mode==5||mode==6));
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==unsigned(success));
  const auto particle=particles::targeted_body_2022(100,1,skill);
  if(particle){check(std::count(packets.begin(),packets.end(),*particle)==unsigned(success));
   if(success)check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),*particle));}
  if(mode==0)check(w.npc_combat_snapshot(target)->health->current==1000-(skill==1095?7:skill==884?0:skill==191||skill==846?20:skill==89?10:5));
  if(!success)check(w.npc_combat_snapshot(target)->health->current==1000);
  if(mode==6&&skill==1379)check(w.resources(1)->energy==100); // Burning grants five Energy after the paid cast.
 }
 std::cout<<"PASS seven NPC elemental Runtime casts: cost,timing,completion/body ordering,cancel,interrupt,stale target,missing hook,failed effect and conditional Energy\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
