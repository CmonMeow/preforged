#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Signet of Agony line "+std::to_string(at.line())+context);
}
int main()try{
 for(unsigned id:{145u,3059u})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<7;++mode){
  context=" id="+std::to_string(id)+" rank="+std::to_string(rank)+" mode="+std::to_string(mode);
  const auto plan=effects::elemental(id,rank);check(plan&&plan->damage==effects::damage_scale(10,70,rank)&&plan->radius==252&&plan->self_bleeding_ms==25000);
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
  // No armor is supplied: this signet's damage must ignore armor.
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Necromancer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{mode==4?50u:mode==6?115u:1000u,1000,50,100},100);
  w.join(2,"Foe",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,50,100},100);
  w.join(3,"Distant foe",300,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,50,100},100);
  w.join(4,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(4,{1000,1000,50,100},100);
  const auto owner=*w.effect_scope(1);if(mode==6)check(w.degeneration_hex(*w.effect_scope(2),owner,*effects::degeneration_hex(19,20),100));Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};Policy policy;policy.primary=4;policy.campaigns=255;policy.pvp_match=id==3059;
  policy.learned[id]=policy.learned[145]=1;attributes::Ranks ranks{};ranks[4]=rank;casting::Hooks h;unsigned commits=0,sacrifice_callbacks=0;std::vector<world::Packet> packets;
  h.resources=[&](unsigned actor){return w.resources(actor);};h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
  h.target_valid=[](unsigned actor,float,bool self){return actor==1&&self;};h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.consume_energy_at=[&](unsigned cost,auto now){check(cost==0);return w.consume_energy(1,cost,owner,now);};
  h.health_sacrifice=[&](unsigned skill,auto now){++sacrifice_callbacks;return w.health_sacrifice(owner,skill,now);};
  if(mode!=3)h.elemental=[&](auto target,const auto& value,auto now)->casting::EffectCompletion{check(target==owner);++commits;if(mode==5)return {true,{casting::generic(21,1,259)}};std::vector<world::Packet> visuals;std::optional<sacrifice::Loss> paid;const bool ok=w.elemental(owner,target,value,20,now,&visuals,&paid);return {ok,std::move(visuals),paid};};
  casting::Runtime runtime(1);check(runtime.start(1000,{id,0,100},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==3){check(w.resources(1)->health==1000);continue;}
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1100,h);
  runtime.update(1749,h);check(!commits);runtime.update(1750,h);const bool success=mode==0||mode==4||mode==6;
  check(commits==unsigned(success||mode==5)&&sacrifice_callbacks==0);check(w.resources(1)->health==(success?(mode==4||mode==6?0u:900u):1000u));check(w.resources(1)->energy==50);
  check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==1000-(success?plan->damage:0));
  check(w.resources(2)->health==1000-(success?unsigned(plan->damage):0));check(w.resources(3)->health==1000&&w.resources(4)->health==1000);
  check(w.has_condition(owner,combat::conditions::Kind::bleeding,1750)==(success&&mode!=4&&mode!=6));
  check(!w.has_condition(*w.effect_scope(100),combat::conditions::Kind::bleeding,1750));
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==unsigned(success));
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,259))==unsigned(success));
  const auto body=*particles::targeted_body_2022(100,1,id);check(std::count(packets.begin(),packets.end(),body)==unsigned(success));
  if(success){check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body));
   check(!w.has_condition(owner,combat::conditions::Kind::bleeding,26750));}
 }
 for(unsigned mode=0;mode<4;++mode){context=" reaction="+std::to_string(mode);world::Instance w;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float((id-1)*60),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),enemy=*w.effect_scope(2),ally=*w.effect_scope(3);const auto plan=*effects::elemental(145,0);
  if(mode==0){check(!w.elemental(owner,owner,plan,20,100));check(w.resources(1)->health==1000&&!w.has_condition(owner,combat::conditions::Kind::bleeding,100));continue;}
  if(mode==1)check(w.degeneration_hex(enemy,owner,*effects::degeneration_hex(19,12),100));
  if(mode==2){check(w.apply_fevered_dreams(enemy,owner,12,false,100,effects::FeveredDazedSpan::attempted_duration));check(w.process_fevered_condition_events(100));}
  std::optional<sacrifice::Loss> paid;std::vector<world::Packet> visuals;auto source=owner;if(mode==3)++source.generation;
  check(w.elemental(source,owner,plan,20,100,&visuals,&paid)==(mode!=3));
  if(mode==3){check(!paid&&visuals.empty()&&w.resources(1)->health==1000);continue;}
  check(paid&&paid->requested==100&&paid->remaining==900&&!paid->killed);
  check(w.resources(1)->health==(mode==1?900-effects::fragility(19,12)->damage:900));
  check(w.resources(2)->health==990);if(mode==2)check(w.has_condition(ally,combat::conditions::Kind::bleeding,100));
 }
 check(!effects::elemental(145,21));auto forged=*effects::elemental(145);++forged.self_bleeding_ms;check(!effects::valid_elemental(forged));
 std::cout<<"PASS Signet of Agony PvE/PvP ranks,armor-ignoring mixed area,range/allies,self Bleeding,sacrifice including death,zero Energy,cancel/interrupt,missing handler and captured body resources\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
