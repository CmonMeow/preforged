#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include "gw_combat.h"
#include <compare>

namespace fw::gw::skills::effects {
// Native 2022 Signet of Shadows metadata plus the archived skill description.
// A signet: no spell-only targeting/protection/refund callbacks are implied.
struct SignetShadowsPlan {
    unsigned rank=0,damage=5,blind_bonus=15;
    float range=1248;
    std::uint64_t activation=1000,aftercast=750,recharge=30000;
    auto operator<=>(const SignetShadowsPlan&)const=default;
};
inline std::optional<SignetShadowsPlan> signet_shadows(unsigned rank){
    const auto* row=find_skill(876);
    if(rank>20||!row||row->profession!=7||row->attribute!=30||row->type!=7||row->special||
       row->energy_cost()!=0||row->health_sacrifice||row->adrenaline||row->target!=5||
       row->activation!=1.f||row->aftercast!=.75f||row->recharge!=30||
       row->scale0!=5||row->scale15!=35||row->bonus0!=15||row->bonus15!=60||
       row->caster_body_resource!=2077||row->target_body_resource!=946)return {};
    return SignetShadowsPlan{rank,interpolate(5,35,rank),interpolate(15,60,rank)};
}
struct SignetShadowsResult {
    combat::Actor target;
    combat::Hit hit;
    bool blinded=false;
};
// Copy-only preparation. World must validate caster/target lifetime, hostility,
// range and health authority at completion, and atomically publish the result.
// Read Blind at completion, not activation; the generic Conditioned bit cannot
// identify it. Blind is neither removed nor applied by this signet.
inline std::optional<SignetShadowsResult> signet_shadows_target(
        const SignetShadowsPlan& plan,combat::Actor target,
        const combat::conditions::State& conditions,unsigned level,
        bool authoritative_hostile,std::uint64_t now){
    const auto canonical=signet_shadows(plan.rank);
    if(!canonical||*canonical!=plan)return {};
    const bool blinded=conditions.has(combat::conditions::Kind::blind,now);
    const auto hit=combat::apply(target,{combat::Kind::armor_ignoring,
        double(plan.damage+(blinded?plan.blind_bonus:0)),level,876},authoritative_hostile);
    if(hit.reason!=combat::Reason::accepted)return {};
    return SignetShadowsResult{target,hit,blinded};
}
// Captured target946 is an impact attached to the victim, owned by the caster.
// Serialize the pinned2022 opcode159, never the modern capture's opcode160.
inline combat::Packet signet_shadows_impact_2022(unsigned caster,unsigned target){
    if(!caster||caster>65535||!target||target>65535||caster==target)
        throw std::invalid_argument("Signet of Shadows actors required");
    combat::Packet packet{159,0};
    combat::detail::put(packet,20);combat::detail::put(packet,target);
    combat::detail::put(packet,caster);combat::detail::put(packet,946);return packet;
}
}
