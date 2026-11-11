#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <set>
namespace capture {
// One request per quest per instance/character context. Call on the game thread
// only, after a compatible native binding and a complete log are established.
struct QuestInfoQueue {
    std::uint64_t epoch=0,last=0;std::uint32_t actor=0;bool started=false;
    std::set<std::uint32_t> requested;
    void reset(){*this={};}
    std::optional<std::uint32_t> next(std::uint64_t instance,std::uint32_t owner,
        std::uint64_t now,const std::map<std::uint32_t,bool>& loaded,bool binding_ready){
        if(!binding_ready||!owner){reset();return {};}
        if(epoch!=instance||actor!=owner){reset();epoch=instance;actor=owner;}
        if(started&&(now<last||now-last<1000))return {};
        // Completed detail requests may change the log asynchronously. Requests
        // are never inferred to succeed from dispatch alone.
        for(const auto& [id,complete]:loaded)if(id&&!complete&&!requested.contains(id)){
            requested.insert(id);last=now;started=true;return id;
        }
        return {};
    }
};
}
