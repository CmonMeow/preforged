#include "gw_agent_wire.h"
#include "gw_agent_clock.h"
#include "gw_world_map_dimensions_generated.h"
#include "gw_map_unlocks.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
int main(int argc,char** argv)try{
    using namespace fw::gw::agent;
    if(movement_speed_2022(7,1.f,1)!=Packet{43,0,7,0,0,0,0,0,128,63,1})throw std::runtime_error("click forward gait byte");
    const auto facing=facing_2022(7,0.f,1.f);
    if(facing!=Packet{46,0,7,0,0,0,0,0,0,0,0,0,128,63})throw std::runtime_error("native rotation fixture");
    bool bad_facing=false;try{facing_2022(7,0,0);}catch(const std::invalid_argument&){bad_facing=true;}
    if(!bad_facing)throw std::runtime_error("zero rotation speed accepted");
    const std::array<std::uint32_t,3> maps{31,32,1023};
    const auto unlocks=fw::gw::map_unlocks_2022(maps);
    if(unlocks.size()!=140||unlocks[0]!=147||unlocks[10]!=32||unlocks[15]!=128||unlocks[16]!=1||unlocks[139]!=128)
        throw std::runtime_error("map unlock bitfield boundaries or counted-array encoding");
    bool invalid_map=false;try{const std::array<std::uint32_t,1> invalid{1024};fw::gw::map_unlocks_2022(invalid);}
    catch(const std::invalid_argument&){invalid_map=true;}
    if(!invalid_map)throw std::runtime_error("oversized map unlock accepted");
    for(const auto expected:std::array<std::array<unsigned,3>,3>{{{55,256,512},{148,64,128},{248,64,128}}}){
        const auto dimensions=fw::gw::exploration_dimensions_2022(expected[0]);
        if(dimensions.width!=expected[1]||dimensions.height!=expected[2])
            throw std::runtime_error("world-map exploration dimensions differ from pinned texture layout");
    }
    if(fw::gw::exploration_dimensions_2022(65535).width)
        throw std::runtime_error("unknown map received fabricated exploration dimensions");
    Clock clock;
    if(clock.advance(false,1000)||clock.advance(true,500000)||clock.advance(true,500015)||
       clock.advance(true,500016)!=16||clock.advance(true,500016)||clock.advance(true,500010)||
       clock.advance(true,500475)!=459)
        throw std::runtime_error("agent clock initialization, cadence, delayed tick or regression");
    if(clock.advance(false,600000)||clock.advance(true,900000)||clock.advance(true,900100)!=100)
        throw std::runtime_error("agent clock carried elapsed time across world entry");
    Clock reconnect;
    if(reconnect.advance(true,900100)||reconnect.advance(true,900200)!=100)
        throw std::runtime_error("reconnected agent clock inherited server uptime");
    Clock soak;soak.advance(true,0);std::uint64_t total=0;
    for(std::uint64_t now=137;now<=86400000;now+=137)total+=soak.advance(true,now);
    total+=soak.advance(true,86400137);
    if(total!=86400137)throw std::runtime_error("agent clock loses elapsed time during long session");
    if(movement_tick_2022(100)!=Packet{30,0,100,0,0,0})
        throw std::runtime_error("movement tick differs from retail serializer fixture 1e8064000000");
    if(movement_base_speed_2022(7,330.f)!=Packet{39,0,7,0,0,0,0,0,165,67})throw std::runtime_error("Dash base speed native39 wire");
    bool bad_fraction=false;try{movement_speed_2022(7,1.5f);}catch(const std::invalid_argument&){bad_fraction=true;}if(!bad_fraction)throw std::runtime_error("43 accepts fraction beyond native maximum");
    if(movement_speed_2022(7)!=Packet{43,0,7,0,0,0,0,0,128,63,0})
        throw std::runtime_error("movement speed differs from retail serializer fixture 2b80070000000000803f00");
    if(argc!=2)throw std::runtime_error("fixture path required");
    {
        std::ifstream fixture(std::filesystem::path(argv[1]).parent_path()/"map-unlocks-2022.txt");
        std::string hex;fixture>>hex;
        const std::array<std::uint32_t,1> map{55};auto packet=fw::gw::map_unlocks_2022(map);packet[1]|=0x80;
        if(hex.size()!=packet.size()*2)throw std::runtime_error("retail map-unlock fixture size mismatch");
        for(std::size_t i=0;i<packet.size();++i)if(packet[i]!=std::stoul(hex.substr(i*2,2),nullptr,16))
            throw std::runtime_error("map unlock differs from pinned retail serializer");
    }
    const std::vector<Packet> packets{spawn_2022(7,0x30000007,1.25f,-2.5f),
        spawn_2022(7,0x20000011,50,-75,2,0,-1)};
    std::ifstream in(argv[1]);if(!in)throw std::runtime_error("missing retail spawn fixtures");
    for(auto packet:packets){
        std::string hex;if(!(in>>hex)||hex.size()%2)throw std::runtime_error("invalid spawn fixture");
        Packet expected;
        for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        packet[1]|=0x80;
        if(packet!=expected)throw std::runtime_error("retail agent spawn mismatch");
    }
    const auto rejects=[](auto action){try{action();}catch(const std::invalid_argument&){return true;}return false;};
    for(float invalid:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        if(!rejects([&]{spawn_2022(1,0x20000001,invalid,0);})||
           !rejects([&]{spawn_2022(1,0x20000001,0,invalid);})||
           !rejects([&]{spawn_2022(1,0x20000001,0,0,0,invalid,0);}))
            throw std::runtime_error("nonfinite agent data accepted");
    }
    if(!rejects([]{spawn_2022(0,0x20000001,0,0);})||
       !rejects([]{spawn_2022(1,0x10000001,0,0);})||
       !rejects([]{spawn_2022(1,0x20000001,0,0,0,0,0);}))
        throw std::runtime_error("invalid identity or heading accepted");
    std::cout<<"PASS player/NPC spawn retail fixtures, coordinates and heading validation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
