#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Action hex cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{898u,899u})for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();const unsigned target=npc?100:2;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),victim=*w.effect_scope(target);const auto plan=*effects::action_damage_hex(skill,12);
  if(mode!=1){if(npc)check(bool(w.begin_npc_action(victim,combat::npcs::control::Action::spell,100,5000,Slot{320,0})));
   else check(w.cast_activity(victim,320,100,5000));}
  unsigned commits=0;bool in_range=true;std::vector<world::Packet> sent;casting::Hooks h;
  h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  h.consume_energy_at=[&](auto n,auto at){check(n==5);return w.consume_energy(1,n,source,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return in_range&&id==target&&range==1248;};
  h.before_spell_effect=[&](auto id,auto event,auto at){return w.before_spell_effect(source,id,event,at);};
  h.action_damage_hex=[&](auto selected,const auto& p,auto now)->casting::EffectCompletion{
   ++commits;check(p==plan&&selected==victim);std::vector<world::Packet> visuals;
   const bool accepted=w.stage_action_damage_hex(source,selected,p,20,now,&visuals);return {accepted,std::move(visuals)};
  };
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[1]=ranks[2]=12;
  casting::Runtime cast(1);check(casting::Runtime::effect_supported(skill));if(mode==5)h.action_damage_hex={};
  check(cast.start(1000,{skill,0,target},bar,policy,ranks,true,h)==(mode==5?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==5){check(w.resources(1)->energy==100&&!cast.active());continue;}
  check(w.resources(1)->energy==95);const unsigned finish=skill==898?1250:3000;
  cast.update(finish-1,h);check(!commits);
  if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(finish-1,h);
  if(mode==4)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==target)++scope->generation;return scope;};
  if(mode==6)in_range=false;if(mode==7)h.action_damage_hex={};
  box->drain();cast.update(finish,h);check(!cast.active());
  const bool success=mode==0||mode==1;check(commits==unsigned(success));
  const double hp=npc?w.npc_combat_snapshot(victim)->health->current:w.resources(2)->health;
  check(hp==(mode==0?1000-plan.damage:1000));
  const auto body=*particles::targeted_body_2022(target,1,skill);
  check(std::count(sent.begin(),sent.end(),body)==unsigned(success));
  if(success){check(std::find(sent.begin(),sent.end(),casting::terminal(58,1))<std::find(sent.begin(),sent.end(),body));
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{skill,0},5))==1);
   check(cast.start(finish+1,{skill,0,target},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(finish+750,{skill,0,target},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }
  const auto broadcast=box->drain();check(std::count(broadcast.begin(),broadcast.end(),body)==0);
 }
 std::cout<<"PASS Overload/Images normal Runtime->World player/NPC casts,payment,activation,conditional damage,deferred particle/recharge order,cancel/interrupt and lifetime/hook guards;rendered QA pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
