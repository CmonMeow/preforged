#pragma once
#include "gw_skill_catalog_generated.h"
#include <cmath>
#include <optional>
#include <cstdint>

namespace fw::gw::skills::effects {
struct DirectHeal {unsigned skill;float range;bool self;bool below_half_bonus;unsigned divine_multiplier;};
// Reviewed semantic table, independent of player learned/profession policy.
// A trusted NPC actor may use the same effect after separate engine eligibility.
inline std::optional<DirectHeal> direct_heal(unsigned id){switch(id){
case 1:return DirectHeal{id,0.f,true,false,0}; // Healing Signet: self only, Tactics, no Divine Favor.
case 293:return DirectHeal{id,1248.f,true,false,0}; // Signet: Divine Favor rank, no spell bonus.
case 281:return DirectHeal{id,1248.f,true,false,1}; // Orison
case 282:return DirectHeal{id,1248.f,true,true,1}; // Word of Healing
case 1120: // Jamei's Gaze: exact Factions duplicate of Heal Other.
case 286:return DirectHeal{id,1248.f,false,false,1}; // Heal Other
case 313:return DirectHeal{id,144.f,true,false,2}; // Healing Touch
case 958:return DirectHeal{id,624.f,false,false,1}; // Healing Whisper
case 959:{ // Ethereal Light: native easily-interrupted spell,normal Divine Favor.
 const auto* r=find_skill(id);
 if(!r||r->type!=5||r->profession!=3||r->attribute!=13||r->special!=32||r->energy_cost()!=5||
    r->activation!=1.f||r->aftercast!=.75f||r->recharge!=5||r->scale0!=25||r->scale15!=100||
    r->caster_body_resource!=1258||r->target_body_resource!=2077)return {};
 return DirectHeal{id,1248.f,true,false,1};
}
case 867:return DirectHeal{id,1248.f,true,false,1}; // Healing Light
case 1686:return DirectHeal{id,1248.f,true,false,1}; // Glimmer of Light
case 1687:{ // Zealous Benediction: the below-half reward is Energy, not extra healing.
 const auto* r=find_skill(id);
 if(!r||r->profession!=3||r->attribute!=15||r->type!=5||r->special!=4||
    r->energy_cost()!=10||r->activation!=.75f||r->aftercast!=.75f||r->recharge!=4||
    r->scale0!=30||r->scale15!=180||r->bonus0!=7||r->bonus15!=7||
    r->target_body_resource!=501)return {};
 return DirectHeal{id,1248.f,true,false,1};
}
default:return std::nullopt;
}}
inline unsigned heal_scale(unsigned low,unsigned high,unsigned rank){return static_cast<unsigned>(std::lround(double(low)+(double(high)-low)*rank/15.));}
inline std::optional<unsigned> direct_heal_amount(unsigned skill,unsigned rank,unsigned divine,
                                                 unsigned health,unsigned maximum){
    const auto effect=direct_heal(skill);const auto* row=find_skill(skill);
    if(!effect||!row||rank>20||divine>20||!maximum||health>maximum)return std::nullopt;
    auto amount=heal_scale(row->scale0,row->scale15,rank);
    if(effect->below_half_bonus&&std::uint64_t(health)*2<maximum)amount+=heal_scale(row->bonus0,row->bonus15,rank);
    amount+=static_cast<unsigned>(std::lround(3.2*divine*effect->divine_multiplier));
    return amount;
}
// World owns fractional Health. UI-rounded Health must not suppress Word's
// below-half bonus (for example499.5/1000 is still below half).
inline std::optional<unsigned> direct_heal_amount_precise(unsigned skill,unsigned rank,unsigned divine,
                                                         double health,unsigned maximum){
    if(!std::isfinite(health)||health<0||!maximum||health>maximum)return {};
    auto amount=direct_heal_amount(skill,rank,divine,maximum,maximum);
    const auto plan=direct_heal(skill);const auto* row=find_skill(skill);
    if(!amount||!plan||!row)return {};
    if(plan->below_half_bonus&&health*2<double(maximum))
        *amount+=heal_scale(row->bonus0,row->bonus15,rank);
    return amount;
}
}
