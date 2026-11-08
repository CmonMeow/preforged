#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <compare>
namespace fw::gw::skills::ranger {
// Local Ignite_Arrows.html revision2608468 (2016): each arrow explodes on
// contact, even if Blind/block prevents the weapon hit. Fire is a separate
// adjacent packet, not a conversion of arrow damage. Preparation lasts24s.
struct IgniteArrowsPlan {unsigned rank=0,damage=0;float radius=166;auto operator<=>(const IgniteArrowsPlan&)const=default;};
inline std::optional<IgniteArrowsPlan> ignite_arrows(unsigned rank){
    const auto* row=find_skill(431);if(rank>20||!row||row->type!=19||row->attribute!=24)return {};
    return IgniteArrowsPlan{rank,effects::interpolate(row->scale0,row->scale15,rank)};
}
inline bool valid_ignite_arrows(const IgniteArrowsPlan& plan){const auto expected=ignite_arrows(plan.rank);return expected&&*expected==plan;}
}
