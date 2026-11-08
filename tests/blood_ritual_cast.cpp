#include "gw_blood_ritual.h"
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Blood Ritual cast line "+std::to_string(at.line()));
}
int main()try{
 for(bool pvp:{false,true})for(bool npc:{false,true})for(unsigned mode=0;mode<=16;++mode){
  if(mode==11||mode==12||(mode==9&&!npc)||(mode==15&&npc))continue;
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=mode==10?2000.f:60.f;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=mode==8?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{0,100};if(mode!=9)d.energy_pips=2;
   n.combat_definition=d;w.configure_npcs({n});}
  const unsigned maximum=1000,current=1000;
  w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{current,maximum,mode==6?0u:50u,100},100);
  if(!npc){w.join(2,"Ally",mode==10?2000.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==8?2:1);w.set_vitals(2,{1000,1000,0,100},100);}
  w.tick(100);const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);const auto selected=mode==7?source:target;
  Policy policy;policy.primary=4;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[157]=1;
  Slots bar{};bar[0]={157,0};attributes::Ranks ranks{};ranks[4]=12;casting::Hooks h;std::vector<Packet> packets;unsigned payments=0,calls=0,sacrifices=0;bool stale=false;
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id)->std::optional<Scope>{if(stale&&id==target.actor)return {};return w.effect_scope(id);};
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.consume_energy_at=[&](auto cost,auto at){++payments;check(cost==5);return w.consume_energy(1,cost,source,at);};
  if(mode!=4)h.energy_enchantment_target_valid=[&](auto id,const auto& plan,auto at){return w.valid_energy_enchantment_target(source,id,plan,at);};
  if(mode!=3)h.energy_enchantment=[&](auto destination,const auto& plan,auto at){++calls;
   if(mode==13)return casting::EffectCompletion{true}; // Invalid provider receipt must not complete.
   std::optional<sacrifice::Loss> paid;std::vector<Packet> visuals;
   const bool ok=w.energy_enchantment(source,destination,plan,at,&paid,&visuals);return casting::EffectCompletion{ok,std::move(visuals),paid};};
  if(mode!=14)h.health_sacrifice=[&](auto id,auto at){++sacrifices;return w.health_sacrifice(source,id,at);};
  casting::Runtime cast(1);const auto admitted=cast.start(500,{157,0,selected.actor},bar,policy,ranks,true,h);
  const bool missing=mode==3||mode==4,invalid=mode==7||mode==8||mode==9||mode==10;
  check(admitted==(missing?casting::Reason::unsupported:invalid?casting::Reason::target:mode==6?casting::Reason::energy:casting::Reason::accepted));
  if(missing||invalid||mode==6){check(!calls&&!sacrifices&&payments==unsigned(mode==6)&&w.resources(1)->health==current);continue;}
  check(payments==1&&w.resources(1)->energy==45);
  if(mode==1)cast.cancel(h,600);if(mode==2)cast.interrupt(600,h);if(mode==5)stale=true;
  if(mode==15){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1000,1000,0,100},100);check(*w.effect_scope(2)!=target);}
  if(mode==16)check(w.kill(source,600));
  cast.update(2499,h);check(w.resources(1)->health==(mode==16?0u:current));cast.update(2500,h);
  const bool completed=mode!=1&&mode!=2&&mode!=5&&mode!=13&&mode!=15&&mode!=16;
  check(!sacrifices&&calls==unsigned(completed||mode==13));
  const auto body=*casting::skill_success_body_effect(1,157);
  check(std::count(packets.begin(),packets.end(),body)==unsigned(completed));
  check(w.resources(1)->health==(completed?(830u):mode==16?0u:current));
  if(completed){const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish<std::find(packets.begin(),packets.end(),body));
   if(npc)check(w.npc_combat_snapshot(target)->effects&0x80);else{const auto state=w.status_snapshot(target,2500);check(state&&std::any_of(state->begin(),state->end(),[](const auto& e){return e.skill==157;}));}
   if(mode!=12){check(cast.start(3249,{157,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::busy);
    check(cast.start(4499,{157,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
    check(cast.start(4500,{157,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);}
  }
 }
 std::cout<<"PASS Blood Ritual Runtime PvE/PvP player/NPC,other-ally/known-resource admission,5Energy,2s/.75s/2s recharge,exactly-once sacrifice,stale/cancel/interrupt and finish before caster281 order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
