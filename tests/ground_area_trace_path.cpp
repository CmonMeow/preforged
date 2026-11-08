#include "gw_layered_navigation.h"
#include "gw_ward_harm_exposure.h"
#include "gw_health_regeneration.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace navigation;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ground trace line "+std::to_string(at.line()));
}
int main(int argc,char**argv)try{
 check(argc==2);const std::filesystem::path path=argv[1];std::filesystem::create_directories(path.parent_path());
 std::ofstream file(path,std::ios::binary);auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)file.put(char(v>>(i*8)));};
 auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};file.write("FWNAV001",8);
 for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);f32(0);f32(0);f32(30);f32(10);u32(3);
 auto cell=[&](float left,float right){for(unsigned i=0;i<5;++i)u32(0xffffffff);f32(0);f32(10);f32(left);f32(right);f32(left);f32(right);};
 u32(2);cell(0,10);cell(20,30);u32(1);cell(10,20);u32(1);cell(0,30);file.close();
 const auto mesh=Mesh::load(path);const std::array<SurfaceLink,2> portals{{{{0,0},{1,0}},{{1,0},{0,1}}}};
 LayeredMesh linked(mesh,portals),unlinked(mesh);unsigned cases=0;
 const auto ward=*ward_against_harm({1,25,1},80,2,1,15,5,*ward_harm_plan(239,0),0);const std::array wards{ward};
 for(bool reverse:{false,true})for(float y:{2.f,5.f,8.f})for(float inset:{0.f,2.f}){
  const Point a{reverse?25-inset:5+inset,y},b{reverse?5+inset:25-inset,y};const Surface start{0,reverse?1u:0u};
  const auto end=linked.trace(start,a,b);const auto trace=linked.trace_path(start,a,b);check(end&&trace&&trace->size()>=4);
  check(trace->front().fraction==0&&trace->front().position.surface==start&&trace->back().fraction==1&&trace->back().position.surface==*end);
  const double duration=std::abs(b.x-a.x)*100.;std::vector<TimedGroundAreaLeg> legs;
  bool deck=false;float previous=0;
  for(std::size_t i=0;i<trace->size();++i){const auto& p=(*trace)[i];check(p.fraction>=previous&&p.fraction>=0&&p.fraction<=1);previous=p.fraction;deck|=p.position.surface.plane==1;
   check(std::abs(p.position.point.z-y)<1e-5&&std::abs(p.position.point.x-std::lerp(a.x,b.x,p.fraction))<.001);
   if(i+1<trace->size()){const auto& q=(*trace)[i+1];legs.push_back({{{1,30,1},2,unsigned(p.position.surface.plane),p.position.point.x,p.position.point.z,true,false},q.position.point.x,q.position.point.z,p.fraction*duration,q.fraction*duration});}
  }
  check(deck);const auto bands=ward_harm_regeneration_bands(wards,legs);check(bool(bands));double pip_ms=0;
  std::vector<skills::regeneration::ExternalPips> external;for(const auto& band:*bands){pip_ms+=(band.through-band.from)*band.pips;external.push_back({band.from,band.through,int(band.pips)});}
  check(std::abs(pip_ms-1000)<.03);skills::regeneration::State health({1,30,1},0);double hp=100;
  check(health.advance_external({1,30,1},std::uint64_t(duration),hp,1000,external).has_value());check(std::abs(hp-102)<.0001);++cases;
 }
 check(!unlinked.trace_path({0,0},{5,5},{25,5}));check(!linked.trace_path({0,0},{5,5},{25,5},true));
 check(!linked.trace_path({0,0},{5,5},{25,15}));++cases;
 {const auto trace=linked.trace_path({0,0},{10,5},{15,5});check(trace&&trace->back().position.surface==Surface{1,0});
  for(std::size_t i=1;i<trace->size();++i)check((*trace)[i].fraction>=(*trace)[i-1].fraction);++cases;}
 {const auto trace=linked.trace_path({1,0},{15,5},{15,5});check(trace&&trace->front().position.surface==Surface{1,0}&&trace->back().position.surface==Surface{1,0});++cases;}
 {const auto overlap_path=std::filesystem::path(path.string()+".overlap");std::ofstream f(overlap_path,std::ios::binary);
  const auto put=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)f.put(char(v>>(8*i)));};
  const auto number=[&](float v){put(std::bit_cast<std::uint32_t>(v));};f.write("FWNAV001",8);
  for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})put(n);number(0);number(0);number(30);number(10);put(3);
  const auto trap=[&](float left,float right){for(unsigned i=0;i<5;++i)put(0xffffffff);number(0);number(10);number(left);number(right);number(left);number(right);};
  put(2);trap(0,12);trap(20,30);put(1);trap(10,22);put(1);trap(0,30);f.close();
  const auto overlapping=Mesh::load(overlap_path);bool invalid_overlap=false;
  try{LayeredMesh invalid(overlapping,portals);}catch(const std::runtime_error&){invalid_overlap=true;}
  check(invalid_overlap);++cases; // Explicit links still require shared edges.
  f.open(overlap_path,std::ios::binary|std::ios::trunc);f.write("FWNAV001",8);
  for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})put(n);number(0);number(0);number(30);number(10);put(3);
  put(2);trap(0,12);trap(22,30);put(1);trap(12,22);put(1);trap(0,30);f.close();
  const auto edges=Mesh::load(overlap_path);LayeredMesh connected(edges,portals);
  for(bool reverse:{false,true}){const Point a{reverse?25.f:5.f,5},b{reverse?5.f:25.f,5};const Surface start{0,reverse?1u:0u};
   const auto trace=connected.trace_path(start,a,b);check(trace&&trace->size()>=4);
   for(float x:{7.f,11.98f,12.02f,15.f,21.98f,22.02f,23.f}){const Point point{x,5};
    const float fraction=(x-a.x)/(b.x-a.x);
    const auto prefix=connected.trace(start,a,point);check(prefix.has_value());Surface reported=trace->front().position.surface;
    for(const auto& step:*trace)if(step.fraction<=fraction)reported=step.position.surface;
    if(reported!=*prefix){
     std::cerr<<"reverse="<<reverse<<" x="<<x<<" reported="<<reported.plane<<":"<<reported.cell<<" prefix="<<prefix->plane<<":"<<prefix->cell<<" steps:";
     for(const auto& q:*trace)std::cerr<<" "<<q.fraction<<"="<<q.position.surface.plane<<":"<<q.position.surface.cell;
     std::cerr<<"\n";
    }
    check(reported==*prefix);
   }++cases;
  }
 }
 std::cout<<cases<<" collinear navigation trace cases preserve default reachability, bridge plane fractions and actual Ward/Health exposure across same-floor endpoints\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
