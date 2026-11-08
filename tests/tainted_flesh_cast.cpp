#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;using combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Tainted Flesh cast line "+std::to_string(at.line()));
}
int main()try{
 {
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Completion reaction",id==1?1000.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,50,100},100);}
  const auto hexer=*w.effect_scope(1),owner=*w.effect_scope(2);
  check(w.spiteful_spirit(hexer,owner,*spiteful_spirit(12),20,100));
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[113]=1;
  Slots slots{};slots[0]={113,0};attributes::Ranks ranks{};ranks[5]=12;casting::Hooks h;
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.events=[](const auto&){};
  h.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(2,cost,owner,at);};
  h.tainted_flesh_target_valid=[&](auto id,const auto& plan,auto at){return w.valid_tainted_flesh_target(owner,id,plan,at);};
  h.tainted_flesh=[&](auto selected,const auto& plan,auto at){std::vector<Packet> visuals;const bool ok=w.tainted_flesh(owner,selected,plan,at,&visuals);return casting::EffectCompletion{ok,std::move(visuals)};};
  unsigned completions=0;h.completed_action=[&](auto skill,auto at){++completions;return w.completed_skill_action(owner,skill,at);};
  casting::Runtime cast(2);check(cast.start(500,{113,0,2},slots,policy,ranks,true,h)==casting::Reason::accepted);
  cast.update(1499,h);check(!completions&&w.resources(2)->health==1000);
  cast.update(1500,h);check(completions==1&&w.resources(2)->health==971);
  cast.update(1500,h);check(completions==1&&w.resources(2)->health==971);
 }
 for(bool pvp:{false,true})for(unsigned kind=0;kind<3;++kind)for(unsigned mode=0;mode<=10;++mode){
  if((mode==8&&kind==0)||(mode==10&&kind!=1)||(mode==7&&kind==0))continue;
  world::Instance w;
  if(kind==2){world::Instance::Npc n;n.id=n.definition.id=100;n.x=mode==8?2000.f:60.f;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=mode==7?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,mode==6?4u:50u,100},100);
  if(kind==1){w.join(2,"Ally",mode==8?2000.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==7?2:1);w.set_vitals(2,{1000,1000,50,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(kind==0?1:kind==1?2:100);
  if(kind!=2&&mode!=7&&mode!=8){w.join(9,"Foe",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(9,{1000,1000,100,100},100);check(w.rotting_flesh(*w.effect_scope(9),target,*rotting_flesh(0),100));}
  Policy policy;policy.primary=4;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[113]=1;
  Slots bar{};bar[0]={113,0};attributes::Ranks ranks{};ranks[5]=12;casting::Hooks h;std::vector<Packet> packets;unsigned payments=0,calls=0;bool stale=false;
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id)->std::optional<Scope>{if(stale&&id==target.actor)return {};return w.effect_scope(id);};
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.consume_energy_at=[&](auto cost,auto at){++payments;check(cost==5);return w.consume_energy(1,cost,source,at);};
  if(mode!=4)h.tainted_flesh_target_valid=[&](auto id,const auto& plan,auto at){return w.valid_tainted_flesh_target(source,id,plan,at);};
  if(mode!=3)h.tainted_flesh=[&](auto selected,const auto& plan,auto at){++calls;std::vector<Packet> visuals;const bool ok=w.tainted_flesh(source,selected,plan,at,&visuals);return casting::EffectCompletion{ok,std::move(visuals)};};
  unsigned completions=0;h.completed_action=[&](unsigned skill,auto at){check(skill==113&&at==1500);++completions;return true;};
  casting::Runtime cast(1);const auto admitted=cast.start(500,{113,0,kind==0&&mode==0?0u:target.actor},bar,policy,ranks,true,h);
  const bool missing=mode==3||mode==4,invalid=mode==7||mode==8;
  check(admitted==(missing?casting::Reason::unsupported:invalid?casting::Reason::target:mode==6?casting::Reason::energy:casting::Reason::accepted));
  if(missing||invalid||mode==6){check(!calls&&payments==unsigned(mode==6));continue;}
  check(payments==1&&w.resources(1)->energy==45);
  if(mode==1)cast.cancel(h,1000);if(mode==2)cast.interrupt(1000,h);if(mode==5)stale=true;
  if(mode==9)check(w.kill(source,1000));
  if(mode==10){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},1000,1);w.set_vitals(2,{1000,1000,50,100},100);}
  cast.update(1499,h);check(!calls);cast.update(1500,h);
  const bool completed=mode!=1&&mode!=2&&mode!=5&&mode!=9&&mode!=10;
  check(calls==unsigned(completed));
  check(completions==unsigned(completed));cast.update(1500,h);check(completions==unsigned(completed));
  const auto body=kind==0?combat::detail::integer(21,1,214):*particles::targeted_body_2022(target.actor,1,113);
  check(std::count(packets.begin(),packets.end(),body)==unsigned(completed));
  if(completed){
   check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body));
   check(!w.has_condition(target,Kind::disease,1500));
   if(kind!=2){const auto entries=w.status_snapshot(target,1500);check(entries&&std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==113;}));}
   check(cast.start(2249,{113,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(2250,{113,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  }
 }
 std::cout<<"PASS Tainted Flesh Runtime self/ally/NPC PvE/PvP,5Energy,1s/.75s/zero recharge,cleanse,cancel/interrupt/stale/death and finish214 order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
