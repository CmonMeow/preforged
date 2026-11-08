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
struct SteeringFixture {
 world::Instance w;Scope player,npc_scope,maintainer,recipient;
 SteeringFixture(bool npc_maintains,bool turning,bool ward,bool lease){
  auto body=npc(100,0);body.combat_definition->relation=combat::npcs::Relation::allied_to_players;body.combat_definition->packed_allegiance=6;w.configure_npcs({body});
  w.join(1,"Steering actor",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  player=*w.effect_scope(1);npc_scope=*w.effect_scope(100);maintainer=npc_maintains?npc_scope:player;recipient=npc_maintains?player:npc_scope;
  const combat::ai::ordinary::Reviewed weapon{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},"synthetic standing friendly ordinary oracle"};
  check(w.configure_npc_opposed_ordinary(npc_scope,weapon,100));
  if(lease)check(w.stage_retribution(maintainer,recipient,*retribution(12),100));
  if(ward){w.join(2,"Ward author",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
   const auto author=*w.effect_scope(2);check(w.apply_ward_foes(author,*ward_foes_plan(0),100));check(w.kill(author,100));}
  w.steer(1,1,0,100);if(turning)w.rotate(1,std::numeric_limits<float>::infinity(),.01f,100);
 }
 std::pair<float,float> materialize(std::uint64_t at){
  // A real input observation materializes the old held vector/turn BEFORE
  // changing its turn rate. No maintained lease is installed in oracle worlds,
  // so the behavior being checked cannot influence its own reference motion.
  w.rotate(1,0,0,at);return w.position(1,at);
 }
};
std::pair<double,double> interpolated(const std::vector<MaintainedRangePoint>& path,std::uint64_t at){
 const auto found=std::lower_bound(path.begin(),path.end(),at,[](const auto& p,auto time){return p.at<time;});check(found!=path.end());
 if(found->at==at)return {found->x,found->y};check(found!=path.begin());const auto& before=*std::prev(found);
 const double t=double(at-before.at)/double(found->at-before.at);return {std::lerp(before.x,found->x,t),std::lerp(before.y,found->y,t)};
}
int main()try{
 unsigned mismatched_positions=0,mismatched_crossings=0;
 for(bool npc_maintains:{false,true})for(unsigned mode=0;mode<3;++mode){const bool turning=mode!=0,ward=mode==2;constexpr std::uint64_t horizon=20100;
  SteeringFixture fixture(npc_maintains,turning,ward,true);
  const auto caster=fixture.w.maintained_range_path(fixture.maintainer,100,horizon),recipient=fixture.w.maintained_range_path(fixture.recipient,100,horizon);
  check(caster&&recipient);const auto projected=maintained_range_exit(*caster,*recipient,100,horizon);check(projected&&projected->first_outside);
  const auto& moving=npc_maintains?*recipient:*caster;
  double maximum_error=0;
  for(auto at:{1000ULL,3000ULL,7000ULL,13000ULL,17000ULL,20100ULL}){
   SteeringFixture oracle(npc_maintains,turning,ward,false);const auto actual=oracle.materialize(at),nominal=interpolated(moving,at);
   const auto error=std::hypot(double(actual.first)-nominal.first,double(actual.second)-nominal.second);maximum_error=std::max(maximum_error,error);
   if(error>.002)++mismatched_positions;
  }
  const auto outside=[&](std::uint64_t at){SteeringFixture oracle(npc_maintains,turning,ward,false);const auto point=oracle.materialize(at);return std::hypot(double(point.first),double(point.second))>5000.;};
  check(!outside(100)&&outside(horizon));std::uint64_t low=100,exact=horizon;
  while(exact-low>1){const auto middle=low+(exact-low)/2;if(outside(middle))exact=middle;else low=middle;}
  if(exact!=*projected->first_outside)++mismatched_crossings;
  std::cout<<"npc_maintains="<<npc_maintains<<" mode="<<mode<<" max_position_error="<<maximum_error<<" projected_cross="<<*projected->first_outside<<" actual_cross="<<exact<<'\n';
  // Integration consumer must retain through the last actually-inside tick,
  // and retire the real lease at the first materialized-float outside tick.
  check(fixture.w.settle_resources(fixture.maintainer,exact-1));check(fixture.w.maintained_enchantment_count(fixture.maintainer,exact-1)==1);
  check(fixture.w.settle_resources(fixture.maintainer,exact));check(fixture.w.maintained_enchantment_count(fixture.maintainer,exact)==0);
 }
 check(mismatched_positions==0&&mismatched_crossings==0);
 std::cout<<"PASS steering/player-NPC exact materialization and maintained range (turns+Ward177)\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
