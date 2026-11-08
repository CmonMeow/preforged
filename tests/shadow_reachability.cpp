#include "gw_layered_navigation.h"
#include <fstream>
#include <iostream>
using namespace fw::gw::navigation;
void check(bool ok){if(!ok)throw std::runtime_error("bounded reachability");}
int main(int argc,char** argv)try{
 if(argc!=2)throw std::runtime_error("fixture path required");
 const auto parent=std::filesystem::path(argv[1]).parent_path();if(!parent.empty())std::filesystem::create_directories(parent);
 std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("fixture output unavailable");const auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(i*8)));};const auto f32=[&](float f){u32(std::bit_cast<std::uint32_t>(f));};
 out.write("FWNAV001",8);for(auto v:{1u,7u,0xeefe704cu,24u,1u,1u})u32(v);f32(-10);f32(-10);f32(40);f32(110);u32(2);
 const auto cell=[&](float l,float r,float low,float high,std::array<int,4> n){for(auto v:n)u32(v);u32(0xffffffff);f32(low);f32(high);f32(l);f32(r);f32(l);f32(r);};
 u32(5);cell(-10,10,-10,10,{2,-1,-1,-1});cell(30,40,-10,10,{4,-1,-1,-1});
 cell(-10,10,10,100,{0,3,-1,-1});cell(10,40,90,110,{2,4,-1,-1});cell(30,40,10,90,{3,1,-1,-1});
 u32(1);cell(-10,40,-10,110,{-1,-1,-1,-1});out.close();
 const auto mesh=Mesh::load(argv[1]);LayeredMesh nav(mesh);const Surface start{0,0},goal{0,1};const Point a{0,0},b{35,0};
 check(bool(nav.route(start,a,goal,b))); // Connected by a long U-shaped detour.
 check(nav.reachable_within(start,a,goal,b,50)==false); // Endpoints alone being in range is insufficient.
 check(nav.reachable_within(start,a,goal,b,120)==true); // Walking distance exceeds120;range bounds positions,not distance.
 check(nav.reachable_within(start,a,{1,0},b,120)==false); // XY overlap does not connect floors.
 const std::array<SurfaceLink,1> gate{{{{0,2},{0,3}}}};
 check(nav.reachable_within(start,a,goal,b,120,gate)==false);
 const std::array<SurfaceLink,1> reverse{{{{0,3},{0,2}}}};check(nav.reachable_within(start,a,goal,b,120,reverse)==false);
 check(!nav.reachable_within(start,a,goal,b,-1));check(!nav.reachable_within({9,0},a,goal,b,120));
 check(nav.reachable_within(start,a,start,{5,5},10)==true);check(nav.reachable_within(start,a,start,{9,9},10)==false);
 check(!nav.reachable_within(start,a,start,{10.005f,0},120)); // Trace crossing tolerance is not landing clearance.
 check(!nav.reachable_within(start,{10.005f,0},start,a,120));
 const auto wall=nav.straight_prefix(start,a,b);check(wall&&wall->clipped&&!wall->blocked&&wall->end.point.x<=10.0001f&&wall->end.point.x>9.999f&&wall->end.point.z==0);
 const auto diagonal=nav.straight_prefix(start,a,{35,5});check(diagonal&&diagonal->clipped&&diagonal->end.point.x<=10.0001f);
 check(std::abs(diagonal->end.point.z-diagonal->end.point.x/7)<.0001f); // No tangent travel after contact.
 const auto open=nav.straight_prefix(start,a,{0,50});check(open&&!open->clipped&&open->end.surface==Surface{0,2});
 const std::array<SurfaceLink,1> closed{{{{0,0},{0,2}}}};
 const auto shut=nav.straight_prefix(start,a,{0,50},closed);check(shut&&shut->clipped&&shut->end.point.z<=10.0001f&&shut->end.point.z>9.999f);
 const auto head_on=nav.straight_prefix(start,{10,0},{35,0});check(head_on&&head_on->clipped&&head_on->blocked&&head_on->end.point.x==10);
 const auto stay=nav.straight_prefix(start,a,a);check(stay&&!stay->clipped&&!stay->blocked);
 check(!nav.straight_prefix({9,0},a,b));
 const auto jump=nav.shadow_destination(start,a,goal,b,120);check(jump&&!jump->clipped&&jump->end.surface==goal&&jump->end.point.x==35);
 const auto limited=nav.shadow_destination(start,a,goal,b,50);check(limited&&limited->clipped&&limited->end.point.x<=10.0001f);
 const auto gated=nav.shadow_destination(start,a,goal,b,120,gate);check(gated&&gated->clipped&&gated->end.point.x<=10.0001f);
 const auto overlap=nav.shadow_destination(start,a,{1,0},b,120);check(overlap&&overlap->clipped&&overlap->end.surface.plane==0);
 check(!nav.shadow_destination(start,a,{9,0},b,120));
 check(!nav.shadow_destination(start,a,goal,b,10)); // Cannot turn an out-of-range target into unrestricted movement.
 const auto wrong_floor=nav.shadow_destination(start,a,{1,0},{5,5},120);
 check(wrong_floor&&wrong_floor->clipped&&!wrong_floor->blocked&&wrong_floor->end.surface==start);
 const auto stacked=nav.shadow_destination(start,a,{1,0},a,120);
 check(stacked&&stacked->clipped&&stacked->blocked&&stacked->end.surface==start);
 std::cout<<"PASS disk-constrained topology:detour bounds,not path length,overlapping planes,closed edges,invalid geometry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
