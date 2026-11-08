#pragma once
#include "gw_skillbar.h"
#include "gw_aura_restoration.h"
namespace fw::gw::skills::effects {
// Trusted current action observations only. An absent slot means idle;
// an explicitly unknown skill (slot0) cannot establish this conditional area.
// Generic skill actions include signets/glyphs, which are not spells.
inline std::optional<bool> liquid_flame_spreads(std::optional<Slot> skill,bool attacking=false){
    if(attacking)return true;
    if(!skill)return false;
    const auto* row=find_skill(skill->skill);
    if(!row||!row->id||skill->copy>65535)return {};
    return row->type==14||is_spell(*row);
}
}
