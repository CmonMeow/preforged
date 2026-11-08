#include "gw_layered_navigation.h"
#include <iostream>
using namespace fw::gw::navigation;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char**argv)try{
    if(argc!=4)throw std::runtime_error("fixture, bridge and stair asset paths required");
    std::ofstream out(argv[1],std::ios::binary);
    const auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(i*8)));};
    const auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
    for(float v:{0.f,0.f,10.f,20.f})f32(v);u32(3);
    const auto cell=[&](int neighbour,float yt,float yb,float xtl,float xtr,float xbl,float xbr){
        u32(neighbour);for(int i=0;i<4;++i)u32(-1);
        for(float v:{yt,yb,xtl,xtr,xbl,xbr})f32(v);
    };
    // Two zero-height portal cells, with opposite collapsed endpoints, as in
    // retail PATH_INFO. Their explicit pairing connects ground to bridge.
    u32(2);cell(1,0,10,0,10,0,10);cell(0,10,10,0,10,0,0);
    u32(2);cell(1,10,10,10,10,0,10);cell(0,10,20,0,10,0,10);
    u32(1);cell(-1,0,20,0,10,0,10);out.close();
    auto mesh=Mesh::load(argv[1]);
    require(mesh.discard_unverified_portals()==0,"connector neighbours must survive pruning");
    const std::array<SurfaceLink,1> links{{{{0,1},{1,0}}}};
    LayeredMesh graph(mesh,links),unlinked(mesh);
    require(graph.trace({0,0},{5,5},{5,15})==Surface{1,1},"cross zero-height connector chain");
    require(graph.trace({1,1},{5,15},{5,5})==Surface{0,0},"reverse connector chain");
    require(!unlinked.trace({0,0},{5,5},{5,15}),"no implicit bridge across touching geometry");
    require(!graph.trace({0,0},{5,5},{15,15}),"connector does not permit crossing side walls");
    require(!graph.route({0,0},{5,5},{2,0},{5,15}),"unrelated overlap stays isolated");
    auto route=graph.route({0,0},{5,5},{1,1},{5,15});require(bool(route),"click route across connectors");
    for(std::size_t i=1;i<route->size();++i){
        auto a=(*route)[i-1],b=(*route)[i];
        require(bool(graph.trace(a.surface,a.point,b.point)),"route segments including connectors traversable");
    }
    auto retail=Mesh::load(argv[2]);retail.discard_unverified_portals();LayeredMesh live(retail);
    for(auto test: {std::array<float,4>{-6100.08f,-2877.01f,-6105,-2904},
                   std::array<float,4>{-6067.98f,-2227.15f,-6066,-2200}}){
        Point a{test[0],test[1]},b{test[2],test[3]};auto start=retail.locate_cell(0,a);
        require(bool(start),"logged start is on ground");
        auto end=live.trace({0,*start},a,b);require(bool(end)&&end->plane!=0,"logged blocked bridge step now crosses");
        require(live.trace(*end,b,a)==Surface{0,*start},"logged bridge step reversible");
    }
    // User subsequently traversed the entrance and stopped in the temple
    // courtyard. Cover the whole connector/deck/ground chain, not just entry.
    const Point entrance{-6100.08f,-2877.01f},courtyard{-6085.01f,-5112.59f};
    const Surface entry{0,*retail.locate_cell(0,entrance)},exit{0,*retail.locate_cell(0,courtyard)};
    require(live.trace(entry,entrance,courtyard)==exit,"full bridge reaches temple courtyard");
    require(live.trace(exit,courtyard,entrance)==entry,"full bridge reverse traversal");
    auto stairs=Mesh::load(argv[3]);stairs.discard_unverified_portals();LayeredMesh stair_graph(stairs);
    for(float angle:{48.f,48.5f,49.f})for(float seconds:{.05f,.1f,.125f}){
        SurfacePoint current{{7081.05f,4619.13f},{20,*stairs.locate_cell(20,{7081.05f,4619.13f})}};
        const float radians=angle*3.14159265358979323846f/180.f;
        for(unsigned n=0;n<20;++n){
            auto motion=stair_graph.slide(current.surface,current.point,
                {current.point.x+220*seconds*std::cos(radians),current.point.z+220*seconds*std::sin(radians)},.33f,.01f,true);
            if(!motion||motion->blocked){std::cerr.precision(9);std::cerr<<"stair failure angle="<<angle<<" seconds="<<seconds<<" step="<<n<<" from="<<current.point.x<<','<<current.point.z<<" plane="<<current.surface.plane;
                if(motion)std::cerr<<" end="<<motion->end.point.x<<','<<motion->end.point.z<<" plane="<<motion->end.surface.plane;
                std::cerr<<'\n';}
            require(motion&&!motion->blocked,"wall slide resumes original input across connected stair tip without a false stop");
            require(std::hypot(motion->end.point.x-current.point.x,motion->end.point.z-current.point.z)<=220*seconds+.02f,
                "resuming input never exceeds the remaining motion budget");
            current=motion->end;
        }
        require(current.surface.plane==0,"sliding exits the stair layer onto connected ground");
    }
    std::cout<<"PASS zero-height connectors, bridge isolation and stair-tip continuation\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
