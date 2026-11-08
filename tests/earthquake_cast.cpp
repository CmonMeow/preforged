#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earthquake cast line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;Scope source,target;bool npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool is_npc=false,bool armor=true):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
   d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;
   d.earth_armor=60;d.knockdown_allowed=true;d.knockdown_evidence="fixture";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Elementalist",0,0,box,0,{}, {},100,1);w.set_vitals(1,{2000,2000,50,100},100);
  check(w.energy_capacity(1,6,0,100,{80,-2}));source=*w.effect_scope(1);check(w.elemental_armor(source,60,60));
  if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,50,100},100);if(armor)check(w.combat_armor(*w.effect_scope(2),60));}
  target=*w.effect_scope(npc?100:2);
 }
 double health()const{return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 void enchant(std::uint64_t end=60100,std::uint64_t now=100){check(w.status_mutation(target,180,99,12,now,end,true));}
};
int main()try{
 for(unsigned skill:{170u,1086u}){
  const auto plan=*elemental(skill,12);combat::Actor actor;actor.id=2;actor.health=combat::Health{1000,1000};actor.earth_armor=60;actor.damageable=true;
  check(!elemental_target(plan,actor,{},20,1,100).accepted&&actor.health->current==1000);
  bool rejected=false;try{combat::ai::offensive::Plan unsupported(plan);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }

 for(unsigned skill:{170u,1086u})for(bool pvp:{false,true})for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  Fixture f(npc);if(mode==5)f.w.set_vitals(1,{2000,2000,24,100},100);
  Policy policy;policy.primary=6;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[skill]=1;
  Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[9]=12;casting::Hooks h;std::vector<Packet> packets;unsigned payments=0;bool stale=false;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id)->std::optional<Scope>{if(stale&&id==f.target.actor)return {};return f.w.effect_scope(id);};
  h.events=[&](const auto& batch){f.box->enqueue(batch);};h.projectile_target_valid=[&](auto id,float){return id==f.target.actor;};
  if(mode!=3)h.consume_overcast_energy=[&](auto id,auto cost,auto event,auto at){++payments;check(id==skill&&cost==25);return f.w.activate_overcast_energy(f.source,id,cost,event,at);};
  if(mode!=4)h.elemental=[&](auto target,const auto& plan,auto at){std::vector<Packet> visuals;std::function<void()> publication;const bool ok=f.w.elemental(f.source,target,plan,20,at,&visuals,nullptr,nullptr,{},&publication);return casting::EffectCompletion{ok,std::move(visuals),{},std::move(publication),true};};
  f.box->drain();casting::Runtime cast(1);check(cast.start(500,{skill,0,f.target.actor},bar,policy,ranks,true,h)==(mode==3||mode==4?casting::Reason::unsupported:mode==5?casting::Reason::energy:casting::Reason::accepted));
  if(mode==3||mode==4){check(!payments&&f.w.resources(1)->energy==50);continue;}
  if(mode==5){check(payments==1&&f.w.resources(1)->energy==24&&!f.w.overcast_energy_snapshot(f.source));continue;}
  check(payments==1&&f.w.resources(1)->energy==25&&f.w.overcast_energy_snapshot(f.source)->curve.burden_units()==1000000);
  if(mode==1)cast.cancel(h,1000);if(mode==2)cast.interrupt(1000,h);
  if(mode==6){if(npc){stale=true;}else{f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,50,100},100);check(f.w.combat_armor(*f.w.effect_scope(2),60));}}
  cast.update(3500,h);packets=f.box->drain();const bool success=mode==0;
  if(!(npc&&mode==6))check(std::abs(f.health()-(success?(npc?914.8:915):1000))<1e-9);
  const auto damage=combat::detail::integer(10,1,skill);const auto body=combat::detail::integer(21,1,303);
  check(std::count(packets.begin(),packets.end(),damage)==unsigned(success)&&std::count(packets.begin(),packets.end(),body)==unsigned(success));
  if(success){const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish<std::find(packets.begin(),packets.end(),body)&&std::find(packets.begin(),packets.end(),body)<std::find(packets.begin(),packets.end(),damage));
   check(f.w.knocked_down(f.target,5499)&&!f.w.knocked_down(f.target,5500));
   check(cast.start(4249,{skill,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(18499,{skill,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
   check(cast.start(18500,{skill,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  }
 }
 std::cout<<"PASS Earthquake/Dragon's Stomp Runtime player/NPC PvE/PvP,atomic25Energy+10Overcast,cast lifecycle and finish/caster/damage order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
