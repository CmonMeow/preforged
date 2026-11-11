#pragma once
#include <cstdint>
#include <unordered_set>
#include <vector>
namespace capture {
class ArrivalTracker {
    std::uint64_t epoch=0;
    bool seeded=false;
    std::unordered_set<std::uint32_t> seen;
public:
    std::vector<std::uint32_t> observe(std::uint64_t instance,const std::vector<std::uint32_t>& ids,std::uint32_t local){
        if(epoch!=instance){epoch=instance;seeded=false;seen.clear();}
        std::vector<std::uint32_t> result;
        for(auto id:ids){
            if(!id||id>=65536)continue;
            const bool fresh=seen.insert(id).second;
            if(seeded&&fresh&&id!=local)result.push_back(id);
        }
        seeded=true;return result;
    }
};
}
