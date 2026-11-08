#pragma once
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <cstdint>
#include <optional>

namespace fw::gw::skills::effects {

struct DodgeThisPlan {
    unsigned skill=2354,rank=0,bonus_damage=14;
    std::uint64_t duration_ms=16000;
};

inline std::optional<DodgeThisPlan> dodge_this(unsigned rank) {
    const auto* row=find_skill(2354);
    if(rank>10||!row||row->campaign!=4||row->profession||row->attribute!=51||
       row->type!=15||row->special!=524288||row->energy_cost()!=0||
       row->activation||row->aftercast||row->recharge!=0||row->adrenaline!=100||
       row->scale0!=14||row->scale15!=20||row->duration0!=16||row->duration15!=20||
       row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
    static constexpr unsigned damage[]{14,15,16,18,19,20};
    static constexpr unsigned seconds[]{16,17,18,18,19,20};
    const auto index=std::min(rank,5u);
    return DodgeThisPlan{2354,rank,damage[index],1000ull*seconds[index]};
}

struct IAmTheStrongestPlan {
    unsigned skill=2355,rank=0,charges=5,bonus_damage=14;
};

// A charge belongs to a successful contact, not to an attack animation.
// One scythe swing can therefore spend more than one charge.
struct IAmTheStrongestCharges {
    unsigned remaining=0,bonus_damage=0;
    std::uint64_t expires_at=0;
    bool active(std::uint64_t now)const {return remaining&&now<expires_at;}
    unsigned on_hit(std::uint64_t now){
        if(!active(now))return 0;
        --remaining;
        return bonus_damage;
    }
};

inline std::optional<IAmTheStrongestCharges> begin_i_am_the_strongest(
    const IAmTheStrongestPlan& plan,std::uint64_t now,
    const IAmTheStrongestCharges* previous=nullptr){
    if(now>UINT64_MAX-27000000ull)return {};
    // Retail anomaly: reapplying this shout ends the current effect.
    if(previous&&previous->active(now))return IAmTheStrongestCharges{};
    return IAmTheStrongestCharges{plan.charges,plan.bonus_damage,now+27000000ull};
}

inline std::optional<IAmTheStrongestPlan> i_am_the_strongest(unsigned rank) {
    const auto* row=find_skill(2355);
    if(rank>10||!row||row->campaign!=4||row->profession||row->attribute!=51||
       row->type!=15||row->special!=524288||row->energy_cost()!=5||
       row->activation||row->aftercast||row->recharge!=20||
       row->scale0!=5||row->scale15!=8||row->bonus0!=14||row->bonus15!=20||
       row->caster_body_resource!=1909||row->target_body_resource!=2077)return {};
    static constexpr unsigned charges[]{5,6,6,7,7,8};
    static constexpr unsigned damage[]{14,15,16,18,19,20};
    const auto index=std::min(rank,5u);
    return IAmTheStrongestPlan{2355,rank,charges[index],damage[index]};
}

}
