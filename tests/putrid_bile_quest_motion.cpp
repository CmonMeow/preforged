#include "gw_world_simulation.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bile quest motion line "+std::to_string(at.line()));
}
world::Instance::Npc make_body(unsigned id,double hp,combat::npcs::Relation relation){
    world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;
    n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
    d.relation=relation;d.spawn_health=combat::Health{hp,1000};d.damageable=d.conditions_allowed=true;
    d.knockdown_allowed=true;d.knockdown_evidence="synthetic movement control";n.combat_definition=d;return n;
}
void add_player(world::Instance& w,unsigned id,float x,float y,unsigned hp,unsigned team){
    w.join(id,"Motion "+std::to_string(id),x,y,std::make_shared<world::Outbox>(),0,{}, {},100,team);
    w.set_vitals(id,{hp,1000,100,100},100);
}
std::shared_ptr<const navigation::Mesh> corridor(){
    const auto path=std::filesystem::temp_directory_path()/("bile-route-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".fwnav");
    std::ofstream out(path,std::ios::binary);check(bool(out));
    const auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
    const auto real=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})word(n);
    for(float n:{0.f,0.f,5000.f,400.f})real(n);word(1);word(3);
    for(unsigned i=0;i<3;++i){
        word(i?i-1:0xffffffffu);word(i<2?i+1:0xffffffffu);
        for(unsigned j=0;j<3;++j)word(0xffffffffu);
        const float left=i==0?0.f:i==1?180.f:220.f,right=i==0?180.f:i==1?220.f:5000.f;
        for(float n:{i==1?300.f:0.f,400.f,left,right,left,right})real(n);
    }
    out.close();auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
    std::filesystem::remove(path);return mesh;
}
void verify(world::Instance& w,skills::effects::Scope owner,skills::effects::Scope target,float x,float y){
    check(w.npc_combat_snapshot(target)->health->current==0&&w.pending_putrid_deaths()==0);
    check(w.resources(2)->health==175&&w.resources(3)->health==200);
    const auto deaths=w.take_npc_deaths(owner);check(deaths.size()==1&&deaths[0].target==target&&deaths[0].time==1100);
    check(std::hypot(deaths[0].position.x-x,deaths[0].position.y-y)<.01&&deaths[0].position.plane==0);
    w.tick(6100);check(w.take_npc_deaths(owner).empty()&&w.pending_putrid_deaths()==0);
    check(w.resources(2)->health==175&&w.resources(3)->health==200);
}
int main()try{
    unsigned cases=0;
    for(bool dense:{false,true})for(float speed:{100.f,300.f})for(bool cripple:{false,true})for(bool knockdown:{false,true}){
        world::Instance w;auto guard=make_body(100,1000,combat::npcs::Relation::neutral);guard.x=1000;
        w.configure_npcs({guard});
        const float expected=speed*(1.f-(cripple?.25f:0.f)-(knockdown?.2f:0.f));
        add_player(w,1,0,0,1000,1);add_player(w,2,expected+200,0,200,2);add_player(w,3,expected-300,0,200,2);
        const auto owner=*w.effect_scope(1),guardian=*w.effect_scope(100);
        auto enemy=make_body(101,2,combat::npcs::Relation::hostile_to_players);
        check(w.begin_wave_objective(owner,guardian,73,1,3,1)&&w.spawn_quest_wave(owner,73,1,{enemy},[]{return true;}));
        const auto target=*w.effect_scope(60000);
        if(cripple)check(w.apply_condition(owner,target,combat::conditions::Kind::crippled,100,500));
        check(w.chase_quest_guardian(target,speed,100));
        check(w.degeneration_hex(owner,target,*skills::effects::degeneration_hex(2058,0),100,20));
        if(knockdown){
            if(dense)for(unsigned at=200;at<600;at+=100)w.tick(at);
            check(w.knockdown_npc(target,200,600)==combat::npcs::control::Knockdown::accepted);
        }
        if(dense)for(unsigned at=knockdown?700:200;at<5100;at+=100)w.tick(at);
        check(w.settle_resources(*w.effect_scope(2),5100));w.tick(5100);
        verify(w,owner,target,expected,0);++cases;
    }
    for(bool dense:{false,true}){
        const auto mesh=corridor();navigation::LayeredMesh layered(*mesh);
        const navigation::Point start{160,100},goal{1000,100};
        const auto from=mesh->locate_cell(0,start),to=mesh->locate_cell(0,goal);check(from&&to);
        const auto route=layered.route({0,*from},start,{0,*to},goal);check(route&&route->size()>2);
        double budget=300;navigation::Point expected=start;
        for(std::size_t i=1;i<route->size()&&budget>0;++i){
            const auto p=(*route)[i].point;const double length=std::hypot(p.x-expected.x,p.z-expected.z);
            if(length<=budget){expected=p;budget-=length;}
            else{expected.x+=float((p.x-expected.x)*budget/length);expected.z+=float((p.z-expected.z)*budget/length);budget=0;}
        }
        check(mesh->walkable(0,expected)&&expected.z>200&&expected.x>220);
        world::Instance w;w.configure_navigation(mesh);
        auto guard=make_body(100,1000,combat::npcs::Relation::neutral);guard.x=goal.x;guard.y=goal.z;w.configure_npcs({guard});
        add_player(w,1,start.x,start.z,1000,1);add_player(w,2,expected.x+200,expected.z,200,2);add_player(w,3,10,10,200,2);
        const auto owner=*w.effect_scope(1);auto enemy=make_body(101,2,combat::npcs::Relation::hostile_to_players);enemy.x=start.x;enemy.y=start.z;
        check(w.begin_wave_objective(owner,*w.effect_scope(100),73,1,3,1)&&w.spawn_quest_wave(owner,73,1,{enemy},[]{return true;}));
        const auto target=*w.effect_scope(60000);
        check(w.chase_quest_guardian(target,300,100)&&w.degeneration_hex(owner,target,*skills::effects::degeneration_hex(2058,0),100,20));
        if(dense)for(unsigned at=200;at<5100;at+=100)w.tick(at);
        check(w.settle_resources(*w.effect_scope(2),5100));w.tick(5100);
        verify(w,owner,target,expected.x,expected.z);++cases;
    }
    std::cout<<cases<<" routed quest NPC Bile death-origin cases: control intervals, corners, coarse/fine and one-shot publication\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
