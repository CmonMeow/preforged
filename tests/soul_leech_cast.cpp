#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Soul Leech cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{128u})for(bool npc:{false,true})for(unsigned mode=0;mode<12;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();const unsigned target=npc?100:2;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),victim=*w.effect_scope(target);const auto plan=*effects::soul_leech(12);
  unsigned commits=0;bool in_range=true;std::vector<world::Packet> sent;casting::Hooks h;
  h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  h.consume_energy_at=[&](auto n,auto at){check(n==10);return w.consume_energy(1,n,source,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return in_range&&id==target&&range==1248;};
  h.before_spell_effect=[&](auto id,auto event,auto at){return w.before_spell_effect(source,id,event,at);};
  h.soul_leech=[&](auto selected,const auto& p,auto now)->casting::EffectCompletion{
   ++commits;check(p==plan&&selected==victim);std::vector<world::Packet> visuals;
   const bool accepted=w.stage_soul_leech(source,selected,p,now,&visuals);return {accepted,std::move(visuals)};
  };
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[4]=12;
  casting::Runtime cast(1);check(casting::Runtime::effect_supported(skill));if(mode==5)h.soul_leech={};if(mode==8)h.before_spell_effect={};if(mode==10)w.set_vitals(1,{1000,1000,0,100},100);
  check(cast.start(1000,{skill,0,target},bar,policy,ranks,true,h)==(mode==5||mode==8?casting::Reason::unsupported:mode==10?casting::Reason::energy:casting::Reason::accepted));
  if(mode==5||mode==8||mode==10){check(w.resources(1)->energy==(mode==10?0:100)&&!cast.active());continue;}
  check(w.resources(1)->energy==90);const unsigned finish=3000;
  cast.update(finish-1,h);check(!commits);
  if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(finish-1,h);
  if(mode==4)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==target)++scope->generation;return scope;};
  if(mode==6)in_range=false;if(mode==7)h.soul_leech={};if(mode==9)h.before_spell_effect={};if(mode==11)h.before_spell_effect=[](auto,auto,auto){return false;};
  box->drain();cast.update(finish,h);check(!cast.active());
  const bool success=mode==0||mode==1;check(commits==unsigned(success));
  const double hp=npc?w.npc_combat_snapshot(victim)->health->current:w.resources(2)->health;
  check(hp==1000);
  const auto body=*particles::targeted_body_2022(target,1,skill);
  check(std::count(sent.begin(),sent.end(),body)==unsigned(success));
  if(success){check(std::find(sent.begin(),sent.end(),casting::terminal(58,1))<std::find(sent.begin(),sent.end(),body));
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{skill,0},15))==1);
   check(cast.start(finish+1,{skill,0,target},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(finish+750,{skill,0,target},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }
  if(mode==0){check(w.before_spell_effect(victim,281,1,finish));
   const auto after=npc?w.npc_combat_snapshot(victim)->health->current:w.resources(2)->health;check(after==933);}
  const auto broadcast=box->drain();check(std::count(broadcast.begin(),broadcast.end(),body)==0);
 }
 std::cout<<"PASS Soul Leech normal Runtime->World player/NPC casts,10Energy/2s/15s,spell reaction,deferred237,cancel/interrupt and lifetime/hook guards;retail/rendering gaps remain\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
