// Detached acceptance target; controller adapter implementation is reserved next.
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Maintained controller projection line "+std::to_string(at.line()));
}
std::uint32_t u32(const world::Packet& p,std::size_t i){return std::uint32_t(p[i])|std::uint32_t(p[i+1])<<8|std::uint32_t(p[i+2])<<16|std::uint32_t(p[i+3])<<24;}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source;
 explicit Fixture(unsigned kind){
  world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding={160,100,100,0,0,0,0,1};d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;d.damageable=d.conditions_allowed=true;n.combat_definition=d;
  w.configure_npcs({n});w.join(1,"Pursuit target",1400,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  source=*w.effect_scope(100);const auto owner=*w.effect_scope(1);
  check(w.physical_armor(owner,0)&&w.elemental_armor(owner,0,0));
  combat::ai::FlarePolicy p;p.source=source;p.target=owner;p.level=1;p.speed=100;p.acquire=1500;p.follow=1800;p.leash=2000;
  p.flight={1000,32};p.evidence="synthetic controller maintained-projection oracle";
  if(kind==0)check(w.configure_npc_flare_ai(p,100));
  if(kind==1){check(w.configure_npc_elemental_ai(p,{{{214,0},*skills::effects::water_projectile(214,0),p.evidence}},4,100));check(w.observe_npc_overcast(source,0,100,p.evidence));}
  if(kind==2){combat::ai::ordinary::Reviewed a{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};check(w.configure_npc_hostile_ordinary(p,a,100));}
  w.tick(100);check(w.npc_pursuit_movement_snapshot(source).has_value());box->drain();
 }
 std::pair<double,double> materialize(std::uint64_t at){
  w.tick(at);std::pair<double,double> position{0,0};
  for(const auto& packet:box->drain())if(packet.size()==16&&packet[0]==44&&packet[1]==0&&u32(packet,2)==source.actor)
   position={std::bit_cast<float>(u32(packet,6)),std::bit_cast<float>(u32(packet,10))};
  return position;
 }
};
std::pair<double,double> interpolate(const std::vector<skills::effects::MaintainedRangePoint>& path,std::uint64_t at){
 const auto found=std::lower_bound(path.begin(),path.end(),at,[](const auto& p,auto time){return p.at<time;});check(found!=path.end());
 if(found->at==at)return {found->x,found->y};check(found!=path.begin());const auto& prior=*std::prev(found);
 const double t=double(at-prior.at)/double(found->at-prior.at);return {std::lerp(prior.x,found->x,t),std::lerp(prior.y,found->y,t)};
}
int main()try{
 for(unsigned kind=0;kind<3;++kind){
  Fixture baseline(kind);const auto before=baseline.w.npc_pursuit_movement_snapshot(baseline.source);const auto bodies=baseline.w.npc_packets();
  const auto path=baseline.w.maintained_range_path(baseline.source,100,400);check(path&&path->front().at==100&&path->back().at==400);
  check(baseline.box->drain().empty());check(baseline.w.npc_packets()==bodies);
  const auto after=baseline.w.npc_pursuit_movement_snapshot(baseline.source);check(after&&before&&before->at==after->at&&before->outcome.kind==after->outcome.kind&&before->outcome.travelled==after->outcome.travelled);
  // Fresh world for EVERY millisecond: direct materialization reuses production
  // integration once from the same100ms baseline, without float-rounding drift
  // from extra observation calls and without a second physics implementation.
  for(std::uint64_t at=200;at<=400;++at){Fixture oracle(kind);const auto actual=oracle.materialize(at),projected=interpolate(*path,at);
   check(std::hypot(actual.first-projected.first,actual.second-projected.second)<.0001);
  }
  auto stale=baseline.source;++stale.generation;check(!baseline.w.maintained_range_path(stale,100,400));
 }
 std::cout<<"PASS Flare/elemental/ordinary maintained projection vs per-ms materialization; no publication/mutation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
