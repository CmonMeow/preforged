#pragma once
#include "gw_effect_registry.h"
#include "gw_weapon_spells.h"
#include "gw_lightning_reflexes.h"
#include "gw_escape.h"
#include "gw_natural_stride.h"
#include "gw_distortion.h"
#include "gw_shield_stance.h"
#include "gw_defensive_stance.h"
#include "gw_gladiator_defense_policy.h"
#include "gw_bonetti_defense_policy.h"
#include "gw_whirling_defense_policy.h"
#include "gw_aegis.h"
#include "gw_wary_stance.h"
#include "gw_frenzied_defense.h"
#include "gw_shield_bash.h"
#include <cmath>
#include <span>
#include "gw_attack_outcomes.h"
#include "gw_shield_deflection.h"

namespace fw::gw::skills::effects {
struct RigorMortisPlan {
    unsigned rank=0;
    Time duration=8000;
    float range=1248;
    auto operator<=>(const RigorMortisPlan&)const=default;
};
inline std::optional<RigorMortisPlan> rigor_mortis(unsigned rank){
    const auto* row=find_skill(137);
    if(rank>20||!row||row->type!=4||row->attribute!=7||row->duration0!=8||row->duration15!=20)return {};
    return RigorMortisPlan{rank,static_cast<Time>(std::lround(8.+12.*rank/15.))*1000};
}
// Pass the defender's current generation-scoped registry entries. This is
// only the block decision: bypassing it must never turn a miss, obstruction,
// dodge, cancelled attack or retired target into a successful hit.
inline bool rigor_mortis_prevents_block(std::span<const Entry> entries,Time now){
    for(const auto& entry:entries){
        if(entry.skill!=137||entry.kind!=Category::hex||!entry.instance||!entry.cause||
           entry.applied>now||!entry.expires||now>=*entry.expires)continue;
        if(rigor_mortis(entry.rank))return true;
    }
    return false;
}
// Independent blocking effects multiply their chances of being bypassed.
// Callers select active, stackable effects before supplying probabilities.
// Invalid inputs remain invalid even when an attack bypasses blocking.
inline std::optional<double> attack_block_probability(std::span<const double> chances,
        bool unblockable,std::span<const Entry> defender,Time now){
    double pass=1;
    for(double chance:chances){
        if(!std::isfinite(chance)||chance<0||chance>1)return {};
        pass*=1-chance;
    }
    if(unblockable||rigor_mortis_prevents_block(defender,now))return 0.;
    return 1-pass;
}
struct GuardianBlockPlan {
    unsigned rank=0;
    Time duration=2000;
    double chance=.5;
    auto operator<=>(const GuardianBlockPlan&)const=default;
};
inline std::optional<GuardianBlockPlan> guardian_block(unsigned rank){
    const auto* row=find_skill(258);
    if(rank>20||!row||row->type!=6||row->attribute!=15||row->duration0!=2||row->duration15!=7||
       row->scale0!=50||row->scale15!=50)return {};
    return GuardianBlockPlan{rank,static_cast<Time>(std::lround(2.+5.*rank/15.))*1000};
}
// Reviewed Guardian and Lightning Reflexes sources. Unknown effects are not
// assigned guessed chances. Duplicate same-name enchantments never stack.
inline std::optional<double> reviewed_block_probability(std::span<const Entry> defender,bool unblockable,Time now,std::optional<ShieldEquipment> equipment={},std::optional<unsigned> strength={},unsigned attack_skill=0,std::optional<Entry> owned_warding={}){
    const auto shield=shield_stance_protection(defender,now,equipment,strength);if(!shield)return {};
    bool guardian=false,deflection=false;
    for(const auto& entry:defender){
        if((entry.skill!=258&&entry.skill!=259)||entry.kind!=Category::enchantment||!entry.instance||!entry.cause||
           entry.applied>now||!entry.expires||now>=*entry.expires)continue;
        if(entry.skill==258){if(!guardian_block(entry.rank))return {};guardian=true;}
        else{if(!shield_deflection(entry.rank))return {};deflection=true;}
    }
    double chances[16];unsigned count=0;
    const auto bash=shield_bash_protection(defender,now,equipment,attack_skill);if(!bash)return {};if(bash->instance)chances[count++]=1.;
    const auto frenzied=frenzied_defense_active(defender,now);if(!frenzied)return {};if(*frenzied)chances[count++]=.75;
    const auto wary=wary_would_block(defender,attack_skill,now);if(!wary)return {};if(*wary)chances[count++]=1.;
    for(const auto& e:defender)if(e.skill==380&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires){if(!bonetti_defense(e.rank))return {};chances[count++]=.75;break;}
    for(const auto& e:defender)if(e.skill==372&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires){if(!gladiator_defense(e.rank))return {};chances[count++]=.75;break;}
    for(const auto& e:defender)if(e.skill==345&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires){if(!defensive_stance(e.rank))return {};chances[count++]=.75;break;}
    for(const auto& e:defender)if(e.skill==450&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires){if(!whirling_defense(e.rank))return {};chances[count++]=.75;break;}
    for(const auto& e:defender)if(e.skill==257&&e.kind==Category::enchantment&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires){if(!aegis(e.rank))return {};chances[count++]=.5;break;}
    if(owned_warding)chances[count++]=.5;
    if(shield->block)chances[count++]=shield->block;
    if(guardian)chances[count++]=.5;
    if(deflection)chances[count++]=.75;
    if(lightning_reflexes_active(defender,now))chances[count++]=.75;
    if(escape_active(defender,now))chances[count++]=.75;
    for(const auto& entry:defender)if(entry.skill==1727&&entry.kind==Category::stance&&entry.instance&&entry.cause&&entry.applied<=now&&entry.expires&&now<*entry.expires){
        const auto natural=natural_stride_entry(entry,now);if(!natural)return {};chances[count++]=double(natural->block_percent)/100.;break;
    }
    for(const auto& entry:defender)if(entry.skill==11&&entry.kind==Category::stance&&entry.instance&&entry.cause&&entry.applied<=now&&entry.expires&&now<*entry.expires){
        if(!distortion(entry.rank))return {};chances[count++]=.75;break;
    }
    return attack_block_probability(std::span<const double>(chances,count),unblockable,defender,now);
}
// Contact resolution is separate from damage and hit qualification. Preserve
// an earlier terminal failure; only an otherwise successful contact can block.
// The server owns the draw; client input must not choose it.
inline std::optional<combat::weapon::AttackOutcome> resolve_block_contact(combat::weapon::AttackOutcome earlier,
        double probability,double draw){
    using Outcome=combat::weapon::AttackOutcome;
    if(!std::isfinite(probability)||probability<0||probability>1||!std::isfinite(draw)||draw<0||draw>=1)return {};
    if(earlier!=Outcome::hit)return earlier;
    return draw<probability?Outcome::block:Outcome::hit;
}
}


