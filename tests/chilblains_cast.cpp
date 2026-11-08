#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Chilblains cast line "+std::to_string(at.line()));
}
int main()try{
 for(bool production:{false,true})for(auto order:{effects::ChilblainsOrder::damage_then_remove,effects::ChilblainsOrder::remove_then_damage})for(unsigned skill:{144u})for(bool npc:{false,true})for(unsigned mode=0;mode<12;++mode){
  if(production&&order!=effects::captured_chilblains_order)continue;
  world::Instance w;auto box=std::make_shared<world::Outbox>();const unsigned target=npc?100:2;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.cold_armor=60;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  if(!npc)check(w.elemental_armor(*w.effect_scope(2),60,60));
  const auto source=*w.effect_scope(1),victim=*w.effect_scope(target);const auto plan=*effects::chilblains(12);
  unsigned commits=0;bool in_range=true;std::vector<world::Packet> sent;casting::Hooks h;
  h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());box->enqueue(packets);};
  h.consume_energy_at=[&](auto n,auto at){check(n==25);return w.consume_energy(1,n,source,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return in_range&&id==target&&range==1248;};
  h.before_spell_effect=[&](auto id,auto event,auto at){return w.before_spell_effect(source,id,event,at);};
  h.chilblains=[&](auto selected,const auto& p,auto now)->casting::EffectCompletion{
   ++commits;check(p==plan&&selected==victim);std::vector<world::Packet> visuals;
   std::function<void()> publication;
   const bool accepted=production?w.complete_chilblains(source,selected,p,20,now):w.stage_chilblains(source,selected,p,order,20,now,&visuals,&publication);
   if(!production)check(box->drain().empty());return {accepted,std::move(visuals),{},std::move(publication),false,production};
  };
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[7]=12;
  casting::Runtime cast(1);check(casting::Runtime::effect_supported(skill));if(mode==5)h.chilblains={};if(mode==8)h.before_spell_effect={};if(mode==10)w.set_vitals(1,{1000,1000,0,100},100);
  check(cast.start(1000,{skill,0,target},bar,policy,ranks,true,h)==(mode==5||mode==8?casting::Reason::unsupported:mode==10?casting::Reason::energy:casting::Reason::accepted));
  if(mode==5||mode==8||mode==10){check(w.resources(1)->energy==(mode==10?0:100)&&!cast.active());continue;}
  check(w.resources(1)->energy==75);const unsigned finish=3000;
  cast.update(finish-1,h);check(!commits);
  if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(finish-1,h);
  if(mode==4)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==target)++scope->generation;return scope;};
  if(mode==6)in_range=false;if(mode==7)h.chilblains={};if(mode==9)h.before_spell_effect={};if(mode==11)h.before_spell_effect=[](auto,auto,auto){return false;};
  box->drain();cast.update(finish,h);check(!cast.active());
  auto production_packets=production?box->drain():std::vector<world::Packet>{};if(production)sent=production_packets;
  const bool success=mode==0||mode==1;check(commits==unsigned(success));
  const double hp=npc?w.npc_combat_snapshot(victim)->health->current:w.resources(2)->health;
  check(hp==(success?(npc?1000-plan.damage:std::ceil(1000-plan.damage)):1000));
  check(w.has_condition(source,combat::conditions::Kind::poison,finish)==success);
  const auto body=particles::chilblains_target_2022(target,1);
  const auto caster_body=combat::detail::integer(21,1,256);
  check(std::count(sent.begin(),sent.end(),caster_body)==unsigned(success));
  check(std::count(sent.begin(),sent.end(),body)==unsigned(success));
  if(success){check(std::find(sent.begin(),sent.end(),casting::terminal(58,1))<std::find(sent.begin(),sent.end(),caster_body));
   check(std::find(sent.begin(),sent.end(),caster_body)<std::find(sent.begin(),sent.end(),body));
   cast.update(finish,h);check(commits==1&&std::count(sent.begin(),sent.end(),body)==1);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{skill,0},8))==1);
   check(cast.start(finish+1,{skill,0,target},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(finish+750,{skill,0,target},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }
  const auto broadcast=production?production_packets:box->drain();check(std::count(broadcast.begin(),broadcast.end(),body)==unsigned(success));
  if(success){const auto poison=std::find_if(broadcast.begin(),broadcast.end(),[](const auto& p){return p.size()>=8&&p[0]==65&&p[1]==0&&p[6]==228&&p[7]==1;});
   check(poison!=broadcast.end());
   const auto damage=std::find_if(broadcast.begin(),broadcast.end(),[&](const auto& p){return p.size()>=18&&p[0]==162&&p[1]==0&&p[2]==16&&p[6]==target&&p[10]==1;});
   check(damage!=broadcast.end()&&poison<damage);
   check(std::find(broadcast.begin(),broadcast.end(),casting::terminal(58,1))<poison);
   check(std::find(broadcast.begin(),broadcast.end(),caster_body)<poison);
   check(poison<std::find(broadcast.begin(),broadcast.end(),body));}
 }
 std::cout<<"PASS72 Chilblains staged/production Runtime->World player/NPC casts,25Energy/2s/8s,self-Poison,caster256/target257,cancel/interrupt and lifetime/hook guards;retail/rendering gaps remain\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
