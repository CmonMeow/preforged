#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <compare>
namespace fw::gw::skills::effects {
// Shared removal/damage plan. Shatter Delusions27 removes the newest Mesmer
// hex from a foe (adjacent damage); Shatter Hex67 removes any newest hex from
// an ally (nearby damage). Smite Hex302 is Holy in-the-area damage and\n// carries separately validated primary-Monk Divine Favor. Only removal damages.
struct ShatterDelusionsPlan {unsigned rank=0,damage=0;float range=1248,radius=166;unsigned skill=27;bool ally=false;bool holy=false;unsigned divine_rank=0;auto operator<=>(const ShatterDelusionsPlan&)const=default;};
inline std::optional<ShatterDelusionsPlan> shatter_delusions(unsigned rank){
    const auto* row=find_skill(27);if(rank>20||!row||row->type!=5||row->profession!=5||row->attribute!=2)return {};
    return ShatterDelusionsPlan{rank,interpolate(row->scale0,row->scale15,rank)};
}
inline std::optional<ShatterDelusionsPlan> shatter_hex(unsigned rank){
    const auto* row=find_skill(67);if(rank>20||!row||row->type!=5||row->profession!=5||row->attribute!=2)return {};
    return ShatterDelusionsPlan{rank,interpolate(row->scale0,row->scale15,rank),1248,252,67,true};
}
inline std::optional<ShatterDelusionsPlan> smite_hex(unsigned rank,unsigned divine=0){
    const auto* row=find_skill(302);
    if(rank>20||divine>20||!row||row->type!=5||row->profession!=3||row->attribute!=14||
       row->energy_cost()!=5||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=12||
       row->scale0!=10||row->scale15!=85||row->caster_body_resource!=2077||row->target_body_resource!=536)return {};
    return ShatterDelusionsPlan{rank,interpolate(10,85,rank),1248,322,302,true,true,divine};
}
inline bool valid_shatter_delusions(const ShatterDelusionsPlan& plan){const auto expected=plan.skill==27?shatter_delusions(plan.rank):plan.skill==67?shatter_hex(plan.rank):plan.skill==302?smite_hex(plan.rank,plan.divine_rank):std::nullopt;return expected&&*expected==plan;}
}
