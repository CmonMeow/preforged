// Independent pre/post acceptance harness: nominal diagnostics and exact lifecycle authority.
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
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope player,npc_scope,maintainer,recipient;
 SteeringFixture(bool npc_maintains,bool turning,bool ward,bool lease){
  auto body=npc(100,0);body.combat_definition->relation=combat::npcs::Relation::allied_to_players;body.combat_definition->packed_allegiance=6;w.configure_npcs({body});
  w.join(1,"Steering actor",500,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
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
 unsigned cases=0;
 for(bool npc_maintains:{false,true})for(bool turning:{false,true})for(bool ward:{false,true})for(bool poison_boundary:{false,true}){
  constexpr std::uint64_t horizon=20100;SteeringFixture fixture(npc_maintains,turning,ward,true);
  const auto entry=fixture.w.status_snapshot(fixture.recipient,100)->front();const auto wire=*mending_end_wire(fixture.maintainer,fixture.recipient,entry);
  if(poison_boundary){fixture.w.join(3,"Poison author",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);fixture.w.set_vitals(3,{1000,1000,100,100},100);
   check(fixture.w.apply_condition(*fixture.w.effect_scope(3),fixture.player,combat::conditions::Kind::poison,100,2000));}
  fixture.box->drain();
  const auto a=fixture.w.maintained_range_path(fixture.maintainer,100,horizon),b=fixture.w.maintained_range_path(fixture.recipient,100,horizon);check(a&&b);
  const auto nominal=maintained_range_exit(*a,*b,100,horizon);check(bool(nominal));check(fixture.box->drain().empty());
  check(fixture.w.position(1,100)==std::make_pair(500.f,0.f));
  check(fixture.w.maintained_enchantment_count(fixture.maintainer,100)==1);
  check(fixture.w.status_snapshot(fixture.recipient,100)->front().instance==entry.instance);
  // Public knots describe ONE outer-horizon nominal trajectory, not exact
  // authority at arbitrary queries using a DIFFERENT turning subdivision.
  // Its endpoint at that SAME horizon must still match shared materialization.
  SteeringFixture endpoint_oracle(npc_maintains,turning,ward,false);const auto endpoint=endpoint_oracle.materialize(horizon);
  const auto& moving=npc_maintains?*b:*a;const double endpoint_error=std::hypot(moving.back().x-endpoint.first,moving.back().y-endpoint.second);
  const auto outside=[&](std::uint64_t at){SteeringFixture oracle(npc_maintains,turning,ward,false);const auto point=oracle.materialize(at);return std::hypot(double(point.first),double(point.second))>5000.;};
  check(!outside(100)&&outside(horizon));std::uint64_t low=100,exact=horizon;
  while(exact-low>1){const auto middle=low+(exact-low)/2;if(outside(middle))exact=middle;else low=middle;}
  std::cout<<"npc_maintains="<<npc_maintains<<" turn="<<turning<<" ward="<<ward<<" poison="<<poison_boundary<<" same_horizon_endpoint_error="<<endpoint_error<<" nominal_exit="<<(nominal->first_outside?std::to_string(*nominal->first_outside):"none")<<" exact_exit="<<exact<<'\n';
  // This assertion isolates the old Ward miss without demanding that nominal
  // intermediate interpolation equal another horizon's actual turn physics.
  check(endpoint_error<.002);
  check(fixture.w.settle_resources(fixture.maintainer,exact-1));check(fixture.w.maintained_enchantment_count(fixture.maintainer,exact-1)==1);
  check(fixture.w.settle_resources(fixture.maintainer,exact));check(fixture.w.maintained_enchantment_count(fixture.maintainer,exact)==0);
  const auto packets=fixture.box->drain();check(std::count(packets.begin(),packets.end(),wire.effect)==1);
  check(std::count(packets.begin(),packets.end(),wire.upkeep)==(npc_maintains?0:1));
  const auto after=fixture.w.status_snapshot(fixture.recipient,exact);check(after&&std::none_of(after->begin(),after->end(),[&](const auto& live){return live.instance==entry.instance;}));
  check(fixture.w.settle_resources(fixture.maintainer,exact+1000)&&fixture.w.maintained_enchantment_count(fixture.maintainer,exact+1000)==0);
  const auto repeated=fixture.box->drain();check(std::count(repeated.begin(),repeated.end(),wire.effect)==0);
  ++cases;
 }
 std::cout<<"PASS "<<cases<<" independent exact steering lifecycle consumers: both player/NPC directions, held turns, Ward177, earlier poison boundary\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
