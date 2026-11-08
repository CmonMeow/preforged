#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Spiteful Spirit cast line "+std::to_string(at.line()));
}
int main()try{
 for(bool pvp:{false,true})for(bool npc:{false,true})for(unsigned mode=0;mode<10;++mode){
  if(npc&&mode==7)continue;
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,mode==6?14u:50u,100},100);
  if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==7?1:2);w.set_vitals(2,{1000,1000,50,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  Policy policy;policy.primary=4;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[121]=1;
  Slots bar{};bar[0]={121,0};attributes::Ranks ranks{};ranks[7]=12;casting::Hooks h;
  std::vector<Packet> packets;unsigned payments=0,calls=0,completed_uses=0;bool stale=false;std::uint64_t at=500;std::string phase;
  h.diagnostic=[&](auto,const char* value){phase=value;};
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id)->std::optional<Scope>{if(stale&&id==target.actor)return {};return w.effect_scope(id);};
  h.projectile_target_valid=[&](auto id,auto range){return w.projectile_target_valid(source,id,range,at);};
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.consume_energy_at=[&](auto cost,auto when){++payments;check(cost==15);return w.consume_energy(1,cost,source,when);};
  if(mode!=4)h.spiteful_spirit=[&](auto selected,const auto& plan,auto when){++calls;check(plan.rank==12);std::vector<Packet> visuals;const bool ok=w.spiteful_spirit(source,selected,plan,20,when,&visuals);return casting::EffectCompletion{ok,std::move(visuals)};};
  if(mode!=8)h.before_spell_effect=[&](auto skill,auto event,auto when){return w.before_spell_effect(source,skill,event,when);};
  if(mode!=9)h.completed_action=[&](auto skill,auto when){++completed_uses;return w.completed_skill_action(source,skill,when);};
  casting::Runtime cast(1);const auto admitted=cast.start(at,{121,0,target.actor},bar,policy,ranks,true,h);
  const bool missing=mode==4||mode==8||mode==9;
  check(admitted==(missing?casting::Reason::unsupported:mode==7?casting::Reason::target:mode==6?casting::Reason::energy:casting::Reason::accepted));
  if(missing||mode==6||mode==7){check(!calls&&!completed_uses);continue;}
  check(payments==1&&w.resources(1)->energy==35);
  if(mode==1)cast.cancel(h,1000);if(mode==2)cast.interrupt(1000,h);if(mode==3)stale=true;if(mode==5)check(w.kill(source,1000));
  at=2500;cast.update(2499,h);check(!calls);cast.update(2500,h);
  const bool applied=mode==0;
  if(calls!=unsigned(applied)||completed_uses!=unsigned(applied))std::cerr<<"mode="<<mode<<" npc="<<npc<<" pvp="<<pvp<<" calls="<<calls<<" completions="<<completed_uses<<" phase="<<phase<<'\n';
  check(calls==unsigned(applied)&&completed_uses==unsigned(applied));
  const auto body=*particles::targeted_body_2022(target.actor,source.actor,121);
  check(std::count(packets.begin(),packets.end(),body)==unsigned(applied));
  if(applied){
   check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body));
   check(w.completed_skill_action(target,1239,2600));
   check((npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health)==971);
   cast.update(2600,h);check(calls==1&&completed_uses==1);
   check(cast.start(3249,{121,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(12499,{121,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
   at=12500;check(cast.start(at,{121,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  }
 }
 std::cout<<"PASS Spiteful Spirit Runtime player/NPC PvE/PvP,15Energy,2s cast,.75s aftercast,10s recharge,rank,227 finish ordering,trigger and denial/cancel/interrupt/death gates\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
