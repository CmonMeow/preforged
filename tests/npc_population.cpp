#include "gw_world_simulation.h"
#include <iostream>
#include <limits>
int main()try{
    using namespace fw::gw::world;
    Instance world;
    Instance::Npc first;
    first.id=100;first.definition={17,1234,5678,{0,0,0,100},0,8,1,20,u"\x8101\x1234\x1"};
    first.model_files={1234};first.x=10;first.y=20;
    auto second=first;second.id=101;second.x=30;
    world.configure_npcs({first,second});
    const auto original=world.npc_packets();
    if(original.size()!=4||original[0][0]!=85||original[1][0]!=86||original[2][0]!=32||original[3][0]!=32)
        throw std::runtime_error("definitions must be deduplicated and precede spawns");
    auto rejects=[&](const std::vector<Instance::Npc>& population){
        bool rejected=false;try{world.configure_npcs(population);}catch(const std::exception&){rejected=true;}
        if(!rejected||world.npc_packets()!=original)throw std::runtime_error("invalid population changed world");
    };
    rejects({first,first});
    auto invalid=second;invalid.definition.model_file=99;rejects({first,invalid});
    invalid=second;invalid.x=std::numeric_limits<float>::quiet_NaN();rejects({invalid});
    invalid=second;invalid.model_files.resize(9);rejects({invalid});
    invalid=second;invalid.id=0;rejects({invalid});
    for(unsigned scale=0;scale<=5;++scale){
        invalid=second;invalid.definition.visual_adjustment[3]=static_cast<std::uint8_t>(scale);
        rejects({invalid});
    }
    for(const auto scale:{6u,255u}){
        Instance boundary;auto scaled=first;
        scaled.definition.visual_adjustment[3]=static_cast<std::uint8_t>(scale);
        boundary.configure_npcs({scaled});
        const auto packets=boundary.npc_packets();
        if(packets.size()!=3||packets[0][17]!=scale)
            throw std::runtime_error("valid NPC scale boundary changed on wire");
    }
    auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();
    world.join(1,"Alice",0,0,a);world.join(2,"Bob",0,0,b);a->drain();b->drain();
    if(!world.select_target(1,100,101)||!world.call_target(1,100)||world.call_target(1,999))
        throw std::runtime_error("NPC selection/target validation");
    if(a->drain()!=std::vector<Packet>{fw::gw::callout::target_response(1,100)}||!b->drain().empty())
        throw std::runtime_error("NPC target call leaked to another solo party");
    rejects({});
    bool collision=false;try{world.join(100,"Collision",0,0,a);}catch(const std::exception&){collision=true;}
    if(!collision||!a->drain().empty()||!b->drain().empty())throw std::runtime_error("player/NPC ID collision published");
    world.leave(2);a->drain();
    if(world.selection(1)!=std::pair<std::uint32_t,std::uint32_t>{100,101}||world.npc_packets()!=original)
        throw std::runtime_error("player departure changed NPC population");
    world.leave(1);world.configure_npcs({});
    if(!world.npc_packets().empty())throw std::runtime_error("empty-world reconfiguration failed");
    std::cout<<"PASS NPC ownership, atomic configuration, shared definitions and party-scoped targeting\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
