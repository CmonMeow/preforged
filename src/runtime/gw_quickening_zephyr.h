#pragma once
#include "gw_skill_effect_plan.h"
#include <cstdint>
#include <optional>

namespace fw::gw::skills::effects {
struct QuickeningZephyrPlan {
    unsigned rank=0,level=0;
    std::uint64_t lifetime=0;
    bool operator==(const QuickeningZephyrPlan&)const=default;
};

// The archived pre-Reforged description and the pinned 2022 catalogue agree
// on a level 1..10 spirit with a 15..45-second lifetime at ranks 0..15.
inline std::optional<QuickeningZephyrPlan> quickening_zephyr(unsigned rank){
    const auto* row=find_skill(475);
    if(rank>20||!row||row->campaign!=0||row->profession!=2||
       row->attribute!=24||row->type!=22||row->special!=256||
       row->energy_cost()!=25||row->activation!=5.f||
       row->aftercast!=.75f||row->recharge!=60||row->target!=0||
       row->scale0!=1||row->scale15!=10||
       row->duration0!=15||row->duration15!=45)return {};
    return QuickeningZephyrPlan{rank,interpolate(1,10,rank),
                               std::uint64_t(interpolate(15,45,rank))*1000};
}
}
