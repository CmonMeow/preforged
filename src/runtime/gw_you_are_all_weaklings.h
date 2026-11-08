#pragma once
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <compare>
#include <cstdint>
#include <optional>

namespace fw::gw::skills::effects {
struct YouAreAllWeaklingsPlan {
    unsigned skill=2359,norn_rank=0;
    std::uint64_t weakness_ms=8000;
    float range=1248,adjacent=166;
    auto operator<=>(const YouAreAllWeaklingsPlan&)const=default;
};
inline std::optional<YouAreAllWeaklingsPlan> you_are_all_weaklings(unsigned rank){
    const auto* row=find_skill(2359);
    if(rank>10||!row||row->campaign!=4||row->profession||row->attribute!=51||
       row->type!=15||row->special!=524288||row->energy_cost()!=5||
       row->activation||row->aftercast||row->recharge!=12||row->target!=5||
       row->duration0!=8||row->duration15!=12||
       row->caster_body_resource!=1914||row->target_body_resource!=1915)return {};
    static constexpr unsigned seconds[]{8,9,10,10,11,12};
    return YouAreAllWeaklingsPlan{2359,rank,1000ull*seconds[std::min(rank,5u)]};
}
}
