#include "gw_map_portal_candidates.h"
#include "gw_world_simulation.h"
#include "gw_map_assets_generated.h"
#include <iostream>
namespace navigation=fw::gw::navigation;

int main(int argc,char** argv)try{
    using namespace fw::gw::map_portals;
    const auto require=[](bool okay,const char* why){if(!okay)throw std::runtime_error(why);};
    const auto rejects=[&](auto work,const char* why){bool failed=false;try{work();}catch(const std::runtime_error&){failed=true;}require(failed,why);};
    require(argc==4,"usage: map_portals_test generated-dir real-catalog navigation-dir");
    const std::filesystem::path generated=argv[1];std::filesystem::create_directories(generated);
    const auto fixture=generated/"portal-fixture.fwnav";
    {std::ofstream out(fixture,std::ios::binary);
        const auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
        const auto scalar=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
        out.write("FWNAV001",8);for(auto n:{1u,99u,0xeefe704cu,12u,1u,1u})word(n);
        for(float n:{0.f,0.f,100.f,100.f})scalar(n);word(2);
        for(unsigned floor=0;floor<2;++floor){word(1);for(unsigned n=0;n<5;++n)word(0xffffffff);
            for(float n:{0.f,100.f,0.f,100.f,0.f,100.f})scalar(n);}
    }
    const auto mesh=navigation::Mesh::load(fixture);
    {fw::gw::world::Instance world;world.configure_navigation(std::make_shared<navigation::Mesh>(mesh));
        const auto first=std::make_shared<fw::gw::world::Outbox>(),second=std::make_shared<fw::gw::world::Outbox>();
        world.join(1,"Upper",45,50,first,0,{},1);
        world.join(2,"Ground",45,50,second,0,{},0);
        require(world.plane(1)==1&&world.plane(2)==0,"per-player arrival plane was replaced or changed the global floor");
        rejects([&]{world.join(3,"Invalid",45,50,std::make_shared<fw::gw::world::Outbox>(),0,{},2);},"invalid granted plane accepted");
    }
    const auto write=[&](const std::string& content){std::ofstream out(generated/"catalog.txt");out<<content;out.close();return Catalog::read(generated/"catalog.txt");};
    const std::string row="gate 1 2 99 99 0 40 40 50 60 0 5 5 0 1 0 1000 test_development\n";
    auto catalog=write("FWGATE01\n"+row);
    catalog.validate_all([&](std::uint32_t){return &mesh;});
    const auto union_mesh=[&](std::string name,bool hole,bool connected){
        const auto file=generated/(name+".fwnav");std::ofstream out(file,std::ios::binary);
        const auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
        const auto scalar=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
        out.write("FWNAV001",8);for(auto n:{1u,99u,0xeefe704cu,12u,1u,1u})word(n);
        for(float n:{0.f,0.f,100.f,100.f})scalar(n);word(2);word(4);
        for(unsigned c=0;c<4;++c){
            const unsigned a=c==0?1:c==3?1:0,b=c==0?2:c==3?2:3;
            word(connected?a:0xffffffffu);word(connected?b:0xffffffffu);
            for(unsigned n=0;n<3;++n)word(0xffffffffu);
            const float low=c==2?75.f:0.f,high=c==1?(hole?65.f:75.f):100.f,
                left=c==0?0.f:c==3?60.f:40.f,right=c==0?40.f:c==3?100.f:60.f;
            for(float n:{low,high,left,right,left,right})scalar(n);
        }
        // An overlapping other floor must not hide the plane0 interior hole.
        word(1);for(unsigned n=0;n<5;++n)word(0xffffffffu);
        for(float n:{0.f,100.f,0.f,100.f,0.f,100.f})scalar(n);
        out.close();return navigation::Mesh::load(file);
    };
    const auto connected_union=union_mesh("connected-union",false,true);
    auto broad=catalog.routes().front();broad.trigger={20,10,80,90};
    validate(broad,connected_union,connected_union);
    const auto interior_hole=union_mesh("interior-hole",true,true);
    for(Point p:std::array<Point,4>{{{20,10},{80,10},{80,90},{20,90}}})
        require(interior_hole.walkable(0,p),"hole regression requires all corners to be walkable");
    rejects([&]{validate(broad,interior_hole,interior_hole);},"corner-only union proof accepted an interior gap or wrong-floor fill");
    const auto disconnected_union=union_mesh("disconnected-union",false,false);
    rejects([&]{validate(broad,disconnected_union,disconnected_union);},"unlinked adjacent cells accepted as connected doorway");
    const auto candidate=reverse_candidate(catalog.routes().front(),mesh);
    require(candidate&&candidate->source==2&&candidate->destination==1&&
        !contains(catalog.routes().front().trigger,candidate->arrival.point)&&
        candidate->provenance=="generated_shared_asset_review_required","safe review-required reverse inference failed");
    auto unequal=catalog.routes().front();unequal.destination_asset=100;
    require(!reverse_candidate(unequal,mesh),"different asset coordinate system guessed as reverse gate");
    auto invalid=catalog.routes().front();invalid.arrival.point={101,5};
    rejects([&]{validate(invalid,mesh,mesh);},"off-mesh arrival accepted");
    invalid=catalog.routes().front();invalid.source_asset=100;
    rejects([&]{validate(invalid,mesh,mesh);},"asset pin ignored");
    invalid=catalog.routes().front();invalid.trigger={-1,40,50,60};
    rejects([&]{validate(invalid,mesh,mesh);},"trigger crossing outside geometry accepted");
    invalid=catalog.routes().front();invalid.outward={0,0};
    rejects([&]{validate(invalid,mesh,mesh);},"no outward direction accepted");
    rejects([&]{write("FWGATE01\n"+row+row);},"duplicate portal accepted");
    rejects([&]{write("FWGATE01\n"+row+"other 1 3 99 99 0 45 45 55 65 0 5 5 0 1 0 1000 test\n")
        .validate_all([&](std::uint32_t){return &mesh;});},"ambiguous overlap accepted");
    rejects([&]{write("FWGATE01\n"+row+"garbage\n");},"truncated catalog row accepted");
    rejects([&]{write("FWGATE02\n");},"unknown catalog version accepted");
    const auto sample=[](float x,float z,std::uint64_t time=100,std::uint32_t plane=0,std::uint32_t map=1,std::uint64_t epoch=7){
        return Sample{map,plane,epoch,time,{x,z}};};
    Detector d;d.reseed(catalog,sample(30,50));
    auto crossing=d.observe(catalog,sample(45,50,200));
    require(crossing&&crossing->destination==2&&crossing->arrival.point.x==5&&crossing->epoch==7,"valid authoritative entry failed");
    require(d.pending()&&!d.observe(catalog,sample(70,50,300)),"pending transfer repeated");
    d.resolve(300);require(!d.pending(),"failed transfer cannot be resolved");
    require(!d.observe(catalog,sample(30,50,400))&&!d.observe(catalog,sample(45,50,500)),"cooldown ignored");
    require(!d.observe(catalog,sample(30,50,1400))&&d.observe(catalog,sample(45,50,1500)),"failed transfer did not rearm after leave/cooldown");
    Detector swept;swept.reseed(catalog,sample(30,50));
    require(swept.observe(catalog,sample(70,50,200)).has_value(),"thin portal tunneled through between samples");
    Detector spawned;spawned.reseed(catalog,sample(45,50));
    require(!spawned.observe(catalog,sample(46,50,200)),"spawn inside portal bounced");
    require(!spawned.observe(catalog,sample(30,50,300))&&spawned.observe(catalog,sample(45,50,400)),"spawn suppression failed to rearm");
    Detector reverse;reverse.reseed(catalog,sample(70,50));
    require(!reverse.observe(catalog,sample(30,50,200)),"backwards entry traveled");
    Detector wrong;wrong.reseed(catalog,sample(30,50,100,1));
    require(!wrong.observe(catalog,sample(45,50,200,1)),"overlapping other floor triggered");
    Detector connector;connector.reseed(catalog,sample(30,50,100,1));
    require(connector.observe(catalog,sample(45,50,200,0)).has_value(),"collision-validated connector entry missed the gate");
    Detector face_connector;face_connector.reseed(catalog,sample(40,50,100,1));
    require(face_connector.observe(catalog,sample(45,50,200,0)).has_value(),"validated connector exactly at inward face missed the gate");
    Detector vertical;vertical.reseed(catalog,sample(45,50,100,1));
    require(!vertical.observe(catalog,sample(46,50,200,0)),"XY-overlapping plane switch fabricated a gate entry");
    Detector touch;touch.reseed(catalog,sample(40,30));
    require(!touch.observe(catalog,sample(40,70,200)),"walking along inward trigger face transferred");
    Detector edge_lane;edge_lane.reseed(catalog,sample(30,40));
    require(edge_lane.observe(catalog,sample(70,40,200)).has_value(),"valid closed transverse navigation-edge lane missed");
    Detector corner;corner.reseed(catalog,sample(30,50));
    require(!corner.observe(catalog,sample(50,30,200)),"single corner contact transferred");
    Detector face;face.reseed(catalog,sample(30,50));
    require(!face.observe(catalog,sample(40,50,200))&&face.observe(catalog,sample(45,50,300)),"exact inward-face sample swallowed next crossing");
    Detector seeded_face;seeded_face.reseed(catalog,sample(40,50));
    require(seeded_face.observe(catalog,sample(45,50,200)).has_value(),"spawn exactly on inward face swallowed outward gate entry");
    Detector reposition_face;reposition_face.reseed(catalog,sample(30,50));
    require(!reposition_face.observe(catalog,sample(40,50,200),false)&&
        reposition_face.observe(catalog,sample(45,50,300)),"explicit reposition onto inward face permanently suppressed entry");
    Detector side;side.reseed(catalog,sample(45,30));
    require(!side.observe(catalog,sample(46,50,200)),"entering transverse side without crossing inward face transferred");
    Detector teleport;teleport.reseed(catalog,sample(30,50));
    require(!teleport.observe(catalog,sample(70,50,200),false),"reposition triggered crossing");
    Detector gap;gap.reseed(catalog,sample(30,50));
    require(!gap.observe(catalog,sample(70,50,4000)),"missing movement samples fabricated a crossing");
    Detector epoch;epoch.reseed(catalog,sample(30,50));
    require(!epoch.observe(catalog,sample(45,50,200,0,1,8)),"new instance reused stale segment");
    require(!epoch.observe(catalog,sample(46,50,300,0,1,8)),"new instance spawned inside gate triggered");
    Detector clock;clock.reseed(catalog,sample(30,50,500));
    require(!clock.observe(catalog,sample(45,50,400)),"clock reversal reused stale segment");
    auto real=Catalog::read(argv[2]);std::map<std::uint32_t,navigation::Mesh> assets;
    require(fw::gw::map_asset_file(160)==113190,"Green Hills captured shared-asset binding missing");
    require(fw::gw::map_asset_file(161)==113355,"Wizard's Folly captured shared-asset binding missing");
    std::map<std::uint32_t,std::uint32_t> maps;
    for(const auto& route:real.routes()){
        for(const auto [map,asset]:{std::pair{route.source,route.source_asset},std::pair{route.destination,route.destination_asset}}){
            require(!maps.contains(map)||maps.at(map)==asset,"logical map asset conflict");maps[map]=asset;
            require(fw::gw::map_asset_file(static_cast<std::uint16_t>(map))==asset,"portal and pinned resolver asset disagree");
            if(!assets.contains(asset))assets.emplace(asset,navigation::Mesh::load(std::filesystem::path(argv[3])/(std::to_string(asset)+".fwnav")));}
    }
    // The running host consumes the navigation catalog, not only the generated
    // resolver. Catch a new logical binding whose asset exists but whose
    // catalog row is absent, which would otherwise fail host startup.
    std::ifstream navigation_catalog(std::filesystem::path(argv[3])/"catalog.txt");
    require(bool(navigation_catalog),"runtime navigation catalog unavailable");
    std::map<std::uint32_t,std::filesystem::path> bindings;std::string binding;
    while(std::getline(navigation_catalog,binding)){
        if(!binding.empty()&&binding.back()=='\r')binding.pop_back();
        if(binding.empty()||binding[0]=='#')continue;
        const auto at=binding.find('=');require(at!=std::string::npos,"invalid runtime navigation catalog row");
        std::uint32_t map=0;const auto parsed=std::from_chars(binding.data(),binding.data()+at,map);
        require(parsed.ec==std::errc{}&&parsed.ptr==binding.data()+at,"invalid runtime navigation catalog map");
        if(maps.contains(map))require(bindings.emplace(map,binding.substr(at+1)).second,"duplicate active navigation catalog binding");
    }
    require(!navigation_catalog.bad(),"runtime navigation catalog read failed");
    for(const auto& [map,asset]:maps){
        require(bindings.contains(map),"active portal map absent from runtime navigation catalog");
        const auto path=navigation::catalog_asset_path(bindings.at(map).string(),std::filesystem::absolute(argv[3]));
        require(navigation::Mesh::load(path).asset_hash()==asset,"runtime navigation catalog points to wrong asset");
    }
    real.validate_all([&](std::uint32_t map){return &assets.at(maps.at(map));});
    // Multiple private failures ran below the previous single-cell rectangle
    // at y1437.78/1424.18. Cover every lane of the actual connected corridor,
    // including its outer wall edges, without a teleport or wrong-floor fill.
    for(float y:{1312.f,1330.f,1360.f,1424.18f,1437.78f,1440.f,1458.f,1510.f,1540.f,1553.f}){
        const Point a{-7455,y},b{-7490,y};
        require(assets.at(113190).walkable(0,a)&&assets.at(113190).walkable(0,b),"Barradin lane is not genuinely navigable");
        Detector lane;lane.reseed(real,{163,0,1,100,a});
        const auto exit=lane.observe(real,{163,0,1,200,b});
        require(exit&&exit->id=="barradin_green_hills"&&exit->destination==160,
            "validated Barradin exit corridor lane missed portal");
    }
    // Real walkable approach lanes, not synthetic segments through box centers.
    // Abbey's shared-asset passage runs east/west; a captured facing was not
    // evidence for requiring a northward crossing. Catacombs crosses0->47.
    for(const auto map: {146u,164u})for(float y:{-6340.f,-6310.f,-6270.f,-6221.f,-6180.f,-6110.f}){
        const bool into=map==146;const Point a{into?-11070.f:-11240.f,y},b{into?-11230.f:-11070.f,y};
        const auto& mesh=assets.at(113021);const auto cell=mesh.locate_cell(0,a);
        require(cell.has_value(),"Abbey approach starts off mesh");
        navigation::LayeredMesh graph(mesh);require(graph.trace({0,*cell},a,b).has_value(),"Abbey lane crosses a wall");
        Detector lane;lane.reseed(real,{map,0,1,100,a});const auto gate=lane.observe(real,{map,0,1,200,b});
        require(gate&&gate->destination==(into?164u:146u),"Abbey connected corridor lane missed portal");
    }
    for(float y:{2020.f,2050.f,2100.f,2135.f,2170.f,2200.f,2240.f}){
        const auto& mesh=assets.at(116016);navigation::LayeredMesh graph(mesh);
        Point previous{14380,y};const auto cell=mesh.locate_cell(0,previous);
        require(cell.has_value(),"Catacombs exit approach starts off floor0");
        navigation::Surface surface{0,*cell};Detector lane;lane.reseed(real,{145,0,1,100,previous});bool crossed=false;
        for(unsigned step=1;step<=7;++step){
            Point next{14380.f+20.f*step,y};const auto reached=graph.trace(surface,previous,next);
            require(reached.has_value(),"Catacombs connector lane crosses a wall");surface=*reached;
            if(const auto gate=lane.observe(real,{145,static_cast<std::uint32_t>(surface.plane),1,100+100*step,next})){
                require(gate->destination==164,"Catacombs exit reached wrong map");crossed=true;
            }
            previous=next;
        }
        require(crossed,"Catacombs actual0-to47 approach missed exit");
    }
    // Foibles' old center-only rectangle missed both outer lanes of the
    // actual plane0 corridor. The complete rectangle is physical cell586;
    // exact traces also prove its inward adjacency before detector checks.
    for(const auto map:{165u,161u})for(float x:{192.f,200.f,220.f,250.f,350.f,470.f,550.f,560.f,576.f}){
        const bool into=map==165;
        const Point a{x,into?7776.f:7660.f},b{x,into?7660.f:7776.f};
        const auto& mesh=assets.at(113355);const auto cell=mesh.locate_cell(0,a);
        require(cell.has_value(),"Foibles corridor approach starts off mesh");
        navigation::LayeredMesh graph(mesh);const auto reached=graph.trace({0,*cell},a,b);
        require(reached&&reached->plane==0,"Foibles corridor lane crosses a wall or wrong plane");
        Detector lane;lane.reseed(real,{map,0,1,100,a});
        const auto gate=lane.observe(real,{map,0,1,200,b});
        require(gate&&gate->id==(into?"foibles_wizards_folly":"wizards_folly_foibles")&&
            gate->destination==(into?161u:165u),"Foibles full connected corridor lane missed portal");
        require(!lane.observe(real,{map,0,1,300,b}),"Foibles pending crossing duplicated");
        Detector arrival;arrival.reseed(real,{gate->destination,gate->arrival.plane,2,400,gate->arrival.point});
        require(!arrival.observe(real,{gate->destination,gate->arrival.plane,2,500,gate->arrival.point}),
            "Foibles retained arrival bounced through reverse gate");
        Detector reverse;reverse.reseed(real,{map,0,1,100,b});
        require(!reverse.observe(real,{map,0,1,200,a}),"Foibles backwards crossing transferred");
        Detector other_floor;other_floor.reseed(real,{map,1,1,100,a});
        require(!other_floor.observe(real,{map,1,1,200,b}),"Foibles overlapping wrong floor transferred");
    }
    for(const auto map:{165u,161u}){
        Detector transverse;transverse.reseed(real,{map,0,1,100,{192,7720}});
        require(!transverse.observe(real,{map,0,1,200,{576,7720}}),"Foibles transverse corridor travel transferred");
    }
    // Older captures precede local_map_transition_observed. Their map epochs
    // and earliest entry records prove these arrivals; raw52044 independently
    // proves the Green Hills reverse and the actual Regent/Wizard bridges.
    // Bounds are authored approach strips, not recovered retail trigger width.
    struct CapturedGateLane {
        const char* id;unsigned source,destination,asset,plane;
        Point anchor;unsigned anchor_plane;bool horizontal;float start,end;
        std::array<float,3> lanes;Point arrival;float facing;
    };
    const std::array<CapturedGateLane,7> captured_lanes{{
        {"capture_reverse_160_146_1517",160,146,113190,0,{21932,12966},0,true,22080,22170,{12880,12966,13120},{-13891,10107},0},
        {"capture_reverse_161_146_1892",161,146,113355,0,{9449,19702},0,false,19780,19860,{9620,9800,10280},{-12914,-20062},0},
        {"capture_reverse_162_146_1636",162,146,113437,0,{-14893,16791},0,false,16980,17060,{-15180,-14900,-14620},{3795,-19624},2.448958f},
        {"capture_bridge_162_161_177962",162,161,113437,5,{-24248,-12108},0,true,-25980,-26070,{-13160,-13100,-13040},{19849,-10398},2.44895816f},
        {"capture_bridge_161_162_179176",161,162,113355,9,{19849,-10398},0,true,19980,20070,{-10070,-10040,-10010},{-24248,-12108},1.43474317f},
        {"capture_northlands_146_147_22704",146,147,113021,36,{-5434,11799},0,false,13788,13830,{-5740,-5500,-5330},{-11574,-16170},1.434743f},
        {"capture_northlands_147_146_24425",147,146,116025,22,{-11574,-16170},0,false,-16890,-16930,{-11880,-11700,-11470},{-5434,11799},-.717372f}
    }};
    for(const auto& fixture:captured_lanes){
        auto mesh=assets.at(fixture.asset);mesh.discard_unverified_portals();navigation::LayeredMesh graph(mesh);
        const auto anchor=mesh.locate_cell(fixture.anchor_plane,fixture.anchor);
        require(anchor.has_value(),"captured reverse-gate approach anchor off pinned mesh");
        for(float transverse:fixture.lanes){
            const Point a=fixture.horizontal?Point{fixture.start,transverse}:Point{transverse,fixture.start},
                b=fixture.horizontal?Point{fixture.end,transverse}:Point{transverse,fixture.end};
            const auto cell=mesh.locate_cell(fixture.plane,a);require(cell.has_value(),"captured reverse-gate lane starts off mesh");
            const auto approach=graph.route({fixture.anchor_plane,*anchor},fixture.anchor,{fixture.plane,*cell},a);
            require(approach.has_value(),"captured arrival cannot reach authored gate through real connectors");
            for(std::size_t i=1;i<approach->size();++i){const auto& from=(*approach)[i-1];const auto& to=(*approach)[i];
                require(graph.straight_route(from.surface,from.point,to.surface,to.point).has_value(),"reverse-gate approach route contains an illegal segment");}
            const auto reached=graph.trace({fixture.plane,*cell},a,b);
            require(reached&&reached->plane==fixture.plane,"captured reverse-gate lane crosses wall or wrong floor");
            Detector lane;lane.reseed(real,{fixture.source,fixture.plane,1,100,a});
            const auto gate=lane.observe(real,{fixture.source,fixture.plane,1,200,b});
            require(gate&&gate->id==fixture.id&&gate->destination==fixture.destination&&gate->arrival.plane==0&&
                gate->arrival.point.x==fixture.arrival.x&&gate->arrival.point.z==fixture.arrival.z&&
                std::abs(gate->arrival.facing-fixture.facing)<.000001f,"captured reverse gate missing or changed exact arrival");
            require(!lane.observe(real,{fixture.source,fixture.plane,1,300,b}),"captured reverse crossing duplicated");
            Detector arrival;arrival.reseed(real,{fixture.destination,0,2,400,fixture.arrival});
            require(!arrival.observe(real,{fixture.destination,0,2,500,fixture.arrival}),"captured reverse arrival bounced");
            Detector backward;backward.reseed(real,{fixture.source,fixture.plane,1,100,b});
            require(!backward.observe(real,{fixture.source,fixture.plane,1,200,a}),"captured reverse wrong-direction crossing transferred");
            const unsigned wrong_plane=fixture.plane?0:1;
            Detector other_floor;other_floor.reseed(real,{fixture.source,wrong_plane,1,100,a});
            require(!other_floor.observe(real,{fixture.source,wrong_plane,1,200,b}),"captured reverse overlapping wrong floor transferred");
        }
    }
    std::set<std::string> expected{"capture_lakeside_regent_1574","ascalon_lakeside","lakeside_ascalon","ashford_catacombs","catacombs_ashford","fort_ranik_regent","regent_fort_ranik","barradin_green_hills","green_hills_barradin","foibles_wizards_folly","wizards_folly_foibles"};
    for(const auto& route:real.routes()){
        expected.erase(route.id);
        if(route.id=="regent_fort_ranik"){
            require(route.source==162&&route.destination==166&&route.source_plane==7&&route.arrival.plane==8,"Fort Ranik reverse lost actual connector plane");
            require(!contains(route.trigger,route.arrival.point)&&assets.at(113437).walkable(8,route.arrival.point),"reverse arrival must be outside gate on walkable destination surface");
        }
        const Point center{(route.trigger.min_x+route.trigger.max_x)*.5f,(route.trigger.min_z+route.trigger.max_z)*.5f};
        const float span=std::hypot(route.trigger.max_x-route.trigger.min_x,route.trigger.max_z-route.trigger.min_z);
        Detector walker;walker.reseed(real,Sample{route.source,route.source_plane,1,100,{center.x-route.outward.x*span,center.z-route.outward.z*span}});
        const auto result=walker.observe(real,Sample{route.source,route.source_plane,1,200,center});
        require(result&&result->id==route.id&&result->arrival.map==route.destination,"real catalog direction/arrival mismatch");
    }
    require(expected.empty(),"required observed portal directions missing");
    std::cout<<"portal sweep, suppression, discontinuity, catalog and pinned navigation directions passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
