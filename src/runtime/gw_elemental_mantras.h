#pragma once
#include "gw_combat.h"
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct ElementalMantra {unsigned skill,rank,reduction,energy;combat::Kind kind;Time duration;};
inline std::optional<ElementalMantra> elemental_mantra(unsigned skill,unsigned rank=0){
    if(rank>20||skill<6||skill>9)return {};
    const auto kind=skill==6?combat::Kind::earth:skill==7?combat::Kind::fire:skill==8?combat::Kind::cold:combat::Kind::lightning;
    return ElementalMantra{skill,rank,interpolate(26,50,rank),2,kind,Time(interpolate(30,90,rank))*1000};
}
inline std::optional<ElementalMantra> active_elemental_mantra(const std::vector<Entry>& active,Time now){
    for(const auto& e:active)if(e.kind==Category::stance&&e.applied<=now&&(!e.expires||now<*e.expires))
        if(auto plan=elemental_mantra(e.skill,e.rank))return plan;
    return {};
}
inline std::optional<ElementalMantra> active_elemental_mantra(const Registry& registry,Scope scope,Time now){
    const auto entry=registry.newest(scope,Category::stance,now);
    return entry?elemental_mantra(entry->skill,entry->rank):std::nullopt;
}
}
