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
 explicit Fixture(unsigned kind,unsigned seed=0){
  const float goal_x=1400.f+float(seed%7)*53.f,goal_y=float(int(seed%9)-4)*31.f;
  world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding={160,100,100,0,0,0,0,1};d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;d.damageable=d.conditions_allowed=true;n.combat_definition=d;
  w.configure_npcs({n});w.join(1,"Pursuit target",goal_x,goal_y,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  source=*w.effect_scope(100);const auto owner=*w.effect_scope(1);
  check(w.physical_armor(owner,0)&&w.elemental_armor(owner,0,0));
  combat::ai::FlarePolicy p;p.source=source;p.target=owner;p.level=1;p.speed=50.f+float(seed%11)*13.f;p.acquire=2000;p.follow=2400;p.leash=3000;
  p.flight={1000,32};p.evidence="synthetic controller maintained-projection oracle";
  if(kind==0)check(w.configure_npc_flare_ai(p,100));
  if(kind==1){check(w.configure_npc_elemental_ai(p,{{{214,0},*skills::effects::water_projectile(214,0),p.evidence}},4,100));check(w.observe_npc_overcast(source,0,100,p.evidence));}
  if(kind==2){combat::ai::ordinary::Reviewed a{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};check(w.configure_npc_hostile_ordinary(p,a,100));}
  w.tick(100);check(w.npc_pursuit_movement_snapshot(source).has_value());
  if(seed%2){
   w.join(2,"Ward author",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);
   const auto caster=*w.effect_scope(2);check(w.apply_ward_foes(caster,*skills::effects::ward_foes_plan(0),100));check(w.kill(caster,100));
  }
  box->drain();
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
 unsigned checks=0;
 for(unsigned kind=0;kind<3;++kind)for(unsigned seed=0;seed<32;++seed){
  Fixture baseline(kind,seed);const auto path=baseline.w.maintained_range_path(baseline.source,100,10000);check(bool(path));
  for(unsigned observation=0;observation<9;++observation){
   const std::uint64_t at=200+(std::uint64_t(seed*6364136223846793005ULL+observation*1442695040888963407ULL)%9801);
   Fixture oracle(kind,seed);const auto actual=oracle.materialize(at),predicted=interpolate(*path,at);
   if(std::hypot(actual.first-predicted.first,actual.second-predicted.second)>=.0002)
    throw std::runtime_error("projection mismatch kind="+std::to_string(kind)+" seed="+std::to_string(seed)+" time="+std::to_string(at));
   ++checks;
  }
  if(seed==0){
   // The existing shared integrator supports integer clocks through2^53-1;
   // projection must retain that horizon without a60s cap or dense history.
   const auto long_path=baseline.w.maintained_range_path(baseline.source,100,9007199254740991ULL);
   check(long_path&&long_path->back().at==9007199254740991ULL&&long_path->size()<1000);
  }
  check(baseline.box->drain().empty());
 }
 std::cout<<"PASS "<<checks<<" deterministic randomized World/shared-integrator observations through stop/leash horizons\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
