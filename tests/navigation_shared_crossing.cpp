#include "gw_layered_navigation.h"
#include <iostream>
using namespace fw::gw::navigation;
namespace {
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
Mesh fixture(const std::filesystem::path& path,float x,float y,float width,bool same_plane){
    std::ofstream out(path,std::ios::binary);
    const auto word=[&](std::uint32_t value){for(unsigned i=0;i<4;++i)out.put(char(value>>(8*i)));};
    const auto scalar=[&](float value){word(std::bit_cast<std::uint32_t>(value));};
    out.write("FWNAV001",8);for(auto value:{1u,7u,0xeefe704cu,12u,1u,1u})word(value);
    for(float value:{x-20,y-20,x+width+20,y+20})scalar(value);word(same_plane?2:3);
    const auto cell=[&](int neighbour,bool right,bool overlap=false){
        word(neighbour);for(unsigned i=0;i<4;++i)word(0xffffffffu);
        for(float value:{y+1,y,x,right||overlap?x+width:x,right?x+width:x,x+width})scalar(value);
    };
    if(same_plane){word(2);cell(1,false);cell(0,true);}
    else{word(1);cell(-1,false);word(1);cell(-1,true);}
    word(1);cell(-1,false,true);out.close();return Mesh::load(path);
}
void legal(const LayeredMesh& graph,const std::vector<SurfacePoint>& route){
    for(const auto& point:route)
        require(graph.straight_route(point.surface,point.point,point.surface,point.point).has_value(),
            "route published a point outside its named surface");
    for(std::size_t i=1;i<route.size();++i){const auto& a=route[i-1];const auto& b=route[i];
        require(graph.straight_route(a.surface,a.point,b.surface,b.point).has_value(),
            "route published an illegal shared crossing or corner segment");}
}
}
int main(int argc,char**argv)try{
    require(argc==3,"usage: navigation_shared_crossing_test generated-directory northlands-asset");
    const std::filesystem::path generated=argv[1];std::filesystem::create_directories(generated);
    unsigned cases=0,independent_rounding_failures=0;
    for(const auto base:{Point{10000,12000},Point{-6000,16000}})for(bool same_plane:{false,true}){
        auto mesh=fixture(generated/(std::to_string(cases)+".fwnav"),base.x,base.z,481,same_plane);
        require(mesh.discard_unverified_portals()==0,"synthetic genuine same-plane edges were pruned");
        const std::array<SurfaceLink,1> links{{{{0,0},{1,0}}}};
        const LayeredMesh graph=same_plane?LayeredMesh(mesh):LayeredMesh(mesh,links),unlinked(mesh);
        const Surface start{0,0},goal{same_plane?0u:1u,same_plane?1u:0u};
        for(unsigned i=0;i<8;++i)for(unsigned j=0;j<8;++j){
            const Point a{base.x+13+float(i),base.z+.03125f+float(i)*.05f},
                b{base.x+469-float(j),base.z+.5625f+float(j)*.05f};
            // Independent real-line intersection demonstrates the fixture's
            // float quantization hazard without prescribing a repaired point.
            const double dx=double(b.x)-a.x,dz=double(b.z)-a.z;
            const double t=(double(base.x)+481*(double(base.z)+1-a.z)-a.x)/(dx+481*dz);
            const Point raw{float(double(a.x)+t*dx),float(double(a.z)+t*dz)};
            if(!same_plane&&(!graph.straight_route(start,raw,start,raw)||!graph.straight_route(goal,raw,goal,raw)))
                ++independent_rounding_failures;
            for(bool reverse:{false,true}){
                const auto from=reverse?goal:start,to=reverse?start:goal;
                const auto first=reverse?b:a,last=reverse?a:b;
                const auto direct=graph.straight_route(from,first,to,last);
                require(direct.has_value(),"steep reciprocal crossing lost a legal straight route");legal(graph,*direct);
                const auto route=graph.route(from,first,to,last);
                require(route.has_value(),"steep reciprocal crossing lost a legal click route");legal(graph,*route);
                require(graph.trace(from,first,last)==to,"straight motion no longer crosses the genuine edge");
                ++cases;
            }
            if(!same_plane){
                require(!unlinked.straight_route(start,a,goal,b),"XY-adjacent unlinked floors gained a crossing");
                require(!unlinked.straight_route(start,a,goal,{base.x,base.z+1}),"a shared corner fabricated an unlinked floor crossing");
                require(!graph.route(start,a,{2,0},b),"unrelated overlapping floor gained a route");
            }
            const auto isolated=same_plane?Surface{1,0}:Surface{2,0};
            require(!graph.route(start,a,isolated,b),"unlinked overlapping rectangle gained an implicit edge");
            require(!graph.straight_route(start,a,goal,{base.x+490,base.z+.75f}),"quantization projected an off-mesh goal through a wall");
            require(!graph.trace(start,a,{base.x-1,base.z+.75f}),"quantization relaxed a real outer wall");
        }
    }
    require(independent_rounding_failures>0,"steep synthetic fixture did not reproduce independent float coordinate failure");
    auto mesh=Mesh::load(argv[2]);mesh.discard_unverified_portals();LayeredMesh graph(mesh);
    const Point lever{-5508,12787},entry{-5479,13797};
    const auto a=mesh.locate_cell(36,lever),b=mesh.locate_cell(36,entry);
    require(a&&b,"pinned Northlands exact approach endpoints missing");
    for(bool reverse:{false,true}){
        const auto route=reverse?graph.route({36,*b},entry,{36,*a},lever):graph.route({36,*a},lever,{36,*b},entry);
        require(route.has_value(),"pinned Northlands exact approach became unreachable");legal(graph,*route);
    }
    for(bool reverse:{false,true}){
        const Point a{-5600,12122.3f},b{-5320,12122.7f};
        const auto route=reverse?graph.straight_route({37,10},b,{36,20},a):graph.straight_route({36,20},a,{37,10},b);
        require(route.has_value(),"pinned steep cross-plane triangle lost its direct crossing");legal(graph,*route);
    }
    std::cout<<"PASS "<<cases<<" synthetic directed routes; "<<independent_rounding_failures
        <<" independently bad float crossings; same-plane/isolation/wall controls; pinned Northlands both directions\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
