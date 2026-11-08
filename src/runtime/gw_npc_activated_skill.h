#pragma once
#include "gw_shadow_damage.h"
#include "gw_aura_restoration.h"
#include "gw_skillbar.h"
#include "gw_empathy.h"
#include <cmath>
#include <optional>
#include <string>

namespace fw::gw::combat::ai::activated {
// Effect delivery and action classification are independent: armor-ignoring
// damage may be delivered by a spell or by a non-spell touch skill.
enum class Kind {spell,skill};
enum class Completion {deferred,completed,stopped};
inline std::optional<Kind> kind(unsigned skill){
    const auto* row=skills::find_skill(skill);
    if(!row||!row->id)return {};
    if(skills::effects::is_spell(*row))return Kind::spell;
    // Pinned 2022 catalog encodes Skill as10 (modern reference JSON uses38).
    if(row->type==10)return Kind::skill;
    return {};
}
struct Reviewed {
    skills::Slot slot;
    skills::effects::ShadowPlan effect;
    std::string evidence;
    std::optional<skills::effects::EmpathyPlan> empathy_effect;
    float effect_range()const{return empathy_effect?1248.f:effect.range;}
    static std::optional<Reviewed> empathy_spell(skills::Slot slot,unsigned rank,std::string evidence){
        const auto plan=skills::effects::empathy(slot.skill,rank);if(!plan)return {};
        Reviewed entry{slot,{},std::move(evidence),*plan};return entry.valid()?std::optional(entry):std::nullopt;
    }
    bool valid()const{
        const auto* row=skills::find_skill(slot.skill);
        const auto canonical=skills::effects::shadow_damage(effect.skill,effect.rank);
        const auto empathy=empathy_effect?skills::effects::empathy(empathy_effect->skill,empathy_effect->rank):std::nullopt;
        const bool hex=empathy_effect&&empathy&&slot.skill==empathy->skill&&empathy_effect->damage==empathy->damage&&
            empathy_effect->reduction==empathy->reduction&&empathy_effect->duration==empathy->duration&&
            effect==skills::effects::ShadowPlan{};
        // The first immediate-damage consumer has no life-steal followup.
        // Reject those plans rather than silently dropping part of an effect.
        return row&&kind(slot.skill)&&slot.copy<=65535&&
            (empathy_effect?hex:(slot.skill==effect.skill&&canonical&&*canonical==effect&&!effect.steal_above_half))&&
            !row->adrenaline&&!row->health_sacrifice&&!((row->special&1)&&row->overcast)&&
            row->energy_cost()<=25&&std::isfinite(row->activation)&&row->activation>0&&row->activation<=60&&
            !evidence.empty()&&evidence.size()<=1024;
    }
};
}
