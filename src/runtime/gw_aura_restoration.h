#pragma once
#include "gw_skill_catalog_generated.h"
#include <optional>
#include <cmath>
#include <cstdint>
#include <compare>
namespace fw::gw::skills::effects {
// Local Aura_of_Restoration wiki: 60s, PvE 200+20*rank percent of
// cost; PvP 200+(200/15)*rank percent. Round the final heal, not the percent.
// Both versions use unmodified spell cost; one Energy at Energy Storage rank8 and above.
// Glyphs/skills/attacks are not spells. Native types come from the pinned catalog.
inline bool is_spell(const SkillRecord& skill){
    return skill.type==5||skill.type==6||skill.type==4||skill.type==9||skill.type==11||skill.type==24||skill.type==25;
}
struct AuraRestoration {unsigned healing=0,energy=0;};
struct AuraRestorationPlan {
    unsigned rank=0;std::uint64_t duration=60000;
    auto operator<=>(const AuraRestorationPlan&)const=default;
};
inline std::optional<AuraRestorationPlan> aura_restoration_plan(unsigned rank){
    const auto* row=find_skill(180);
    if(rank>20||!row||row->type!=6||row->attribute!=12||row->energy_cost()!=5||row->adrenaline||row->health_sacrifice||
       row->activation!=.25f||row->aftercast!=.75f||row->recharge!=20||row->duration0!=60||row->duration15!=60)return {};
    return AuraRestorationPlan{rank};
}
inline std::optional<AuraRestoration> aura_restoration(const SkillRecord& spell,unsigned rank,bool pvp=false){
    if(rank>20||!is_spell(spell))return {};
    return AuraRestoration{static_cast<unsigned>(std::lround(double(spell.energy_cost())*(2.+double(rank)*(pvp?2.:3.)/15.))),rank>=8?1u:0u};
}
}
