#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>

namespace fw::gw::agent {
// Per world connection: clients advance their agent timer by a delta. Host
// uptime and the preceding connection's elapsed time must never be sent.
class Clock {
    std::optional<std::uint64_t> last_;
public:
    static constexpr std::uint32_t interval_ms=16;
    std::uint32_t advance(bool loaded,std::uint64_t now){
        if(!loaded){last_.reset();return 0;}
        if(!last_){last_=now;return 0;}
        if(now<=*last_||now-*last_<interval_ms)return 0;
        const auto delta=static_cast<std::uint32_t>(std::min<std::uint64_t>(now-*last_,0x7fffffff));
        *last_+=delta;
        return delta;
    }
};
}
