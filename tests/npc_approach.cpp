#include "gw_world_simulation.h"
#include <iostream>

namespace {
using namespace fw::gw;
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
std::shared_ptr<const navigation::Mesh> fixture(const std::filesystem::path& file,bool disconnected=false,float scale=1.f){
    std::ofstream out(file,std::ios::binary|std::ios::trunc);
    const auto word=[&](std::uint32_t value){for(unsigned n=0;n<4;++n)out.put(char(value>>(8*n)));};
    const auto scalar=[&](float value){word(std::bit_cast<std::uint32_t>(value*scale));};
    out.write("FWNAV001",8);for(auto value:{1u,71u,0xeefe704cu,12u,1u,1u})word(value);
    for(float value:{0,0,1000,400})scalar(value);
    word(2);
    // The empty space between left/right rooms is a wall. The only route
    // joins their top edges through the upper corridor, not through the gap.
    word(disconnected?2:3);
    for(unsigned i=0;i<(disconnected?2u:3u);++i){
        word(disconnected?0xffffffffu:i==1?0:1);word(i==1&&!disconnected?2:0xffffffffu);
        for(unsigned n=0;n<3;++n)word(0xffffffffu);
        const float yt=disconnected?0.f:i==1?200.f:0.f,yb=disconnected?400.f:i==1?400.f:200.f;
        const float left=disconnected?(i?220.f:0.f):i==2?800.f:0.f,right=i==0?200.f:1000.f;
        for(float value:{yt,yb,left,right,left,right})scalar(value);
    }
    // XY-overlapping second floor has no DAT connector to the first.
    word(1);for(unsigned n=0;n<5;++n)word(0xffffffffu);
    for(float value:{0,400,0,1000,0,1000})scalar(value);
    out.close();return std::make_shared<const navigation::Mesh>(navigation::Mesh::load(file));
}
world::Instance::Npc make_npc(std::uint32_t id,float x,float y,std::uint16_t plane=0){
    world::Instance::Npc result;result.id=id;
    result.definition={17,1234,5678,{0,0,0,100},0,8,1,20,u"\x8101\x1234\x1"};
    result.model_files={1234};result.x=x;result.y=y;result.plane=plane;result.xunlai_storage=true;return result;
}
}

int main(int argc,char** argv)try{
    using namespace fw::gw::world;
    require(argc==2,"usage: npc_approach_test generated-dir");
    const std::filesystem::path generated=argv[1];std::filesystem::create_directories(generated);
    const auto mesh=fixture(generated/"npc-approach.fwnav");
    {
        Instance distant;distant.configure_navigation(fixture(generated/"long-approach.fwnav",false,60.f));
        distant.configure_npcs({make_npc(100,54000,6000)});
        auto queue=std::make_shared<Outbox>();distant.join(1,"Long Approach",6000,6000,queue,0,{},0);
        require(distant.approach_npc(1,100,0)==Instance::NpcApproach::moving,"long legal approach was rejected");
        distant.tick(130000);require(!distant.take_npc_interaction(1,130000),"long approach authorized interaction before arrival");
        bool arrived=false;
        for(std::uint64_t now=131000;now<=400000;now+=1000){distant.tick(now);if(distant.take_npc_interaction(1,now)){arrived=true;break;}}
        require(arrived,"legal route exceeding120 seconds expired before arrival");
    }
    {
        Instance delayed;delayed.configure_navigation(mesh);delayed.configure_movement_clock([]{return 5000ull;});
        delayed.configure_npcs({make_npc(100,900,100)});
        auto queue=std::make_shared<Outbox>();delayed.join(1,"Delayed Planner",100,100,queue,0,{},0);queue->drain();
        require(delayed.approach_npc(1,100,0)==Instance::NpcApproach::moving,"delayed planner rejected a legal approach");
        delayed.tick(5000);
        require(delayed.position(1,5000)==std::pair{100.f,100.f},"planning latency was incorrectly consumed as movement");
        delayed.tick(5100);const auto position=delayed.position(1,5100);
        require(std::hypot(position.first-100,position.second-100)<=28.81f,"post-planning movement exceeded the speed budget");
        require(position.first<=200&&position.second<=200,"delayed approach crossed the wall instead of following its first segment");
    }
    Instance instance;instance.configure_navigation(mesh);
    instance.configure_npcs({make_npc(100,900,100),make_npc(101,100,100,1)});
    const auto queue=std::make_shared<Outbox>();instance.join(1,"Visitor",100,100,queue,0,{},0);queue->drain();
    require(instance.approach_npc(1,100,0)==Instance::NpcApproach::moving,"distant NPC did not initiate approach");
    require(instance.position(1,0)==std::pair{100.f,100.f},"NPC interaction teleported the player");
    const auto start=queue->drain();
    require(std::any_of(start.begin(),start.end(),[](const auto& p){return p[0]==41;}),"approach did not publish normal destination intent");
    require(std::none_of(start.begin(),start.end(),[](const auto& p){return p[0]==44;}),"approach fabricated a position correction");
    require(!instance.can_access_storage(1,100,0)&&!instance.take_npc_interaction(1,0),"distant storage interaction was authorized early");
    instance.tick(1000);const auto first=instance.position(1,1000);
    const auto turn_packets=queue->drain();
    const auto anchor=std::find_if(turn_packets.begin(),turn_packets.end(),[](const auto& p){return p[0]==44;});
    const auto destination=std::find_if(turn_packets.begin(),turn_packets.end(),[](const auto& p){return p[0]==41;});
    require(anchor!=turn_packets.end()&&destination!=turn_packets.end()&&anchor<destination,
        "route turn must bind the reached position before rebasing native velocity");
    require(first.first>=100&&first.first<900&&first.second>=200,"approach crossed the wall instead of following its nav route");
    require(!instance.take_npc_interaction(1,1000),"NPC dialogue opened before arrival");
    bool arrived=false;std::pair<float,float> stopped;
    for(std::uint64_t now=1100;now<=6000;now+=100){
        instance.tick(now);if(auto actor=instance.take_npc_interaction(1,now)){
            require(*actor==100,"approach returned another actor");stopped=instance.position(1,now);
            require(std::hypot(stopped.first-900,stopped.second-100)<=160,"interaction completed outside range");
            require(instance.plane(1)==0&&instance.can_access_storage(1,100,now),"arrival ignored exact NPC floor or visibility");
            require(!instance.take_npc_interaction(1,now),"NPC dialogue completion repeated");
            instance.tick(now+1000);require(instance.position(1,now+1000)==stopped,"approach kept moving after dialogue arrival");arrived=true;break;
        }
    }
    require(arrived,"validated NPC approach never completed");
    require(instance.approach_npc(1,100,7000)==Instance::NpcApproach::ready,"nearby NPC failed immediate interaction");
    require(instance.approach_npc(1,101,7000)==Instance::NpcApproach::unavailable,"NPC on disconnected overlapping floor became reachable");
    require(instance.approach_npc(1,999,7000)==Instance::NpcApproach::unavailable,"unknown NPC became an interaction target");
    Instance across_wall;across_wall.configure_navigation(fixture(generated/"npc-separated.fwnav",true));
    across_wall.configure_npcs({make_npc(100,225,100)});auto wall_queue=std::make_shared<Outbox>();
    across_wall.join(1,"Visitor",195,100,wall_queue,0,{},0);
    require(!across_wall.can_access_storage(1,100,0)&&
        across_wall.approach_npc(1,100,0)==Instance::NpcApproach::unavailable,
        "nearby NPC across a disconnected wall authorized interaction");
    // Fresh journeys are cancelled by each subsequent authoritative intent.
    for(unsigned operation=0;operation<3;++operation){
        Instance cancelled;cancelled.configure_navigation(mesh);cancelled.configure_npcs({make_npc(100,900,300)});
        auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();cancelled.join(1,"Visitor",100,300,a,0,{},0);
        cancelled.join(2,"Friend",900,300,b,0,{},0);
        require(cancelled.approach_npc(1,100,0)==Instance::NpcApproach::moving,"cancellation fixture could not approach");
        if(operation==0)cancelled.move(1,900,300,100,0);
        if(operation==1)cancelled.steer(1,1000,0,100);
        if(operation==2)require(cancelled.follow(1,2,100),"follow intent rejected");
        const auto checked_at=operation==1?3500u:10000u;cancelled.tick(checked_at);
        require(cancelled.can_access_storage(1,100,checked_at),"cancellation test did not actually reach NPC range");
        require(!cancelled.take_npc_interaction(1,checked_at),"superseded NPC approach still opened dialogue");
    }
    // A replacement click with no valid NPC cancels the old interaction,
    // even though the already validated movement route remains intact.
    Instance replaced;replaced.configure_navigation(mesh);replaced.configure_npcs({make_npc(100,900,100)});
    auto replaced_queue=std::make_shared<Outbox>();replaced.join(1,"Visitor",100,100,replaced_queue,0,{},0);
    require(replaced.approach_npc(1,100,0)==Instance::NpcApproach::moving,"replacement fixture could not approach");
    require(replaced.approach_npc(1,999,100)==Instance::NpcApproach::unavailable,"unknown replacement target accepted");
    replaced.tick(10000);require(replaced.can_access_storage(1,100,10000)&&!replaced.take_npc_interaction(1,10000),
        "invalid replacement left a stale interaction queued");
    Instance timed;timed.configure_navigation(mesh);timed.configure_npcs({make_npc(100,900,100)});
    auto timed_queue=std::make_shared<Outbox>();timed.join(1,"Visitor",100,100,timed_queue,0,{},0);
    require(timed.approach_npc(1,100,0)==Instance::NpcApproach::moving,"timeout fixture could not approach");
    timed.tick(200001);require(!timed.take_npc_interaction(1,200001),"expired interaction survived its route duration plus bounded grace");
    std::cout<<"PASS NPC approach navigation, delayed one-shot interaction, range/floor guards and movement cancellation\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
