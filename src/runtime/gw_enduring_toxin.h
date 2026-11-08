#pragma once
#include "gw_skill_catalog_generated.h"
#include <cmath>
#include <cstdint>
#include <optional>

namespace fw::gw::skills::effects {
struct EnduringToxinPlan {
    unsigned rank=0, pips=1;
    std::uint64_t period=5000;
};

inline std::optional<EnduringToxinPlan> enduring_toxin(unsigned rank) {
    const auto* row=find_skill(800);
    if(rank>20||!row||row->type!=4||row->attribute!=30||
       row->duration0!=5||row->duration15!=5||row->scale0!=1||row->scale15!=5)
        return {};
    return EnduringToxinPlan{rank,static_cast<unsigned>(std::lround(
        row->scale0+(double(row->scale15)-row->scale0)*rank/15.))};
}

// The caller must supply movement at the scheduled deadline, not movement at
// a late server tick. Expiry consumes the current interval exactly once.
class EnduringToxinTimer {
    std::uint64_t deadline_=0;
    bool active_=false;
public:
    enum class Result { none, deferred, renewed, expired };
    bool start(std::uint64_t at) {
        if(active_||!enduring_toxin(0)||at>UINT64_MAX-5000)return false;
        deadline_=at+5000;active_=true;return true;
    }
    void cancel(){active_=false;}
    bool active()const{return active_;}
    std::optional<std::uint64_t> deadline()const{
        return active_?std::optional(deadline_):std::nullopt;
    }
    Result settle(std::uint64_t now,std::optional<bool> moving_at_deadline){
        if(!active_||now<deadline_)return Result::none;
        if(!moving_at_deadline)return Result::deferred;
        if(*moving_at_deadline&&deadline_<=UINT64_MAX-5000){
            deadline_+=5000;return Result::renewed;
        }
        active_=false;return Result::expired;
    }
};
}
