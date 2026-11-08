#pragma once
#include "gw_combat.h"
#include "gw_skill_effect_plan.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>

namespace fw::gw::skills::effects {
struct WardElementsPlan {unsigned rank=0;std::uint64_t duration=0;auto operator<=>(const WardElementsPlan&)const=default;};
inline std::optional<WardElementsPlan> ward_elements_plan(unsigned rank){
 const auto* row=find_skill(175);
 if(rank>20||!row||row->type!=11||row->profession!=6||row->attribute!=9||row->energy_cost()!=15||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=20||row->duration0!=8||row->duration15!=20||row->scale0!=24||row->scale15!=24||row->caster_body_resource!=314||row->target_body_resource!=2077)return {};
 return WardElementsPlan{rank,std::uint64_t(interpolate(8,20,rank))*1000};
}
// Detached ground-effect policy for the pinned 2022 client. The archived wiki
// supplies the area convention (322), duration and +24 elemental armor; the
// one local cast proves activation only, not native ward spawn/expiry packets.
struct WardAgainstElements {
    std::uint64_t map=0;std::uint32_t caster=0;
    std::uint64_t caster_generation=0, created=0, expires=0;
    float x=0, y=0;
    unsigned rank=0;
    static constexpr float radius=322.f;
    static constexpr int elemental_armor=24;
};

inline std::optional<WardAgainstElements> ward_against_elements(
        std::uint64_t map,std::uint32_t caster,std::uint64_t generation,
        float x,float y,unsigned rank,std::uint64_t now) {
    const auto* row=find_skill(175);
    if(!row||row->type!=11||row->profession!=6||row->attribute!=9||
       row->duration0!=8||row->duration15!=20||row->scale0!=24||row->scale15!=24||
       !map||!caster||!generation||rank>20||!std::isfinite(x)||!std::isfinite(y)||
       std::abs(x)>1000000||std::abs(y)>1000000)return {};
    const auto duration=std::uint64_t(interpolate(row->duration0,row->duration15,rank))*1000;
    if(now>std::numeric_limits<std::uint64_t>::max()-duration)return {};
    return WardAgainstElements{map,caster,generation,now,now+duration,x,y,rank};
}

inline bool valid_ward(const WardAgainstElements& ward) {
    const auto canonical=ward_against_elements(ward.map,ward.caster,
        ward.caster_generation,ward.x,ward.y,ward.rank,ward.created);
    return canonical&&canonical->expires==ward.expires;
}

inline bool ward_covers(const WardAgainstElements& ward,std::uint64_t map,
        float x,float y,bool authoritative_ally,bool spirit,std::uint64_t now) {
    if(!valid_ward(ward)||!map||map!=ward.map||!authoritative_ally||spirit||now<ward.created||now>=ward.expires||
       !std::isfinite(x)||!std::isfinite(y)||!std::isfinite(ward.x)||!std::isfinite(ward.y))return false;
    const double dx=double(x)-ward.x,dy=double(y)-ward.y;
    return dx*dx+dy*dy<=double(WardAgainstElements::radius)*WardAgainstElements::radius;
}

// Explicit provisional private overlap policy: multiple copies of the same
// +24 ward do not multiply their armor bonus. Retail overlap is not measured.
// The caller owns map/lifetime/allegiance facts and applies this only to an
// authoritative elemental damage calculation, after validating armor exists.
inline int ward_elemental_armor_bonus(std::span<const WardAgainstElements> wards,
        std::uint64_t map,float x,float y,bool authoritative_ally,bool spirit,
        std::uint64_t now,combat::Kind kind,bool ignores_armor=false) {
    if(ignores_armor||kind==combat::Kind::armor_ignoring)return 0;
    if(kind!=combat::Kind::earth&&kind!=combat::Kind::fire&&
       kind!=combat::Kind::cold&&kind!=combat::Kind::lightning)return 0;
    for(const auto& ward:wards)if(ward_covers(ward,map,x,y,authoritative_ally,spirit,now))
        return WardAgainstElements::elemental_armor;
    return 0;
}
}
