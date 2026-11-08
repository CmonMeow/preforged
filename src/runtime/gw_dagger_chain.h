#pragma once
#include "gw_effect_registry.h"
#include <cstdint>
#include <optional>

namespace fw::gw::skills::effects {
// The modern capture sets dagger status 1 on the first landed Dancing
// Daggers missile and resets long-idle chains near 15 seconds. This ledger
// is World-internal until the corresponding 2022 client packet is verified.
struct DaggerChain {
    Scope target{};
    unsigned stage=0;
    std::uint64_t expires=0;

    bool lead(Scope victim,std::uint64_t now){
        if(!victim.actor||!victim.generation||now>UINT64_MAX-15000)return false;
        target=victim;stage=1;expires=now+15000;return true;
    }
    bool offhand(Scope victim,std::uint64_t now){
        if(current(victim,now)!=1||now>UINT64_MAX-15000)return false;
        stage=2;expires=now+15000;return true;
    }
    bool dual(Scope victim,std::uint64_t now){
        if(current(victim,now)!=2||now>UINT64_MAX-15000)return false;
        stage=3;expires=now+15000;return true;
    }
    unsigned current(Scope victim,std::uint64_t now)const{
        return stage&&target==victim&&now<expires?stage:0;
    }
    void clear(){target={};stage=0;expires=0;}
};
}
