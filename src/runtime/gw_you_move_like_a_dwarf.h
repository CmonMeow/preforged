#pragma once
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <compare>
#include <cstdint>
#include <optional>

namespace fw::gw::skills::effects {
// Detached source-bound effect plan. World owns target validation, immunity,
// knockdown, condition, damage, and packet publication.
struct YouMoveLikeADwarfPlan {
    unsigned skill=2358,norn_rank=0,damage=44;
    std::uint64_t cripple_ms=8000,knockdown_ms=2000;
    auto operator<=>(const YouMoveLikeADwarfPlan&)const=default;
};
inline std::optional<YouMoveLikeADwarfPlan> you_move_like_a_dwarf(unsigned norn_rank){
    const auto* row=find_skill(2358);
    if(norn_rank>10||!row||row->campaign!=4||row->profession||row->attribute!=51||
       row->type!=15||row->special!=524288||row->energy_cost()!=10||
       row->activation||row->aftercast||row->recharge!=10||
       row->scale0!=44||row->scale15!=80||row->duration0!=8||row->duration15!=15||
       row->caster_body_resource!=1912||row->target_body_resource!=1913)return {};
    // Norn ranks 0..5 correspond to the five catalog scaling steps; higher
    // title ranks retain the rank-5 effect. The progression is source-bound.
    static constexpr unsigned damage_by_rank[]{44,51,58,66,73,80};
    static constexpr unsigned cripple_by_rank[]{8,9,11,12,14,15};
    const auto step=std::min(norn_rank,5u);
    return YouMoveLikeADwarfPlan{2358,norn_rank,damage_by_rank[step],
        1000ull*cripple_by_rank[step],2000};
}
}
