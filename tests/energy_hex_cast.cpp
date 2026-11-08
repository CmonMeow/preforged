#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy hex cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{125u,140u})for(bool npc:{false,true})for(unsigned mode=0;mode<9;++mode){
  world::Instance w;const unsigned target=npc?100:2;auto box=std::make_shared<world::Outbox>();
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
   d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  }
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,mode==7?0u:50u,50},100);
  if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),victim=*w.effect_scope(target);
  unsigned commits=0;bool in_range=mode!=6;std::vector<world::Packet> sent;
  casting::Hooks h;h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){check(cost==(skill==125?10u:5u));return w.consume_energy(1,cost,source,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.projectile_target_valid=[&](auto id,float range){return in_range&&id==target&&range==1248;};
  h.before_spell_effect=[&](auto id,auto event,auto now){return w.before_spell_effect(source,id,event,now);};
  h.energy_degeneration_hex=[&](auto scope,const auto& plan,auto now)->casting::EffectCompletion{
   ++commits;check(scope==victim&&plan.skill==skill&&plan.rank==12&&now==2100);
   std::vector<world::Packet> visuals;
   const bool accepted=w.stage_energy_degeneration_hex(source,scope,plan,20,now,&visuals);
   return {accepted,std::move(visuals)};
  };
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;
  Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[7]=12;casting::Runtime cast(1);
  // Production advertisement remains disabled until lifecycle/rendered gates.
  check(!casting::Runtime::effect_supported(skill));
  if(mode==5)h.energy_degeneration_hex={};
  const auto expected=mode==5?casting::Reason::unsupported:mode==6?casting::Reason::target:mode==7?casting::Reason::energy:casting::Reason::accepted;
  check(cast.start(100,{skill,0,target},bar,policy,ranks,true,h)==expected);
  if(mode==5||mode==6||mode==7){check(!cast.active()&&!commits);check(w.resources(1)->energy==(mode==7?0u:50u));continue;}
  check(w.resources(1)->energy==(skill==125?40u:45u));
  cast.update(2099,h);check(!commits);
  if(mode==1)cast.cancel(h);
  if(mode==2)cast.interrupt(2099,h);
  if(mode==3)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==target)++scope->generation;return scope;};
  if(mode==4)h.energy_degeneration_hex={};
  if(mode==8)in_range=false;
  box->drain();cast.update(2100,h);check(!cast.active());
  if(mode){check(!commits);continue;}
  check(commits==1);
  if(!npc){const auto status=*w.status_snapshot(victim,2100);check(status.size()==1&&status[0].skill==skill&&status[0].expires==31100);}
  else check(w.npc_combat_snapshot(victim)->effects&0x800);
  const auto body=*particles::targeted_body_2022(target,1,skill);
  const auto finish=casting::terminal(58,1);
  check(std::count(sent.begin(),sent.end(),body)==1);
  check(std::find(sent.begin(),sent.end(),finish)<std::find(sent.begin(),sent.end(),body));
  const auto broadcasts=box->drain();check(std::count(broadcasts.begin(),broadcasts.end(),body)==0);
  const auto recharge=skill==125?10u:2u;
  check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{skill,0},recharge))==1);
  check(cast.start(2101,{skill,0,target},bar,policy,ranks,true,h)==casting::Reason::busy);
  check(cast.start(2850,{skill,0,target},bar,policy,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS trusted125/140 Runtime payment,2s activation,finish-before-target body,aftercast,recharge,cancel/interrupt and hook/target guards;production advertisement and rendered QA pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
