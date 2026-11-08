#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
namespace fw::gw::arena {
// A match result is published once, ten seconds after its first death.
// The survivor is returned to staging and automatically joins its shared queue.
class MatchLifecycle {
    std::array<std::uint32_t,2> players_;
    std::array<bool,2> alive_{true,true};
    std::array<std::optional<std::uint64_t>,2> ready_at_;
    std::optional<std::uint64_t> started_at_;
    std::optional<std::uint64_t> returns_at_;
    bool delivered_=false;
public:
    struct Result {std::array<std::uint32_t,2> returning;std::optional<std::uint32_t> requeue;};
    explicit MatchLifecycle(std::array<std::uint32_t,2> players):players_(players){
        if(!players[0]||!players[1]||players[0]==players[1])throw std::invalid_argument("invalid arena participants");
    }
    // Called only after the authoritative connection has finished loading.
    // The host must gate combat until both exact participants are ready; this
    // helper does not authenticate connections or publish client packets.
    bool mark_ready(std::uint32_t player,std::uint64_t now){
        if(delivered_||returns_at_)return false;
        const auto index=player==players_[0]?0:player==players_[1]?1:2;
        if(index==2)return false;
        if(ready_at_[index])return true;
        ready_at_[index]=now;
        if(ready_at_[0]&&ready_at_[1])started_at_=*ready_at_[0]>*ready_at_[1]?*ready_at_[0]:*ready_at_[1];
        return true;
    }
    bool is_ready(std::uint32_t player)const{
        const auto index=player==players_[0]?0:player==players_[1]?1:2;
        return index<2&&ready_at_[index].has_value();
    }
    std::optional<std::uint64_t> started_at()const{return started_at_;}
    bool death(std::uint32_t player,std::uint64_t now){
        if(delivered_||(returns_at_&&now>=*returns_at_))return false;
        const auto index=player==players_[0]?0:player==players_[1]?1:2;
        if(index==2||!alive_[index])return false;
        if(!returns_at_){if(now>UINT64_MAX-10000)throw std::overflow_error("arena result clock exhausted");returns_at_=now+10000;}
        alive_[index]=false;return true;
    }
    std::optional<std::uint64_t> return_deadline()const{return returns_at_;}
    std::optional<Result> poll(std::uint64_t now){
        if(delivered_||!returns_at_||now<*returns_at_)return {};
        delivered_=true;Result result{players_,{}};
        if(alive_[0]!=alive_[1])result.requeue=players_[alive_[0]?0:1];
        return result;
    }
};
}
