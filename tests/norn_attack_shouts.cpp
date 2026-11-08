#include "gw_norn_attack_shouts.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace fw::gw::skills::effects;

int main() try {
    constexpr unsigned damage[]{14,15,16,18,19,20};
    constexpr unsigned duration[]{16,17,18,18,19,20};
    constexpr unsigned charges[]{5,6,6,7,7,8};
    for(unsigned rank=0;rank<=10;++rank){
        const auto dodge=dodge_this(rank);
        const auto strongest=i_am_the_strongest(rank);
        const auto index=std::min(rank,5u);
        if(!dodge||dodge->bonus_damage!=damage[index]||dodge->duration_ms!=1000ull*duration[index])
            throw std::runtime_error("Dodge This Norn progression mismatch");
        if(!strongest||strongest->charges!=charges[index]||strongest->bonus_damage!=damage[index])
            throw std::runtime_error("I Am the Strongest Norn progression mismatch");
    }
    if(dodge_this(11)||i_am_the_strongest(11))throw std::runtime_error("invalid Norn rank accepted");
    const auto rank5=i_am_the_strongest(5);
    auto state=begin_i_am_the_strongest(*rank5,100);
    if(!state||!state->active(100)||state->remaining!=8)throw std::runtime_error("charge start failed");
    for(unsigned hit=0;hit<8;++hit)
        if(state->on_hit(101)!=20)throw std::runtime_error("hit did not spend one charge");
    if(state->on_hit(102)||state->active(102))throw std::runtime_error("spent effect remained active");
    state=begin_i_am_the_strongest(*rank5,200);
    if(!state||state->on_hit(200)!=20)throw std::runtime_error("fresh effect did not hit");
    state=begin_i_am_the_strongest(*rank5,201,&*state);
    if(!state||state->active(201))throw std::runtime_error("reapplication did not end effect");
    state=begin_i_am_the_strongest(*rank5,300);
    if(!state||state->on_hit(27000300ull))throw std::runtime_error("expired effect granted damage");
    std::cout<<"PASS Norn attack shout catalog and progression\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
