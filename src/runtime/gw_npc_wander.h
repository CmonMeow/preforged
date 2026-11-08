#pragma once
#include "gw_npc_pursuit_motion.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fw::gw::combat::ai::wander {
// Explicit private-server patrol offsets. Captures establish movement, but do
// not recover retail wander centers, routes, waiting times or retry policy.
struct Policy {
    std::vector<motion::Point> offsets;
    std::uint64_t pause_ms=0,leg_timeout_ms=0;
    std::string evidence;
    bool valid(double leash)const {
        if(!std::isfinite(leash)||leash<=0||offsets.empty()||offsets.size()>32||
           !pause_ms||pause_ms>60000||!leg_timeout_ms||leg_timeout_ms>300000||
           evidence.empty()||evidence.size()>1024)return false;
        for(const auto p:offsets)if(!motion::valid(p)||std::hypot(p.x,p.y)>=leash)return false;
        return true;
    }
};
class State {
    Policy policy_;
    std::size_t cursor_=0;
    std::uint64_t clock_=0,due_=0,started_=0;
    bool active_=false;
    static std::uint64_t after(std::uint64_t now,std::uint64_t delay){
        return now>UINT64_MAX-delay?UINT64_MAX:now+delay;
    }
public:
    State(Policy policy,std::uint64_t now):policy_(std::move(policy)),clock_(now),due_(after(now,policy_.pause_ms)){}
    const Policy& policy()const{return policy_;}
    std::optional<std::uint64_t> leg_deadline()const{
        return active_?std::optional(after(started_,policy_.leg_timeout_ms)):std::nullopt;
    }
    // Suspend every occupied tick. An old idle destination never resumes in
    // the middle of combat, a return-home leg, or unknown-target deferral.
    void suspend(std::uint64_t now){if(now<clock_)return;clock_=now;active_=false;due_=after(now,policy_.pause_ms);}
    std::optional<motion::Point> destination(motion::Point home,std::uint64_t now){
        if(now<clock_||!motion::valid(home)||policy_.offsets.empty())return {};
        clock_=now;
        if(active_&&now-started_>=policy_.leg_timeout_ms){finish(now);return {};}
        if(!active_){if(now<due_)return {};active_=true;started_=now;}
        const auto offset=policy_.offsets[cursor_];
        const motion::Point goal{home.x+offset.x,home.y+offset.y};
        return motion::valid(goal)?std::optional(goal):std::nullopt;
    }
    void finish(std::uint64_t now){
        if(now<clock_||!active_)return;clock_=now;active_=false;
        cursor_=(cursor_+1)%policy_.offsets.size();due_=after(now,policy_.pause_ms);
    }
};
}
