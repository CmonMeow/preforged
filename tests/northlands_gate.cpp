#include "gw_northlands_navigation.h"
#include <iostream>
namespace ng=fw::gw::northlands;
namespace nav=fw::gw::navigation;
namespace {
unsigned checks=0;
void require(bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F f,const char* why){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}require(rejected,why);}
const ng::Lease lease{7,11};const ng::Scope lever{7,62000,17};
ng::Pull pull(std::uint64_t operation=1){return {{7,1,3},{7,1,3},lever,ng::lever_position,operation,true,true,true};}
ng::Controller gate(){return {ng::Scene::south146,lease,lever};}
void legal(const nav::LayeredMesh& graph,const std::vector<nav::SurfacePoint>& route){
    for(std::size_t n=1;n<route.size();++n)
        require(graph.straight_route(route[n-1].surface,route[n-1].point,route[n].surface,route[n].point).has_value(),"dynamic gate route contains illegal edge");
}
std::string hex(const std::vector<std::uint8_t>& bytes){const char* digits="0123456789abcdef";std::string out;for(auto b:bytes){out+=digits[b>>4];out+=digits[b&15];}return out;}
}
int main(int argc,char**argv)try{
    require(argc==4,"usage: northlands_gate_test generated-directory south-asset north-asset");
    const std::filesystem::path generated=argv[1];std::filesystem::create_directories(generated);
    rejects([]{ng::Controller g(ng::Scene::south146,{},lever);},"zero scene lease accepted");
    rejects([]{ng::Controller g(ng::Scene::south146,lease,{8,62000,17});},"foreign lever lifetime accepted");
    rejects([]{ng::Controller g(ng::Scene::north147,lease,lever);},"north return got an invented interactive lever");
    rejects([]{ng::Controller g(ng::Scene::south146,lease,lever,{160,30000});},"animation longer than open interval accepted");
    rejects([]{ng::Controller g(ng::Scene::south146,lease,lever,{0,1000});},"zero authored interaction radius accepted");
    auto g=gate();require(!g.collision_open()&&!g.next_boundary(),"South scene not initially closed");
    auto initial=g.snapshot();require(initial[0].value==2&&initial[0].flags==2&&initial[1].value==3&&initial[1].flags==3,"closed snapshot mismatches captured state");
    auto opened=g.pull(pull(),1000);require(opened.status==ng::Status::opened&&opened.events.size()==1&&g.collision_open(),"valid solo pull did not open gate");
    require(opened.events[0].changes_collision&&opened.events[0].collision_open&&opened.events[0].at==1000&&opened.events[0].value==16,"open collision boundary missing");
    require(g.closes_at()==31000&&g.next_boundary()==2000,"captured30s interval or authored settle boundary wrong");
    require(g.pull(pull(),1001).status==ng::Status::duplicate&&g.closes_at()==31000,"replayed operation extended/toggled gate");
    auto settled=g.advance(2000);require(settled.events.size()==1&&settled.events[0].record==ng::Record::state&&settled.events[0].value==0&&!settled.events[0].changes_collision,"open settle wrong");
    require(g.advance(30999).events.empty()&&g.collision_open(),"gate closed before exact deadline");
    auto closed=g.advance(31000);require(closed.events.size()==1&&!g.collision_open()&&closed.events[0].at==31000&&closed.events[0].value==3,"gate did not close at exact deadline");
    require(g.advance(32000).events.size()==1&&g.advance(50000).events.empty(),"closed settle repeated or stale timer survived");
    require(g.pull(pull(2),50000).status==ng::Status::opened,"new genuine pull failed");
    require(g.pull(pull(3),50172).status==ng::Status::closed,"captured rapid second genuine pull did not toggle closed");
    require(g.pull(pull(4),50200).status==ng::Status::opened,"third genuine pull failed to reopen");
    auto only_current=g.advance(51200);require(only_current.events.size()==1&&only_current.events[0].value==0&&g.closes_at()==80200,"cancelled animation/close timer survived toggle");
    auto late=g.advance(90000);require(late.events.size()==2&&late.events[0].at==80200&&late.events[1].at==81200&&!g.collision_open(),"late tick did not preserve actual closure chronology");
    auto revision=g.revision();require(g.advance(89999).status==ng::Status::clock_regressed&&g.revision()==revision,"clock rollback mutated gate");
    for(unsigned variant=0;variant<10;++variant){
        auto bad=pull();auto item=gate();
        switch(variant){case 0:bad.selected_actor.generation++;break;case 1:bad.current_actor.instance++;bad.selected_actor=bad.current_actor;break;
        case 2:bad.loaded=false;break;case 3:bad.alive=false;break;case 4:bad.operation=0;break;case 5:bad.selected_lever.generation++;break;
        case 6:bad.server_reachable=false;break;case 7:bad.position.x+=161;break;case 8:bad.position.z=std::numeric_limits<float>::quiet_NaN();break;
        case 9:bad.current_actor.actor=lever.actor;bad.selected_actor=bad.current_actor;break;}
        const auto out=item.pull(bad,10);require(out.status!=ng::Status::opened&&!item.collision_open()&&out.events.empty(),"forged/out-of-range/unready pull opened gate");
    }
    auto boundary=gate();auto exact=pull();exact.position.x+=160;
    require(boundary.pull(exact,0).status==ng::Status::opened,"authored exact interaction boundary denied");
    auto other=pull();other.current_actor={7,2,4};other.selected_actor=other.current_actor;
    require(boundary.pull(other,1).status==ng::Status::closed,"second solo actor operation incorrectly shared dedup sequence");
    auto expiry=gate();expiry.pull(pull(),0);auto forged=pull(2);forged.selected_lever.generation++;
    auto denial=expiry.pull(forged,30000);require(denial.status==ng::Status::wrong_target&&denial.events.size()==2&&!expiry.collision_open(),"rejected pull swallowed authoritative timed boundary");
    auto limit=gate();const auto max=std::numeric_limits<std::uint64_t>::max();
    require(limit.pull(pull(),max-31000).status==ng::Status::opened,"last nonoverflowing open denied");
    require(limit.advance(max).events.size()==3&&!limit.collision_open(),"max clock closure overflowed");
    auto overflow=gate();require(overflow.pull(pull(),max-30999).status==ng::Status::time_overflow&&!overflow.collision_open(),"overflowing open committed");
    ng::Controller north(ng::Scene::north147,lease);
    require(north.collision_open()&&!north.next_boundary()&&north.pull(pull(),1).status==ng::Status::not_interactive,"North return invented South timer or party/lever prerequisite");
    require(north.advance(1000000).events.empty()&&north.snapshot()[0].value==0,"North return lost observed open baseline");
    auto south_mesh=nav::Mesh::load(argv[2]);south_mesh.discard_unverified_portals();
    auto north_mesh=nav::Mesh::load(argv[3]);north_mesh.discard_unverified_portals();
    auto shut=ng::navigation_view(south_mesh,ng::Scene::south146,false),open=ng::navigation_view(south_mesh,ng::Scene::south146,true);
    const nav::Point south{-5600,12122.3f},bridge{-5320,12122.7f},entry{-5479,13797};
    const nav::Surface from{36,20},to{37,10},exit{36,*south_mesh.locate_cell(36,entry)};
    for(bool reverse:{false,true}){
        const auto a=reverse?bridge:south,b=reverse?south:bridge;const auto source=reverse?to:from,target=reverse?from:to;
        require(!shut->trace(source,a,b)&&!shut->straight_route(source,a,target,b)&&!shut->route(source,a,target,b),"closed physical barrier bypassed by trace/click/visibility route");
        require(shut->reachable_within(source,a,target,b,5000)==false,"closed barrier bypassed by reachability");
        const auto slide=shut->slide(source,a,b,.33f);require(slide&&slide->clipped&&slide->end.surface!=target,"keyboard slide crossed closed barrier");
        require(open->trace(source,a,b)==target,"open physical barrier blocked direct motion");
        const auto route=open->route(source,a,target,b);require(route.has_value(),"open gate lost route");legal(*open,*route);
    }
    const auto lever_cell=south_mesh.locate_cell(ng::lever_plane,ng::lever_position);require(lever_cell.has_value(),"captured lever off pinned plane");
    require(!shut->route({ng::lever_plane,*lever_cell},ng::lever_position,exit,entry),"closed gate has a nonlocal floor/bridge bypass");
    const auto approach=open->route({ng::lever_plane,*lever_cell},ng::lever_position,exit,entry);require(approach.has_value(),"open lever-to-entry approach unreachable");legal(*open,*approach);
    const auto continue_north=shut->route(to,bridge,exit,entry);require(continue_north.has_value(),"closure trapped an actor already through the physical gate before its portal");legal(*shut,*continue_north);
    auto return_view=ng::navigation_view(north_mesh,ng::Scene::north147,true);
    const nav::Point arrival{-11574,-16170},return_exit{-11711,-16914};
    const auto arrival_cell=north_mesh.locate_cell(0,arrival),return_cell=north_mesh.locate_cell(22,return_exit);require(arrival_cell&&return_cell,"North return exact captured endpoints absent");
    const auto return_route=return_view->route({0,*arrival_cell},arrival,{22,*return_cell},return_exit);require(return_route.has_value(),"South timer blocked Northlands return");legal(*return_view,*return_route);
    rejects([&]{ng::navigation_view(north_mesh,ng::Scene::south146,false);},"mismatched map asset accepted");
    rejects([&]{ng::navigation_view(north_mesh,ng::Scene::north147,false);},"closed North return overlay inferred");
    std::ofstream wires(generated/"wire-fixtures.txt");
    const auto row=[&](const char* name,const auto& bytes){wires<<name<<' '<<hex(bytes)<<'\n';};
    row("closed_state",ng::wire_2022(initial[0]));row("closed_animation",ng::wire_2022(initial[1]));
    row("open_start",ng::wire_2022(opened.events[0]));row("open_state",ng::wire_2022(settled.events[0]));row("close_start",ng::wire_2022(closed.events[0]));
    row("lever_actor_2",ng::lever_spawn_2022(2));row("lever_actor_62000",ng::lever_spawn_2022(62000));
    row("door_actor_1",ng::door_spawn_2022(1));row("door_actor_62001",ng::door_spawn_2022(62001));
    require(ng::lever_spawn_2022(62000).size()==99&&ng::door_spawn_2022(62001).size()==99,"nonliving template framing wrong");
    auto invalid=initial[0];invalid.record=static_cast<ng::Record>(99);rejects([&]{ng::wire_2022(invalid);},"invalid scene record kind serialized");
    rejects([]{ng::lever_spawn_2022(0);},"zero private gadget actor serialized");
    std::cout<<"PASS Northlands scoped controller/toggles/replay/chronology/snapshots and physical closed/open trace/click/slide/pursuit access: "<<checks<<" checks\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
