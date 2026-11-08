#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>

namespace fw::gw {
// Authenticated clients send periodic traffic while playing or browsing their
// roster. Expire abandoned connections, not active sessions of arbitrary age.
class IdleLifetime {
    std::uint64_t last_activity_;
    std::uint64_t timeout_;
public:
    static constexpr std::uint64_t idle_timeout_ms=5*60*1000;
    explicit IdleLifetime(std::uint64_t now,std::uint64_t timeout=idle_timeout_ms):last_activity_(now),timeout_(timeout){}
    void activity(std::uint64_t now){last_activity_=std::max(last_activity_,now);}
    std::uint64_t deadline()const{
        return last_activity_>std::numeric_limits<std::uint64_t>::max()-timeout_
            ?std::numeric_limits<std::uint64_t>::max():last_activity_+timeout_;
    }
    bool expired(std::uint64_t now)const{return now>=deadline();}
};
using AuthenticatedLifetime=IdleLifetime;
}
