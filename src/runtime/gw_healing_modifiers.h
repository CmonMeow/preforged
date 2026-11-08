#pragma once
#include "gw_effect_registry.h"
#include "gw_mark_of_death.h"
#include <span>
namespace fw::gw::skills::effects {
// Reviewed target-side hexes. Same-name hexes do not stack; independent
// reductions multiply. Deep Wound and the joint40% cap are applied at the
// actual healing transaction, never to regeneration, life stealing or health gain.
inline double healing_hex_multiplier(std::span<const Entry> entries,Time now){
    bool defile=false,lingering=false,mark=false;
    for(const auto& entry:entries)if(entry.kind==Category::hex&&entry.applied<=now&&entry.expires&&now<*entry.expires){
        defile|=entry.skill==129;lingering|=entry.skill==142;
        mark|=entry.skill==785;
    }
    // Native Defile Flesh parameter is66% received; Lingering Curse's20 is
    // a reduction. Retail fractional rounding still needs a capture check.
    return (defile?double(find_skill(129)->bonus0)/100.:1.)*
           (lingering?1.-double(find_skill(142)->bonus0)/100.:1.)*
           (mark?1.-double(mark_of_death(0)->reduction)/100.:1.);
}
}
