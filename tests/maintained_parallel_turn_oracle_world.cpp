// Detached diagnostic: nominal trajectory scope vs actual held-input materialization.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Parallel turn oracle line "+std::to_string(at.line()));}
struct Fixture {
 world::Instance w;Scope a,b;
 explicit Fixture(bool ward){
  for(unsigned id:{1u,2u}){w.join(id,id==1?"Turn A":"Turn B",id==1?0.f:5000.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{1000,1000,100,100},100);}
  a=*w.effect_scope(1);b=*w.effect_scope(2);
  if(ward){w.join(3,"Ward author",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);const auto author=*w.effect_scope(3);
   check(w.apply_ward_foes(author,*ward_foes_plan(0),100));check(w.kill(author,100));}
  for(unsigned id:{1u,2u}){w.steer(id,1,0,100);w.rotate(id,std::numeric_limits<float>::infinity(),.01f,100);}
 }
 std::pair<float,float> materialize(unsigned id,std::uint64_t at){w.rotate(id,0,0,at);return w.position(id,at);}
};
std::pair<double,double> sample(const std::vector<MaintainedRangePoint>& path,std::uint64_t at){
 const auto found=std::lower_bound(path.begin(),path.end(),at,[](const auto& point,auto time){return point.at<time;});check(found!=path.end());if(found->at==at)return {found->x,found->y};
 check(found!=path.begin());const auto& prior=*std::prev(found);const double t=double(at-prior.at)/double(found->at-prior.at);return {std::lerp(prior.x,found->x,t),std::lerp(prior.y,found->y,t)};
}
int main()try{
 unsigned mismatches=0;
 for(bool ward:{false,true}){Fixture baseline(ward);const auto a=baseline.w.maintained_range_path(baseline.a,100,10100),b=baseline.w.maintained_range_path(baseline.b,100,10100);check(a&&b);
  for(auto at:{1000ULL,3000ULL,7000ULL,10100ULL}){Fixture oracle(ward);const auto actual_a=oracle.materialize(1,at),actual_b=oracle.materialize(2,at);const auto nominal_a=sample(*a,at),nominal_b=sample(*b,at);
   const auto error_a=std::hypot(double(actual_a.first)-nominal_a.first,double(actual_a.second)-nominal_a.second),error_b=std::hypot(double(actual_b.first)-nominal_b.first,double(actual_b.second)-nominal_b.second);
   std::cout<<"parallel ward="<<ward<<" at="<<at<<" errors="<<error_a<<','<<error_b<<" nominal_distance="<<std::hypot(nominal_a.first-nominal_b.first,nominal_a.second-nominal_b.second)<<" actual_distance="<<std::hypot(double(actual_a.first)-actual_b.first,double(actual_a.second)-actual_b.second)<<'\n';
   if(error_a>.002||error_b>.002)++mismatches;
  }
 }
 // Intentionally strict diagnostic; do not register as a passing checkpoint
 // while the public path still promises actual motion but omits Ward slicing.
 check(mismatches==0);std::cout<<"PASS parallel turning trajectories match actual World materialization\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
