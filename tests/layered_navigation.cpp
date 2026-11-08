#include "gw_layered_navigation.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <sstream>
using namespace fw::gw::navigation;
void require(bool good,const char* why){if(!good)throw std::runtime_error(why);}
std::uint16_t plane_word(const fw::gw::world::Packet& packet,std::size_t offset){
    return std::uint16_t(packet.at(offset))|std::uint16_t(packet.at(offset+1))<<8;
}
const fw::gw::world::Packet& destination(const std::vector<fw::gw::world::Packet>& packets){
    for(const auto& packet:packets)if(packet.at(0)==41)return packet;
    throw std::runtime_error("expected movement destination");
}
int main(int argc,char**argv)try{
    if(argc!=2)throw std::runtime_error("fixture path required");
    const auto links_path=std::filesystem::path(std::string(argv[1])+".links");
    std::filesystem::remove(links_path);
    std::ofstream out(argv[1],std::ios::binary);
    auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(8*i)));};
    auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
    f32(0);f32(0);f32(30);f32(10);u32(3);
    auto cell=[&](float left,float right){for(int i=0;i<5;++i)u32(0xffffffff);f32(0);f32(10);f32(left);f32(right);f32(left);f32(right);};
    u32(2);cell(0,10);cell(20,30); // disconnected ground banks
    u32(1);cell(10,20);           // bridge deck
    u32(1);cell(0,30);           // unrelated overlapping surface: never reachable
    out.close();const auto mesh=Mesh::load(argv[1]);
    {
        const auto fixture=std::filesystem::absolute(argv[1]);
        require(catalog_asset_path(fixture.filename().string()+"\r",fixture.parent_path())==fixture,
            "spawn and portal catalog paths handle relative assets and CRLF");
        std::istringstream relative("55="+fixture.filename().string()+"\n");
        const auto loaded=load_catalog(relative,fixture.parent_path());
        require(loaded.size()==1&&loaded.contains(55),"catalog-relative asset resolves independently of working directory");
        std::istringstream legacy("55="+fixture.string()+"\n");
        require(load_catalog(legacy).contains(55),"legacy absolute catalog still loads");
        std::istringstream unanchored("55="+fixture.filename().string()+"\n");
        bool rejected=false;try{load_catalog(unanchored);}catch(const std::runtime_error&){rejected=true;}
        require(rejected,"relative catalog never silently uses process working directory");
    }
    const std::array<SurfaceLink,2> portals{{{{0,0},{1,0}},{{1,0},{0,1}}}};
    LayeredMesh linked(mesh,portals),unlinked(mesh);
    for(float x:{1.f,5.f,9.f})for(float z:{1.f,5.f,9.f})
    for(float gx:{21.f,25.f,29.f})for(float gz:{1.f,5.f,9.f}){
        const Point a{x,z},b{gx,gz};
        for(bool reverse:{false,true}){
            const auto direct=reverse?linked.route({0,1},b,{0,0},a):linked.route({0,0},a,{0,1},b);
            require(direct.has_value(),"all off-center bridge pairs have a route");
            float length=0;bool deck=false;
            for(std::size_t i=1;i<direct->size();++i){const auto previous=direct->at(i-1),next=direct->at(i);
                length+=std::hypot(next.point.x-previous.point.x,next.point.z-previous.point.z);
                deck|=next.surface.plane==1;
                require(linked.straight_route(previous.surface,previous.point,next.surface,next.point).has_value(),"all shortest bridge segments retain verified connections");
            }
            require(deck&&std::abs(length-std::hypot(gx-x,gz-z))<.001f,"bridge paths match independent straight-distance oracle across arbitrary floor crossing positions");
        }
    }
    require(!linked.straight_route({0,0},{5,5},{2,0},{25,5}),"visibility cannot connect an unrelated overlapping floor");
    const auto side_route=linked.route({0,0},{5,2},{0,1},{25,2});
    require(side_route.has_value(),"off-center bridge route exists");
    for(const auto& point:*side_route)require(std::abs(point.point.z-2.f)<.001f,
        "wide floor connectors retain shortest off-center crossing rather than forcing midpoint");
    const auto reverse_side=linked.route({0,1},{25,8},{0,0},{5,8});
    require(reverse_side.has_value(),"reverse off-center bridge route exists");
    for(const auto& point:*reverse_side)require(std::abs(point.point.z-8.f)<.001f,
        "reverse floor connectors do not pull toward their midpoint");
    require(!unlinked.trace({0,0},{5,5},{25,5}),"overlap must not invent a bridge");
    require(linked.trace({0,0},{5,5},{25,5})==Surface{0,1},"cross both bridge portals");
    require(linked.trace({0,0},{5,5},{15,5})==Surface{1,0},"enter bridge plane");
    require(linked.trace({1,0},{15,5},{5,5})==Surface{0,0},"leave bridge in reverse");
    require(!linked.trace({0,0},{5,5},{25,15}),"bridge motion cannot cross outer wall");
    require(!linked.route({0,0},{5,5},{2,0},{25,5}),"unrelated overlapping plane inaccessible");
    auto route=linked.route({0,0},{5,5},{0,1},{25,5});require(route.has_value(),"click route across bridge");
    bool bridge=false;
    for(std::size_t i=1;i<route->size();++i){
        const auto prev=(*route)[i-1],next=(*route)[i];bridge|=next.surface.plane==1;
        if(prev.surface==next.surface)
            require(linked.trace(prev.surface,prev.point,next.point).has_value(),"every route segment traversable");
        else require(std::hypot(prev.point.x-next.point.x,prev.point.z-next.point.z)<.001f,"plane switch occurs at shared boundary");
    }
    require(bridge,"route uses actual bridge surface");
    const auto across=linked.slide({0,0},{5,5},{25,5});
    require(across&&!across->clipped&&across->end.surface==Surface{0,1},"unobstructed sweep retains bridge connectors");
    {
        // Minimal three-cell reproduction of Lion's Arch's layer-20 corner.
        // The wall and connector share an edge ending at the destination
        // triangle's tip; a tolerance-padded contact misses that connector.
        const auto tip_path=std::filesystem::path(std::string(argv[1])+".tip");
        std::ofstream fixture(tip_path,std::ios::binary);
        const auto word=[&](std::uint32_t value){for(unsigned i=0;i<4;++i)fixture.put(char(value>>(8*i)));};
        const auto scalar=[&](float value){word(std::bit_cast<std::uint32_t>(value));};
        const auto cell=[&](int neighbour,std::array<float,6> shape){
            word(std::bit_cast<std::uint32_t>(neighbour));for(unsigned i=0;i<4;++i)word(0xffffffff);
            for(float value:shape)scalar(value);
        };
        fixture.write("FWNAV001",8);for(auto value:{1u,7u,0xeefe704cu,12u,1u,1u})word(value);
        for(float value:{6200.f,4200.f,7300.f,5000.f})scalar(value);word(2);
        word(1);cell(-1,{4695,4646,7025.681640625f,7241,7099,7099});
        word(2);cell(1,{4646,4226,6514.60888671875f,7099,6234,6818.39111328125f});
        cell(0,{4916,4646,6695,6695,6514.60888671875f,7099});
        fixture.close();const auto tip_mesh=Mesh::load(tip_path);
        const std::array<SurfaceLink,1> tip_links{{{{1,1},{0,0}}}};
        LayeredMesh tip(tip_mesh,tip_links);SurfacePoint actor{{7023.88f,4533.55f},{1,0}};
        bool crossed=false;
        for(unsigned step=0;step<20;++step){
            const auto motion=tip.slide(actor.surface,actor.point,{actor.point.x+22,actor.point.z},.33f);
            require(motion&&!motion->blocked,"sliding must not strand the actor beyond a connector tip");
            actor=motion->end;
            require(tip.trace(actor.surface,actor.point,actor.point).has_value(),"wall-tip contact remains valid on its recorded layer");
            if(actor.surface.plane==0){crossed=true;break;}
        }
        require(crossed,"sliding reaches the connected layer after passing the wall tip");
        const auto beyond=tip.slide(actor.surface,actor.point,{actor.point.x+22,actor.point.z},.33f);
        require(beyond&&!beyond->blocked&&beyond->end.point.x>7099&&beyond->end.point.z>4646,
                "next input continues beyond the tip instead of returning to it");
        // Second reported corner: the outward slide follows the destination
        // triangle's wall, not either wall of the starting triangle.
        fixture.open(tip_path,std::ios::binary|std::ios::trunc);
        fixture.write("FWNAV001",8);for(auto value:{1u,7u,0xeefe704cu,12u,1u,1u})word(value);
        for(float value:{8200.f,3400.f,8900.f,3800.f})scalar(value);word(2);
        word(1);cell(-1,{3704,3602,8760,8760,8647.0712890625f,8847});
        word(1);cell(-1,{3704,3480,8308.982421875f,8760,8512,8512});
        fixture.close();const auto shared_mesh=Mesh::load(tip_path);
        const std::array<SurfaceLink,1> shared_links{{{{1,0},{0,0}}}};
        LayeredMesh shared(shared_mesh,shared_links),isolated(shared_mesh);
        const Point corner{8760,3704},input{8782,3712.14f};
        const auto crossing=shared.slide({1,0},corner,input,.33f);
        require(crossing&&!crossing->blocked&&crossing->end.surface.plane==0&&
                crossing->end.point.x>corner.x&&crossing->end.point.z<corner.z,
                "shared corner uses the valid tangent on its connected destination layer");
        require(std::hypot(crossing->end.point.x-corner.x,crossing->end.point.z-corner.z)<=
                std::hypot(input.x-corner.x,input.z-corner.z)+.01f,
                "connected corner tangent remains inside the original input distance budget");
        const auto no_link=isolated.slide({1,0},corner,input,.33f);
        require(no_link&&no_link->blocked&&no_link->end.surface.plane==1,
                "touching an unlinked destination triangle never grants a cross-layer tangent");
        fixture.open(tip_path,std::ios::binary|std::ios::trunc);
        fixture.write("FWNAV001",8);for(auto value:{1u,7u,0xeefe704cu,12u,1u,1u})word(value);
        for(float value:{6200.f,7900.f,7400.f,8400.f})scalar(value);word(1);
        word(2);cell(1,{8064,7993,6314.01953125f,7296,6282,7225});
        cell(0,{8302.404296875f,8064,6421.53564453125f,7296,6314.01953125f,7296});
        fixture.close();const auto bounce_mesh=Mesh::load(tip_path);LayeredMesh bounce(bounce_mesh);
        for(float ratio:{0.f,.33f}){
            SurfacePoint actor{{7253.22f,8025.49f},{0,0}};
            const float length=std::hypot(704.513f,-303.515f);
            const Point input{22*704.513f/length,22*-303.515f/length};
            for(unsigned step=0;step<40;++step){
                const auto motion=bounce.slide(actor.surface,actor.point,
                    {actor.point.x+input.x,actor.point.z+input.z},ratio);
                require(motion.has_value(),"two-wall contact retains a valid navigation start");
                const float dx=motion->end.point.x-actor.point.x,dz=motion->end.point.z-actor.point.z;
                require(dx*input.x+dz*input.z>=-.02f,"second wall cannot reverse held input into a bounce loop");
                require(std::hypot(dx,dz)<=22.02f,"corner projection respects movement budget");
                actor=motion->end;
            }
            require(std::hypot(actor.point.x-7296,actor.point.z-8064)<.02f,
                "repeated input settles at the two-wall corner");
            const auto escape=bounce.slide(actor.surface,actor.point,
                {actor.point.x-22,actor.point.z},ratio);
            require(escape&&!escape->blocked&&escape->end.point.x<actor.point.x-21,
                "changing input escapes the stopped corner");
        }
        std::filesystem::remove(tip_path);
    }
    const auto isolated=unlinked.slide({0,0},{5,5},{25,5});
    require(isolated&&isolated->blocked&&isolated->end.surface.plane==0&&isolated->end.point.x<=10.011f,
            "sliding cannot enter an overlapping unlinked deck");
    {
        const auto sliding_path=std::filesystem::path(std::string(argv[1])+".sliding");
        std::ofstream fixture(sliding_path,std::ios::binary);
        const auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)fixture.put(char(n>>(8*i)));};
        const auto scalar=[&](float value){word(std::bit_cast<std::uint32_t>(value));};
        fixture.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})word(n);
        for(float n:{0.f,0.f,100000.f,100.f})scalar(n);word(3);
        for(float right_bottom:{100.f,50.f,100000.f}){
            word(1);for(unsigned i=0;i<5;++i)word(0xffffffff);
            const float right_top=right_bottom==100000?100000.f:100.f;
            for(float n:{0.f,100.f,0.f,right_top,0.f,right_bottom})scalar(n);
        }
        fixture.close();const auto sliding_mesh=Mesh::load(sliding_path);LayeredMesh walls(sliding_mesh);
        {
            fw::gw::world::Instance bounded;auto owner=std::make_shared<fw::gw::world::Outbox>();
            bounded.configure_navigation(std::make_shared<const Mesh>(sliding_mesh),1);
            for(unsigned id:{1u,2u}){bounded.join(id,"Party",id==1?20:84,20,id==1?owner:std::make_shared<fw::gw::world::Outbox>(),0,{}, {},100,1);bounded.set_vitals(id,{500,1000,100,100},100);}
            require(bounded.party_action(1,158,2,100)&&bounded.party_action(2,154,1,100),"range wall fixture party");
            const auto caster=*bounded.effect_scope(1),recipient=*bounded.effect_scope(2);
            require(bounded.stage_mending(caster,recipient,*fw::gw::skills::effects::mending(12),100),"range wall fixture upkeep");
            bounded.steer(2,1,0,100);
            require(bounded.settle_resources(caster,30100)&&bounded.maintained_enchantment_count(caster,30100)==1,
                "blocked movement cannot break upkeep using its unfulfilled far-away velocity endpoint");
        }
        require(!walls.trace({0,0},{95,20},{105,40}),"fixture reproduces all-or-nothing diagonal rejection");
        auto glancing=walls.slide({0,0},{95,20},{105,40});
        require(glancing&&glancing->clipped&&!glancing->blocked&&glancing->end.surface==Surface{0,0}&&
            std::abs(glancing->end.point.x-100)<.011f&&std::abs(glancing->end.point.z-40)<.011f,
            "diagonal wall contact consumes legal prefix then slides along tangent");
        require(std::hypot(glancing->end.point.x-95,glancing->end.point.z-20)<=std::hypot(10.f,20.f)+.02f,
            "sliding cannot add movement budget");
        const auto continuing=walls.slide(glancing->end.surface,glancing->end.point,{110,60});
        require(continuing&&!continuing->blocked&&std::abs(continuing->end.point.z-60)<.011f&&
            continuing->end.point.x<=100.011f,"held diagonal input continues along the wall");
        const auto head_on=walls.slide({0,0},{95,20},{105,20});
        require(head_on&&head_on->blocked&&std::abs(head_on->end.point.x-100)<.011f&&
            std::abs(head_on->end.point.z-20)<.011f,"head-on motion reaches wall instead of losing its valid prefix");
        const auto floored_head_on=walls.slide({0,0},{95,20},{105,20},.33f);
        require(floored_head_on&&floored_head_on->blocked&&std::abs(floored_head_on->end.point.z-20)<.011f,
            "minimum tangent ratio cannot invent lateral direction for head-on input");
        SurfacePoint direct{{100,20},{0,0}};
        for(unsigned step=0;step<40;++step){
            const auto jitter=walls.slide(direct.surface,direct.point,{110,direct.point.z+(step%2?.2f:-.2f)},.33f);
            require(jitter&&jitter->blocked&&std::abs(jitter->end.point.z-20)<.001f,
                    "alternating near-head-on input cannot amplify into side-to-side minimum-speed slides");
            direct=jitter->end;
        }
        const auto shallow=walls.slide({0,0},{95,95},{96,105},.33f);
        require(shallow&&shallow->clipped&&!shallow->blocked&&
            std::abs(shallow->end.point.x-97.156f)<.04f&&std::abs(shallow->end.point.z-100)<.011f,
            "held-input tangent uses retail minimum ratio below the orthogonal projection speed");
        require(std::abs(shallow->terminal_delta.x-std::hypot(1.f,10.f)*.33f)<.001f&&
            shallow->terminal_delta.z==0,
            "terminal tangent rate removes the partial free prefix without applying the ratio floor twice");
        const float prefix_distance=std::hypot(.501f,5.01f),tangent_distance=shallow->end.point.x-95.501f;
        require(prefix_distance+tangent_distance<=std::hypot(1.f,10.f)+.02f&&
            walls.trace(shallow->end.surface,shallow->end.point,shallow->end.point),
            "minimum tangent ratio remains within the original travel budget and on a valid surface");
        const auto unchanged=walls.slide({0,0},{95,20},{105,40},.33f);
        require(unchanged&&std::hypot(unchanged->end.point.x-glancing->end.point.x,
            unchanged->end.point.z-glancing->end.point.z)<.001f,
            "projection above retail minimum retains ordinary orthogonal response");
        require(!walls.slide({0,0},{95,20},{105,40},-1.f)&&!walls.slide({0,0},{95,20},{105,40},1.01f),
            "invalid tangent ratio is rejected rather than adding movement budget");
        const auto corner=walls.slide({0,0},{95,95},{105,105});
        require(corner&&corner->blocked&&corner->end.point.x<=100.011f&&corner->end.point.z<=100.011f,
            "closed corner stays bounded without a detour or escape");
        const auto slope=walls.slide({1,0},{84,20},{104,40});
        require(slope&&slope->clipped&&!slope->blocked&&slope->end.surface.plane==1&&
            std::abs(slope->end.point.x-84.8f)<.04f&&std::abs(slope->end.point.z-30.4f)<.04f,
            "sloped wall uses its real tangent rather than axis-only sliding or overlapping floor");
        for(float ratio:{0.f,.33f}){
            const auto twice=walls.slide({1,0},{84,20},{114,220},ratio);
            require(twice&&twice->clipped&&twice->blocked&&
                std::abs(twice->end.point.x-50.f)<.05f&&std::abs(twice->end.point.z-100)<.011f,
                "second wall stops rather than projecting backward against original input");
            require(std::isfinite(twice->terminal_delta.x)&&std::isfinite(twice->terminal_delta.z)&&
                twice->terminal_delta.x==0&&twice->terminal_delta.z==0&&
                std::hypot(twice->terminal_delta.x,twice->terminal_delta.z)<=std::hypot(30.f,200.f),
                "multiple contact prefixes retain a finite bounded terminal rate with the original interval denominator");
        }
        require(!walls.slide({0,0},{-10,20},{20,20}),"invalid start cannot be projected back onto a map");
        const Point beyond{100000.05f,50};
        require(!walls.trace({2,0},{0,50},beyond),"parametric tolerance cannot admit long-span endpoint beyond world-space wall tolerance");
        const auto long_wall=walls.slide({2,0},{0,50},beyond);
        std::vector<MotionPoint> history;
        const auto traced_wall=walls.slide({2,0},{0,50},beyond,0,0,false,&history);
        require(traced_wall&&long_wall&&traced_wall->end.point.x==long_wall->end.point.x&&
            traced_wall->blocked==long_wall->blocked&&history.size()>=3&&history.front().fraction==0&&history.back().fraction==1,
            "optional motion history preserves physical wall result and contact timing");
        for(std::size_t i=1;i<history.size();++i)require(history[i].fraction>=history[i-1].fraction,
            "collision trace fractions never run backwards");
        require(long_wall&&long_wall->clipped&&long_wall->blocked&&long_wall->end.point.x-100000<=.01f&&
            walls.trace(long_wall->end.surface,long_wall->end.point,long_wall->end.point),
            "long-span sweep ends on a valid surface instead of becoming an invalid next start");
        {
            fw::gw::world::Instance sliding;auto box=std::make_shared<fw::gw::world::Outbox>();
            sliding.configure_navigation(std::make_shared<const Mesh>(sliding_mesh),1);
            sliding.join(7,"Sloped Plane",84,20,box);box->drain();
            sliding.steer(7,10,10,1000);box->drain();
            const auto projected=sliding.maintained_range_path(*sliding.effect_scope(7),1000,1100);
            require(projected&&projected->size()>=3&&box->drain().empty(),"range slide history is read-only and retains contacts");
            sliding.tick(1100);
            const auto actual=sliding.position(7,1100);
            require(std::abs(projected->back().x-actual.first)<.001&&std::abs(projected->back().y-actual.second)<.001,
                "range forecast and authoritative wall sliding have the same endpoint");
            const auto packets=box->drain();const auto& goal=destination(packets);
            require(packets[0][0]==40&&packets[1][0]==44&&plane_word(goal,14)==1&&plane_word(goal,16)==1,
                "sliding correction and future goal retain current nonzero plane instead of selecting overlapping ground");
        }
        std::filesystem::remove(sliding_path);
    }
    {
        std::ofstream links(links_path);links<<"FWLINK01 7 3 4\n0 0 1 0\n1 0 0 0\n1 0 0 1\n0 1 1 0\n";
    }
    auto runtime_mesh=std::make_shared<const Mesh>(Mesh::load(argv[1]));
    {
        fw::gw::world::Instance crossing;auto owner=std::make_shared<fw::gw::world::Outbox>(),
            observer=std::make_shared<fw::gw::world::Outbox>();
        crossing.configure_navigation(runtime_mesh);crossing.join(7,"Crossing",15,5,owner);
        crossing.join(8,"Observer",5,5,observer);owner->drain();observer->drain();
        crossing.steer(7,10,0,1000);const auto initial=owner->drain();observer->drain();
        const auto& initial_goal=destination(initial);
        require(plane_word(initial_goal,14)==0&&plane_word(initial_goal,16)==1,
            "keyboard initial destination distinguishes future ground plane from current bridge plane");
        // Original hash-pinned 37130 serializer, descriptor59b104, native
        // [41,7,25.f,5.f,targetPlane0,currentPlane1] produced this wire fixture.
        auto wire=initial_goal;wire.at(1)|=0x80;
        const fw::gw::world::Packet original_wire{0x29,0x80,7,0,0,0,0,0,0xc8,0x41,0,0,0xa0,0x40,0,0,1,0};
        require(wire==original_wire,"destination plane field order matches the pinned original serializer");
        crossing.tick(1040);const auto observed=observer->drain();
        require(observed.empty(),"observer retains the initial forward bridge goal rather than a completed-tick replacement");
        crossing.steer(7,0,0,1040);
    }
    {
        fw::gw::world::Instance click;auto box=std::make_shared<fw::gw::world::Outbox>();
        click.configure_navigation(runtime_mesh);click.join(7,"Click Plane",15,5,box);box->drain();
        click.move(7,18,5,1000,1);const auto first=box->drain();const auto& goal=destination(first);
        require(plane_word(goal,14)==1&&plane_word(goal,16)==1,"click start preserves its nonzero current plane");
        fw::gw::world::Instance transition;auto transitions=std::make_shared<fw::gw::world::Outbox>();
        transition.configure_navigation(runtime_mesh);transition.join(7,"Transition",5,5,transitions);transitions->drain();
        transition.move(7,15,5,1000,1);transitions->drain();transition.tick(1023);
        const auto next=transitions->drain();const auto& next_goal=destination(next);
        require(next.size()>=2&&next.front().at(0)==44&&plane_word(next.front(),14)==1,
            "bridge continuation must anchor the native actor on the reached plane before publishing its next destination");
        require(plane_word(next_goal,14)==1&&plane_word(next_goal,16)==1,
            "click next segment sets its actual bridge current plane after the explicit portal switch");
    }
    {
        fw::gw::world::Instance timed;auto box=std::make_shared<fw::gw::world::Outbox>();
        timed.configure_navigation(runtime_mesh);timed.join(1,"Timed",5,5,box);
        timed.move(1,25,5,1000,0);
        const auto path=timed.maintained_range_path(*timed.effect_scope(1),1000,1100);
        require(path&&path->size()>=4,"range adapter retains queued bridge route boundaries");
        require(path->front().x==5&&path->back().x==25,"range adapter preserves accepted bridge endpoints");
        for(std::size_t i=1;i<path->size();++i)
            require((*path)[i].at>(*path)[i-1].at&&(*path)[i].x>=(*path)[i-1].x,
                "range trajectory follows forward accepted segment clocks");
        timed.tick(1100);
        require(std::abs(timed.position(1,1100).first-25)<.01f,"one delayed tick consumes complete 20-unit bridge route without per-segment pauses");
    }
    fw::gw::world::Instance world;auto outbox=std::make_shared<fw::gw::world::Outbox>();
    world.configure_navigation(runtime_mesh);world.join(1,"Bridge",5,5,outbox);
    world.steer(1,1,0,1000);world.tick(1045);world.steer(1,0,0,1045);world.tick(1200);
    require(std::abs(world.position(1,1200).first-17.96f)<.01f,"keyboard enters bridge and release stays there");
    bool plane_sent=false;for(const auto& packet:outbox->drain())
        if(packet[0]==44&&packet.size()==16&&packet[14]==1)plane_sent=true;
    require(plane_sent,"movement publishes bridge plane to client");
    auto peer=std::make_shared<fw::gw::world::Outbox>();
    const auto prior=world.join(2,"Peer",5,5,peer);
    require(prior.size()==1&&prior[0].plane==1,"joining peer receives occupied bridge plane");
    world.move(1,25,5,1300,0);
    outbox->drain();peer->drain();
    bool unreachable_rejected=false;
    try{world.move(1,25,5,1310,2);}catch(const std::runtime_error&){unreachable_rejected=true;}
    require(unreachable_rejected,"overlapping but disconnected destination rejected");
    require(outbox->drain().empty()&&peer->drain().empty(),"rejected click publishes no movement");
    for(std::uint64_t now=1400;now<=2000;now+=100)world.tick(now);
    require(std::abs(world.position(1,2000).first-25)<.01f,"click exits bridge at ground destination");
    world.move(1,15,5,2100,1);
    for(std::uint64_t now=2200;now<=2800;now+=100)world.tick(now);
    require(std::abs(world.position(1,2800).first-15)<.01f,"click enters bridge in reverse direction");
    require(world.follow(1,2,2900),"follow accepts player on another connected plane");
    for(std::uint64_t now=3000;now<=3800;now+=100){world.tick(now);outbox->drain();peer->drain();}
    require(world.position(1,3800).first<=10.01f,"follow must leave bridge plane before considering nearby target reached");
    {
        std::ofstream links(links_path);links<<"FWLINK01 7 3 1\n0 0 1 0\n";
    }
    bool invalid=false;
    try{Mesh::load(argv[1]);}catch(const std::runtime_error&){invalid=true;}
    require(invalid,"loader rejects missing reciprocal link");
    {
        std::ofstream links(links_path);links<<"FWLINK01 7 3 2\n0 0 2 0\n2 0 0 0\n";
    }
    invalid=false;
    try{Mesh::load(argv[1]);}catch(const std::runtime_error&){invalid=true;}
    require(invalid,"loader rejects overlap with no shared boundary");
    std::filesystem::remove(links_path);
    std::cout<<"PASS cross-plane bridge routing, reverse traversal, walls and overlap isolation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
