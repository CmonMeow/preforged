#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Virulence cast line "+std::to_string(at.line()));
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
  w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{2000,2000,mode==6?4u:50u,100},100);
  if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,50,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  if(mode!=1&&mode!=8){if(npc)check(w.condition_spell(source,target,*condition_spell(117,0),100));
   else check(w.apply_condition(source,target,Kind::blind,100,mode==7?1000:30000));}
  Policy policy;policy.primary=4;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[107]=1;
  Slots bar{};bar[0]={107,0};attributes::Ranks ranks{};ranks[5]=12;casting::Hooks h;std::vector<Packet> packets;unsigned payments=0,calls=0;bool stale=false;
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id)->std::optional<Scope>{if(stale&&id==target.actor)return {};return w.effect_scope(id);};
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.projectile_target_valid=[&](auto id,float range){return id==target.actor&&range==1248;};
  h.consume_energy_at=[&](auto cost,auto at){++payments;check(cost==5);return w.consume_energy(1,cost,source,at);};
  if(mode!=4)h.virulence=[&](auto selected,const auto& plan,auto at){++calls;std::vector<Packet> visuals;const bool ok=w.virulence(source,selected,plan,at,&visuals);return casting::EffectCompletion{ok,std::move(visuals)};};
  casting::Runtime cast(1);const auto admitted=cast.start(500,{107,0,target.actor},bar,policy,ranks,true,h);
  check(admitted==(mode==4?casting::Reason::unsupported:mode==6?casting::Reason::energy:casting::Reason::accepted));
  if(mode==4||mode==6){check(!calls&&payments==unsigned(mode==6)&&w.resources(1)->energy==(mode==6?4u:50u));continue;}
  check(payments==1&&w.resources(1)->energy==45);
  if(mode==2)cast.cancel(h,1000);if(mode==3)cast.interrupt(1000,h);if(mode==5)stale=true;
  if(mode==8){if(npc)check(w.condition_spell(source,target,*condition_spell(117,0),1000));else check(w.apply_condition(source,target,Kind::blind,1000,30000));}
  cast.update(1499,h);check(!w.has_condition(target,Kind::disease,1499));cast.update(1500,h);
  const bool completed=mode!=2&&mode!=3&&mode!=5;
  const bool afflicted=completed&&(mode!=1)&&(mode!=7||npc);
  check(calls==unsigned(completed));
  for(auto kind:{Kind::disease,Kind::poison})check(w.has_condition(target,kind,1500)==afflicted);
  const auto body=*particles::targeted_body_2022(target.actor,source.actor,107);
  check(std::count(packets.begin(),packets.end(),body)==unsigned(completed));
  if(completed){const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish<std::find(packets.begin(),packets.end(),body));
   check(!w.has_condition(target,Kind::disease,14500));
   check(cast.start(2249,{107,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(16499,{107,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
   check(cast.start(16500,{107,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  }
 }
 std::cout<<"PASS Virulence Runtime PvE/PvP player/NPC,5Energy,1s cast,.75s aftercast,15s recharge,cancel/interrupt/stale target and finish/206 ordering\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
