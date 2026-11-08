#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Icy Prism cast line "+std::to_string(at.line()));
}
int main()try{
 for(bool npc:{false,true})for(unsigned mode=0;mode<13;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.cold_armor=60;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1&&(mode==11||mode==12)?50u:1000u,1000,id==1&&mode==9?0u:id==1&&mode==11?10u:100u,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  check(w.energy_capacity(1,6,0,100,{80,-2}));
  const auto source=*w.effect_scope(1),enemy=*w.effect_scope(2),victim=*w.effect_scope(npc?100:2);
  if(mode==1||mode==10)check(w.movement_hex(source,victim,*effects::movement_hex(210,0),100));
  if(mode==11)check(w.apply_mind_wrack(enemy,source,12,false,100));
  if(mode==12)check(w.backfire(enemy,source,*effects::backfire(28,12),20,100));
  const unsigned start=mode==10?7500:1000,finish=start+1000;unsigned paid=0,commits=0;bool in_range=true;std::vector<world::Packet> sent;casting::Hooks h;
  h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  h.consume_overcast_energy=[&](auto skill,auto cost,auto event,auto now){++paid;check(skill==903&&cost==10&&event);return w.activate_overcast_energy(source,skill,cost,event,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return in_range&&id==victim.actor&&range==1248;};
  h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(source,skill,event,now);};
  h.icy_prism=[&](auto target,const auto& plan,auto now)->casting::EffectCompletion{
   ++commits;check(target==victim&&plan.rank==12&&now==finish);std::vector<world::Packet> visuals;
   const bool accepted=w.stage_icy_prism(source,target,plan,20,now,&visuals);return {accepted,std::move(visuals)};
  };
  if(mode==5)h.icy_prism={};if(mode==6)h.consume_overcast_energy={};
  Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[903]=1;Slots bar{};bar[0]={903,0};attributes::Ranks ranks{};ranks[11]=12;casting::Runtime cast(1);
  check(casting::Runtime::effect_supported(903));
  const auto expected=mode==5||mode==6?casting::Reason::unsupported:mode==9?casting::Reason::energy:mode==11?casting::Reason::policy:casting::Reason::accepted;
  check(cast.start(start,{903,0,victim.actor},bar,policy,ranks,true,h)==expected);
  if(mode==5||mode==6){check(!cast.active()&&!paid&&w.resources(1)->energy==100);continue;}
  if(mode==9){check(!cast.active()&&!commits&&w.resources(1)->energy==0);continue;}
  if(mode==11){check(!cast.active()&&!commits&&w.resources(1)->health==0);continue;}
  check(paid==1&&w.resources(1)->energy==90);
  check(w.overcast_energy_snapshot(source)->curve.burden_units()==5u*overcast::CeilingCurve::units_per_energy);
  cast.update(finish-1,h);check(!commits);
  if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(finish-1,h);
  if(mode==4)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==victim.actor)++scope->generation;return scope;};
  if(mode==7)in_range=false;if(mode==8)h.icy_prism={};
  box->drain();cast.update(finish,h);check(!cast.active()&&paid==1);
  const bool success=mode==0||mode==1||mode==10;check(commits==unsigned(success));
  const double primary=npc?w.npc_combat_snapshot(victim)->health->current:w.resources(2)->health;
  const double secondary=npc?w.resources(2)->health:w.npc_combat_snapshot(*w.effect_scope(100))->health->current;
  check(primary==(success?937:1000)&&secondary==(mode==1?937:1000));
  const auto body=*particles::targeted_body_2022(victim.actor,1,903);check(std::count(sent.begin(),sent.end(),body)==unsigned(success));
  if(success){check(std::find(sent.begin(),sent.end(),casting::terminal(58,1))<std::find(sent.begin(),sent.end(),body));check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{903,0},5))==1);
   check(cast.start(finish+1,{903,0,victim.actor},bar,policy,ranks,true,h)==casting::Reason::busy);check(cast.start(finish+750,{903,0,victim.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);}
  const auto broadcast=box->drain();check(std::count(broadcast.begin(),broadcast.end(),body)==0);
  if(mode==2||mode==3)check(w.overcast_energy_snapshot(source)->curve.burden_units()>0); // No cancellation refund.
  if(mode==12)check(w.resources(1)->health==0);
 }
 std::cout<<"PASS Icy Prism normal Runtime player caster to player/NPC targets,atomic10Energy+5Overcast,conditional/expired Water splash,cancel/interrupt/no refund,payment/reaction death,scope/hook guards and finish-body-recharge order;rendered QA pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

