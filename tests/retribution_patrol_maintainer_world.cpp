// Detached integration consumers: actual maintained248 + production controller motion.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Controller maintainer line "+std::to_string(at.line()));
}
world::Instance::Npc npc(unsigned id,float x,unsigned energy=100){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.primary=3;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding={146,id,id,0,0,0,0,20};d.relation=combat::npcs::Relation::hostile_to_players;d.packed_allegiance=3;
 d.spawn_health=combat::Health{1000,1000};d.spawn_energy=combat::npcs::Energy{energy,100};d.energy_pips=4;
 d.physical_armor=60;d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic maintained controller integration";
 d.holy_traits={false,false};d.light_sensitive_evidence=d.summoned_evidence=d.healing_evidence;n.combat_definition=d;return n;
}
std::uint32_t u32(const world::Packet& p,std::size_t at){return std::uint32_t(p[at])|std::uint32_t(p[at+1])<<8|std::uint32_t(p[at+2])<<16|std::uint32_t(p[at+3])<<24;}
void configure_idle(world::Instance& w,unsigned kind,Scope source,Scope owner){
 combat::ai::FlarePolicy p;p.source=source;p.level=20;p.speed=440;p.acquire=100;p.follow=200;p.leash=10000;p.proximity_acquisition=false;
 p.flight={1000,32};p.evidence="synthetic maintained authored patrol frontier";
 if(kind==0)check(w.configure_npc_flare_ai(p,100));
 if(kind==1){check(w.configure_npc_elemental_ai(p,{{{214,0},*water_projectile(214,0),p.evidence}},4,100));check(w.observe_npc_overcast(source,0,100,p.evidence));}
 if(kind==2){combat::ai::ordinary::Reviewed a{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};check(w.configure_npc_hostile_ordinary(p,a,100));}
 check(w.configure_npc_wander(source,{{{6000,0},{-6000,0}},100,1000,p.evidence},100));
}
int main()try{
 for(unsigned kind=0;kind<3;++kind){world::Instance w;auto box=std::make_shared<world::Outbox>();w.configure_npcs({npc(100,0),npc(101,100)});
  w.join(1,"Patrol observer",-9000,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1),source=*w.effect_scope(100),target=*w.effect_scope(101);configure_idle(w,kind,source,owner);
  check(w.stage_retribution(source,target,*retribution(12),100));const auto entry=w.status_snapshot(target,100)->front();const auto wire=*mending_wire(source,target,entry);
  const auto application=box->drain();check(std::count(application.begin(),application.end(),wire.upkeep)==0);
  w.tick(100);const auto wait=w.maintained_range_path(source,100,10000);
  check(wait&&wait->front().x==0&&wait->back().x==0);check(w.maintained_enchantment_count(source,100)==1);
  w.tick(200);const auto active=w.maintained_range_path(source,200,3000);check(bool(active));
  check(std::any_of(active->begin(),active->end(),[](const auto& point){return point.at==1200&&std::abs(point.x-440)<.0001;}));
  check(std::abs(active->back().x-440)<.0001);box->drain();
  // A coarse next driver observes the authored1200ms timeout at2200. Motion
  // clamps at the old timeout; waiting/new patrol decisions start at2200,
  // never fabricate a second leg inside the unobserved interval.
  w.tick(2200);check(w.maintained_enchantment_count(source,2200)==1&&w.status_snapshot(target,2200)->size()==1);
  double actual=0;for(const auto& packet:box->drain())if(packet.size()==16&&packet[0]==44&&packet[1]==0&&u32(packet,2)==source.actor)actual=std::bit_cast<float>(u32(packet,6));
  check(std::abs(actual-440)<.0001);
  w.tick(2299);const auto waiting=w.maintained_range_path(source,2299,4000);check(waiting&&std::abs(waiting->front().x-440)<.0001&&std::abs(waiting->back().x-440)<.0001);
  w.tick(2300);const auto second=w.maintained_range_path(source,2300,5000);check(second&&std::abs(second->back().x)<.0001);
  w.tick(3300);check(w.maintained_enchantment_count(source,3300)==1&&w.status_snapshot(target,3300)->size()==1);
  const auto end=*mending_end_wire(source,target,entry);const auto observed=box->drain();check(std::count(observed.begin(),observed.end(),end.effect)==0&&std::count(observed.begin(),observed.end(),wire.upkeep)==0);
 }
 // Existing friendly/opposed ordinary authority authors attacks, not pursuit.
 // Proven standing controllers support both legal maintained directions.
 {world::Instance w;auto ally=npc(100,60);ally.combat_definition->relation=combat::npcs::Relation::allied_to_players;ally.combat_definition->packed_allegiance=6;w.configure_npcs({ally});
  w.join(1,"Allied player",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(100),player=*w.effect_scope(1);
  combat::ai::ordinary::Reviewed weapon{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},"synthetic standing friendly ordinary"};
  check(w.configure_npc_opposed_ordinary(source,weapon,100));check(w.stage_retribution(source,player,*retribution(12),100));check(w.stage_retribution(player,source,*retribution(12),100));
  const auto path=w.maintained_range_path(source,100,2000);check(path&&path->front().x==60&&path->back().x==60);
  w.tick(2000);check(w.maintained_enchantment_count(source,2000)==1&&w.maintained_enchantment_count(player,2000)==1);
  auto stale=source;++stale.generation;check(!w.maintained_range_path(stale,2000,2000));check(!w.stage_retribution(stale,player,*retribution(12),2000));
 }
 std::cout<<"PASS real248 leases through Flare/elemental/ordinary patrol wait+expiry+new frontier; friendly ordinary standing admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
