#pragma once
#include <cstdint>
#include <optional>
namespace fw::gw {
// One untagged native12 challenge at a time. CMSG9 carries frame interval,
// not latency. The sample includes transport and client dispatch delay.
class PingChallenge {
    std::optional<std::uint64_t> sent_;
public:
    bool begin(std::uint64_t now){
        if(sent_&&now>=*sent_&&now-*sent_<=5000)return false;
        sent_=now;return true;
    }
    std::optional<std::uint32_t> complete(std::uint64_t now){
        if(!sent_)return {};
        const auto sent=*sent_;sent_.reset();
        if(now<sent||now-sent>5000)return {};
        // Client average ignores zero; clamp sub-millisecond local samples.
        return static_cast<std::uint32_t>(now==sent?1:now-sent);
    }
};
}
