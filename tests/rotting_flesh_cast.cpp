#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Rotting Flesh cast line "+std::to_string(at.line()));
}
int main()try{
 using combat::conditions::Kind;
 for(bool pvp:{false,true})for(bool npc:{false,true})for(unsigned mode=0;mode<9;++mode){
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
   d.fleshy=true;d.fleshy_evidence="fixture";d.creature_type="human";d.creature_type_evidence="fixture";d.disease_immune=false;d.disease_immunity_evidence="fixture";
   n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{2000,2000,mode==6?14u:50u,100},100);
  if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,50,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  Policy policy;policy.primary=4;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[106]=1;
  Slots bar{};bar[0]={106,0};attributes::Ranks ranks{};ranks[5]=12;casting::Hooks h;std::vector<Packet> packets;unsigned payments=0,calls=0;bool stale=false;
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id)->std::optional<Scope>{if(stale&&id==target.actor)return {};return w.effect_scope(id);};
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.projectile_target_valid=[&](auto id,float range){return id==target.actor&&range==1248;};
  h.consume_energy_at=[&](auto cost,auto at){++payments;check(cost==15);return w.consume_energy(1,cost,source,at);};
  if(mode!=4)h.rotting_flesh=[&](auto selected,const auto& plan,auto at){++calls;std::vector<Packet> visuals;const bool ok=w.rotting_flesh(source,selected,plan,at,&visuals);return casting::EffectCompletion{ok,std::move(visuals)};};
  casting::Runtime cast(1);const auto admitted=cast.start(500,{106,0,target.actor},bar,policy,ranks,true,h);
  check(admitted==(mode==4?casting::Reason::unsupported:mode==6?casting::Reason::energy:casting::Reason::accepted));
  if(mode==4||mode==6){check(!calls&&payments==unsigned(mode==6)&&w.resources(1)->energy==(mode==6?14u:50u));continue;}
  check(payments==1&&w.resources(1)->energy==35);
  if(mode==2)cast.cancel(h,1000);if(mode==3)cast.interrupt(1000,h);if(mode==5)stale=true;
  cast.update(3499,h);check(!w.has_condition(target,Kind::disease,3499));cast.update(3500,h);
  const bool completed=mode!=2&&mode!=3&&mode!=5;
  const bool afflicted=completed;
  check(calls==unsigned(completed));
  check(w.has_condition(target,Kind::disease,3500)==afflicted);
  check(!w.has_condition(target,Kind::poison,3500));
  const auto body=*particles::targeted_body_2022(target.actor,source.actor,106);
  check(std::count(packets.begin(),packets.end(),body)==unsigned(completed));
  if(completed){const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish<std::find(packets.begin(),packets.end(),body));
   check(!w.has_condition(target,Kind::disease,25500));
   check(cast.start(4249,{106,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(6499,{106,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
   check(cast.start(6500,{106,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  }
 }
 std::cout<<"PASS Rotting Flesh Runtime PvE/PvP player/NPC,15Energy,3s cast,.75s aftercast,3s recharge,cancel/interrupt/stale target and finish/205 ordering\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
