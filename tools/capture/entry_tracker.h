#pragma once
#include <cstdint>
namespace capture {
// Entry candidates require an observed loading interval. Attaching in town
// never arms capture. Delayed or already moving characters are not candidates.
class EntryTracker {
    bool loading_seen=false,pending=false,loaded_before=false;
    std::uint64_t loaded_at=0;
public:
    bool observe(bool loaded,bool local_valid,bool moving,std::uint64_t now){
        if(!loaded){loading_seen=true;pending=false;loaded_before=false;return false;}
        if(!loaded_before){pending=loading_seen;loading_seen=false;loaded_at=now;loaded_before=true;}
        if(!pending)return false;
        if(now-loaded_at>2000||moving){pending=false;return false;}
        if(!local_valid)return false;
        pending=false;return true;
    }
};
}
