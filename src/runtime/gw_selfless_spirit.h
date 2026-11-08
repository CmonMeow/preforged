#pragma once
#include "gw_skill_catalog_generated.h"
#include <algorithm>
#include <array>
#include <optional>

namespace fw::gw::skills::effects {

struct SelflessSpiritPlan {
    unsigned skill=0;
    unsigned rank=0,duration_ms=15000;
    unsigned energy=5, reduction=3;
    unsigned activation_ms=250, aftercast_ms=750, recharge_ms=45000;
    unsigned duration0=15, duration15=20, caster_body=553, target_body=2077;
    bool operator==(const SelflessSpiritPlan&)const=default;
};

inline std::optional<SelflessSpiritPlan> selfless_spirit(unsigned skill,unsigned rank=0){
    if((skill!=1952&&skill!=2095)||rank>12)return {};
    const auto* row=find_skill(skill);
    if(!row||row->campaign!=2||row->profession!=3||row->attribute!=51||
       row->type!=6||row->special!=524288||row->energy_cost()!=5||
       row->health_sacrifice||row->overcast!=99||row->adrenaline||
       row->activation!=.25f||row->aftercast!=.75f||row->recharge!=45||
       row->target||row->scale0!=3||row->scale15!=3||row->bonus0||row->bonus15||
       row->duration0!=15||row->duration15!=20||row->caster_body_resource!=553||
       row->target_body_resource!=2077)return {};
    // Published allegiance-rank progression reaches the rank-6 plateau.
    constexpr std::array<unsigned,7> seconds{15,16,17,17,18,19,20};
    SelflessSpiritPlan result;result.skill=skill;result.rank=rank;
    result.duration_ms=seconds[std::min(rank,6u)]*1000;
    return result;
}

enum class SelflessTarget { unknown, self, other_ally, foe, untargeted };
struct SelflessCostQuote {unsigned before=0, reduction=0, after=0;};

// Pure additive modifier quote. The caller owns target admission, active effect
// lifetime, spell classification and ordering with other cost modifiers.
// Both faction IDs represent one modifier; callers must never sum aliases.
inline std::optional<SelflessCostQuote> quote_selfless_spirit(
        const SelflessSpiritPlan& plan,bool active,bool spell,
        SelflessTarget target,unsigned energy){
    if(selfless_spirit(plan.skill,plan.rank)!=plan||target==SelflessTarget::unknown)return {};
    const unsigned reduction=active&&spell&&target==SelflessTarget::other_ally
        ?(energy<plan.reduction?energy:plan.reduction):0;
    return SelflessCostQuote{energy,reduction,energy-reduction};
}
}
