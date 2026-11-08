#pragma once
#include "gw_selfless_spirit.h"
#include "gw_aura_restoration.h"
#include <cstdint>

namespace fw::gw::skills::effects {

// Semantic family must come from the admitted server effect plan. Neither the
// catalog target byte nor the selected/normalized network target establishes it.
enum class SelflessCastFamily {
    unsupported,
    direct_ally_heal, ally_status_heal, ally_enchantment,
    ally_weapon_spell, ally_condition_removal, ally_hex_removal,
    ally_hex_copy, ally_condition_transfer, ally_skill_copy,
    targeted_ally_resurrection,
    hostile_target, self_effect, party_effect, caster_area,
    corpse_effect, ground_effect, untargeted_effect,
    nonspell
};
enum class SelflessRelation { unknown, self, ally, foe, neutral };

// Relation and admission are authoritative World facts for the captured target
// lifetime at payment, not a client assertion or a team-number comparison.
// Resurrection admission may describe a dead ally: no living-only rule here.
struct SelflessTargetFacts {
    std::uint32_t caster=0, target=0;
    SelflessRelation relation=SelflessRelation::unknown;
    bool admitted=false;
};

inline bool selfless_ally_targeted_family(SelflessCastFamily family){
    switch(family){
    case SelflessCastFamily::direct_ally_heal:
    case SelflessCastFamily::ally_status_heal:
    case SelflessCastFamily::ally_enchantment:
    case SelflessCastFamily::ally_weapon_spell:
    case SelflessCastFamily::ally_condition_removal:
    case SelflessCastFamily::ally_hex_removal:
    case SelflessCastFamily::ally_hex_copy:
    case SelflessCastFamily::ally_condition_transfer:
    case SelflessCastFamily::ally_skill_copy:
    case SelflessCastFamily::targeted_ally_resurrection:return true;
    default:return false;
    }
}

inline SelflessTarget classify_selfless_target(SelflessCastFamily family,
        const SelflessTargetFacts& facts){
    if(family==SelflessCastFamily::unsupported)return SelflessTarget::unknown;
    // An incidental selected ally cannot make an untargeted/party/area skill
    // eligible. In particular, party resurrection and corpse wells are not
    // targeted ally spells, even when their results benefit other allies.
    switch(family){
    case SelflessCastFamily::self_effect:return SelflessTarget::self;
    case SelflessCastFamily::party_effect:
    case SelflessCastFamily::caster_area:
    case SelflessCastFamily::corpse_effect:
    case SelflessCastFamily::ground_effect:
    case SelflessCastFamily::untargeted_effect:
    case SelflessCastFamily::nonspell:return SelflessTarget::untargeted;
    default:break;
    }
    if(!facts.admitted||!facts.caster||!facts.target)return SelflessTarget::unknown;
    if(family==SelflessCastFamily::hostile_target){
        return facts.target!=facts.caster&&facts.relation==SelflessRelation::foe
            ?SelflessTarget::foe:SelflessTarget::unknown;
    }
    if(!selfless_ally_targeted_family(family))return SelflessTarget::unknown;
    if(facts.target==facts.caster)
        return facts.relation==SelflessRelation::self?SelflessTarget::self:SelflessTarget::unknown;
    return facts.relation==SelflessRelation::ally?SelflessTarget::other_ally:SelflessTarget::unknown;
}

inline std::optional<SelflessCostQuote> quote_selfless_targeted_spell(
        const SelflessSpiritPlan& plan,bool active,const SkillRecord& skill,
        SelflessCastFamily family,const SelflessTargetFacts& facts,unsigned energy){
    return quote_selfless_spirit(plan,active,is_spell(skill),
        classify_selfless_target(family,facts),energy);
}
}
