#include "gw_world_directory.h"
#include <array>
#include <iostream>
#include <thread>
#include <vector>
int main()try{
    using fw::gw::world::WorldDirectory;
    std::uint32_t next=100;
    WorldDirectory directory([&]{return next++;});
    const WorldDirectory::Key key{0,148,0,1,0};
    const std::array<std::uint32_t,2> npcs{1,3};
    auto anchor=directory.reserve(key,npcs);
    if(anchor.player()!=2)throw std::runtime_error("NPC identity overlap");
    {
        auto pending=directory.reserve(key,npcs);
        if(pending.world()!=anchor.world()||pending.player()!=4)throw std::runtime_error("district sharing or pending reservation overlap");
        auto consumed=pending;
        pending={};
        auto next_player=directory.reserve(key,npcs);
        if(next_player.player()!=5)throw std::runtime_error("consumed transfer lost active reservation");
    }
    // A continuously occupied district must tolerate more than the entire
    // 16-bit player space in successive reconnects.
    for(unsigned i=0;i<70000;++i){
        auto reconnect=directory.reserve(key,npcs);
        if(reconnect.world()!=anchor.world()||reconnect.player()!=4)throw std::runtime_error("reconnect leaked player identity");
    }
    auto private_world=directory.reserve(std::nullopt,npcs);
    if(private_world.world()==anchor.world()||private_world.player()!=2)throw std::runtime_error("private world collided with public district");
    auto match=directory.reserve_private_pair(npcs);
    auto other_match=directory.reserve_private_pair(npcs);
    if(match[0].world()!=match[1].world()||match[0].player()!=2||match[1].player()!=4||
       match[0].world()==anchor.world()||match[0].world()==private_world.world()||
       match[0].world()==other_match[0].world())throw std::runtime_error("private match sharing/isolation failed");
    const auto match_world=match[1].world();match[0]={};
    if(match[1].world()!=match_world)throw std::runtime_error("one match departure released other player's world");
    std::vector<std::uint32_t> one_slot;
    for(unsigned id=2;id<=32767;++id)one_slot.push_back(id);
    bool pair_full=false;
    try{(void)directory.reserve_private_pair(one_slot);}catch(const std::runtime_error&){pair_full=true;}
    if(!pair_full)throw std::runtime_error("partial match allocation accepted");
    std::vector<WorldDirectory::Lease> concurrent;
    std::mutex results_mutex;
    std::vector<std::thread> workers;
    for(unsigned t=0;t<8;++t)workers.emplace_back([&]{
        for(unsigned i=0;i<100;++i){auto lease=directory.reserve(key,npcs);std::lock_guard lock(results_mutex);concurrent.push_back(std::move(lease));}
    });
    for(auto& worker:workers)worker.join();
    std::set<std::uint32_t> ids;
    for(const auto& lease:concurrent)
        if(lease.world()!=anchor.world()||lease.player()==1||lease.player()==2||lease.player()==3||!ids.insert(lease.player()).second)
            throw std::runtime_error("concurrent player identity collision");
    concurrent.clear();
    auto reused=directory.reserve(key,npcs);
    if(reused.player()!=4)throw std::runtime_error("concurrent release leaked identity");
    // Weak directory records must not exhaust the directory across abandoned maps.
    for(unsigned map=1000;map<6000;++map){auto unused=directory.reserve(WorldDirectory::Key{0,map,0,1,0});}
    std::vector<std::uint32_t> blocked;
    for(std::uint32_t id=1;id<=32767;++id)blocked.push_back(id);
    bool full=false;
    try{(void)directory.reserve(std::nullopt,blocked);}catch(const std::runtime_error&){full=true;}
    if(!full)throw std::runtime_error("player capacity not enforced");
    // Random world IDs must be unique across private as well as public leases.
    std::uint32_t candidate=7;
    WorldDirectory collisions([&]{return candidate;});
    auto first=collisions.reserve(std::nullopt);
    bool rejected=false;
    try{(void)collisions.reserve(key);}catch(const std::runtime_error&){rejected=true;}
    if(!rejected)throw std::runtime_error("active private world ID reused");
    first={};
    auto after_release=collisions.reserve(key);
    if(after_release.world()!=7)throw std::runtime_error("released world remained reserved");
    std::cout<<"PASS 70000 reconnects, concurrent reservations, NPC isolation and world cleanup\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
