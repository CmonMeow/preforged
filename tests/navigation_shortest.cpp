#include "gw_layered_navigation.h"
#include <iostream>
using namespace fw::gw::navigation;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
float route_length(const std::vector<SurfacePoint>& points){float sum=0;for(std::size_t i=1;i<points.size();++i)sum+=std::hypot(points[i].point.x-points[i-1].point.x,points[i].point.z-points[i-1].point.z);return sum;}
void alternate_corridors(const std::filesystem::path& path,unsigned scale){
 // Same rectangular obstacle, different cell subdivision. The independent
 // oracle is the shorter of the two paths touching its top/bottom corners.
 const unsigned width=12*scale,height=8*scale;
 std::map<std::pair<unsigned,unsigned>,unsigned> ids;
 for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)
  if(!(x>=5*scale&&x<7*scale&&y>=2*scale&&y<6*scale))ids[{x,y}]=unsigned(ids.size());
 std::ofstream out(path,std::ios::binary);
 const auto word=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(8*i)));};
 const auto scalar=[&](float v){word(std::bit_cast<std::uint32_t>(v));};
 out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})word(n);
 for(float v:{0.f,0.f,12.f,8.f})scalar(v);word(1);word(unsigned(ids.size()));
 // Emit in ID order (insertion order differs from std::map's sorting).
 std::vector<std::pair<unsigned,unsigned>> cells(ids.size());for(auto [xy,id]:ids)cells[id]=xy;
 for(auto [x,y]:cells){
  for(auto xy:{std::pair{x-1,y},std::pair{x+1,y},std::pair{x,y-1},std::pair{x,y+1}}){auto it=ids.find(xy);word(it==ids.end()?0xffffffff:it->second);}
  word(0xffffffff);
  for(float v:{float(y)/scale,float(y+1)/scale,float(x)/scale,float(x+1)/scale,float(x)/scale,float(x+1)/scale})scalar(v);
 }
 out.close();const auto mesh=Mesh::load(path);LayeredMesh graph(mesh);
 unsigned checked=0;
 for(float x:{.5f,2.5f,4.5f})for(float gx:{7.5f,9.5f,11.5f})for(float y:{2.3f,3.7f,5.7f})for(float gy:{2.3f,5.7f}){
  const Point a{x,y},b{gx,gy};auto start=mesh.locate_cell(0,a),goal=mesh.locate_cell(0,b);
  require(start&&goal,"synthetic endpoints exist");
  float expected=std::numeric_limits<float>::infinity();
  for(float edge:{2.f,6.f})expected=std::min(expected,std::hypot(x-5,y-edge)+2+std::hypot(gx-7,gy-edge));
  for(bool reverse:{false,true}){
   auto route=reverse?graph.route({0,*goal},b,{0,*start},a):graph.route({0,*start},a,{0,*goal},b);
   if(!route||std::abs(route_length(*route)-expected)>=.002f)std::cerr<<"oracle failure scale="<<scale<<" a="<<x<<','<<y<<" b="<<gx<<','<<gy<<" reverse="<<reverse<<" expected="<<expected<<" actual="<<(route?route_length(*route):-1)<<'\n';
   require(route&&std::abs(route_length(*route)-expected)<.002f,"alternative-corridor route must match independent geometric shortest-distance oracle in both directions");
   for(std::size_t i=1;i<route->size();++i)require(graph.straight_route(route->at(i-1).surface,route->at(i-1).point,route->at(i).surface,route->at(i).point).has_value(),"synthetic shortest route preserves surface connectivity");
   ++checked;
  }
 }
 std::cout<<"Independent rectangle-obstacle oracle: "<<checked<<" routes, subdivision="<<scale<<'\n';
}
int main(int argc,char** argv)try{
 require(argc==3,"fixture and Lion's Arch asset required");
 alternate_corridors(std::string(argv[1])+".alternatives",1);
 alternate_corridors(std::string(argv[1])+".subdivided",3);
 const auto straight=pull_string({1,1},{19,9},{{{10,10},{10,0}}});
 require(straight.size()==2,"straight corridor must not bend toward portal midpoint");
 const auto corner=pull_string({1,9},{11,19},{{{10,10},{10,0}},{{10,10},{20,10}}});
 require(corner.size()==3&&corner[1].x==10&&corner[1].z==10,"L corridor must turn at obstacle corner");
 std::ofstream out(argv[1],std::ios::binary);
 auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(8*i)));};
 auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
 out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
 for(float v:{0.f,0.f,20.f,20.f})f32(v);u32(1);u32(3);
 auto cell=[&](int n1,int n2,float l,float r,float low,float high){u32(n1);u32(n2);for(int i=0;i<3;++i)u32(0xffffffff);for(float v:{low,high,l,r,l,r})f32(v);};
 cell(1,-1,0,10,0,10);cell(0,2,10,20,0,10);cell(1,-1,10,20,10,20);out.close();
 auto mesh=Mesh::load(argv[1]);auto route=mesh.find_path(0,{1,9},{11,19});
 require(route&&route->waypoints.size()==3&&mesh.route_stays_walkable(0,*route),"mesh route uses walkable taut corridor");
 require(std::abs(route->length-2*std::sqrt(82.f))<.001f,"mesh route reaches analytic shortest L-corridor length");
 LayeredMesh layered(mesh);auto live=layered.route({0,0},{1,9},{0,2},{11,19});
 require(live&&live->size()==3&&live->at(1).point.x==10&&live->at(1).point.z==10,"runtime layered route uses corner rather than midpoints");
 auto town=Mesh::load(argv[2]);town.discard_unverified_portals();LayeredMesh town_graph(town);
 const Point origin{4797.79f,4034.57f},chest{9812.f,5147.f};
 auto from=town.locate_cell(0,origin),to=town.locate_cell(0,chest);
 require(from&&to,"logged map55 position and chest lie on navigation");
 RouteDiagnostics diagnostics;
 auto town_route=town_graph.route({0,*from},origin,{0,*to},chest,&diagnostics);
 require(town_route.has_value(),"logged map55 chest approach routes across stairs");
 float length=0;
 for(std::size_t i=1;i<town_route->size();++i){const auto a=town_route->at(i-1),b=town_route->at(i);
  require(town_graph.trace(a.surface,a.point,b.point).has_value(),"map55 shortest route remains collision traversable");
  length+=std::hypot(a.point.x-b.point.x,a.point.z-b.point.z);
 }
 std::cout<<"Map55 chest route length="<<length<<" points="<<town_route->size()<<'\n';
 std::cout<<"taut="<<diagnostics.taut<<" failed_portal="<<diagnostics.failed_portal<<" portals="<<diagnostics.portal_count<<'\n';
 std::cout<<"visibility_nodes="<<diagnostics.visibility_nodes<<" improved="<<diagnostics.visibility_improved<<'\n';
 require(diagnostics.taut,"real map55 chest route must not silently fall back to midpoints");
 require(diagnostics.visibility_improved&&length<6350.f,"map55 route must improve on the original 6402-unit cell corridor");
 const Point bilshan{5977.743164f,8938.856445f};auto bilshan_cell=town.locate_cell(35,bilshan);
 require(bilshan_cell.has_value(),"Brother Bilshan floor exists");
 auto direct=town_graph.trace({0,*to},chest,bilshan);
 std::cout<<"Chest to Bilshan direct trace="<<bool(direct)<<" plane="<<(direct?direct->plane:999)<<'\n';
 RouteDiagnostics bilshan_diagnostics;auto bilshan_route=town_graph.route({0,*to},chest,{35,*bilshan_cell},bilshan,&bilshan_diagnostics);
 require(bilshan_route.has_value(),"chest to Brother Bilshan route exists");
 float bilshan_length=0;for(std::size_t i=1;i<bilshan_route->size();++i)bilshan_length+=std::hypot(bilshan_route->at(i).point.x-bilshan_route->at(i-1).point.x,bilshan_route->at(i).point.z-bilshan_route->at(i-1).point.z);
 std::cout<<"Chest to Bilshan route="<<bilshan_length<<" direct="<<std::hypot(chest.x-bilshan.x,chest.z-bilshan.z)<<'\n';
 std::cout<<"Bilshan visibility nodes="<<bilshan_diagnostics.visibility_nodes<<" improved="<<bilshan_diagnostics.visibility_improved<<'\n';
 require(bilshan_diagnostics.visibility_improved&&bilshan_length<8000.f,
     "chest to Bilshan must not retain the original 11468-unit detour");
 require(bilshan_diagnostics.visibility_nodes>2048,"large route must not silently skip geometry search at the former node limit");
 for(std::size_t i=1;i<bilshan_route->size();++i){const auto a=bilshan_route->at(i-1),b=bilshan_route->at(i);
  require(town_graph.trace(a.surface,a.point,b.point).has_value(),"shortened Bilshan route remains collision traversable");}
 std::cout<<"PASS straight corridor, analytic L-corner shortest path and layered runtime route\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
