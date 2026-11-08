#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <cmath>
#include <optional>

namespace fw::gw::skills::effects {

// A cast/claim plan only. Ground field creation and periodic effects are
// separate consumers and must succeed in the same World transaction.
struct WellCastPlan {
    unsigned skill=0,rank=0,energy=0,recharge=0;
    Time activation=0;
    auto operator<=>(const WellCastPlan&)const=default;
};

inline std::optional<WellCastPlan> well_cast_plan(unsigned skill,unsigned rank){
    if(rank>20)return {};
    const auto* row=find_skill(skill);
    if(!row||row->profession!=4||row->type!=9||row->health_sacrifice||row->adrenaline||
       row->aftercast!=.75f||row->caster_body_resource!=2077||row->target_body_resource!=2077)return {};
    const unsigned cost=skill==91?5:skill==94?25:10;
    const unsigned recharge=skill==91?15:skill==92?2:10;
    const float activation=skill==94?3.f:1.f;
    const unsigned attribute=skill==91||skill==92?4:5;
    if((skill<91||skill>94)||row->attribute!=attribute||row->energy_cost()!=cost||
       row->recharge!=recharge||row->activation!=activation)return {};
    return WellCastPlan{skill,rank,cost,recharge,skill==94?3000ull:1000ull};
}

struct WellCorpseFact {
    Scope scope;
    std::uint64_t revision=0;
    unsigned plane=0;
    float x=0,z=0;
    bool dead=false,eligible=false,consumed=false;
    auto operator<=>(const WellCorpseFact&)const=default;
};

struct WellCorpseClaim {
    WellCastPlan plan;
    Scope caster,corpse;
    std::uint64_t corpse_revision=0;
    unsigned plane=0;
    float x=0,z=0;
    Time completed=0;
};

inline bool valid_well_scope(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}

// Caller supplies authoritative corpse eligibility and range admission after
// the cast completes. A position-only or recycled actor ID is never enough.
inline std::optional<WellCorpseClaim> prepare_well_corpse_claim(
    const WellCastPlan& plan,Scope caster,const WellCorpseFact& corpse,
    bool caster_alive,bool in_spell_range,Time completed){
    const auto canonical=well_cast_plan(plan.skill,plan.rank);
    if(!canonical||*canonical!=plan||!valid_well_scope(caster)||!valid_well_scope(corpse.scope)||
       caster.instance!=corpse.scope.instance||caster==corpse.scope||!caster_alive||
       !in_spell_range||!corpse.dead||!corpse.eligible||corpse.consumed||!corpse.revision||
       corpse.plane>65535||!std::isfinite(corpse.x)||!std::isfinite(corpse.z)||
       !completed)return {};
    return WellCorpseClaim{plan,caster,corpse.scope,corpse.revision,corpse.plane,
                           corpse.x,corpse.z,completed};
}

// Copy-only compare-and-consume. World must commit this result together with
// Energy, recharge, field entity and outbox changes under its instance lock.
inline std::optional<WellCorpseFact> consume_well_corpse_claim(
    const WellCorpseClaim& claim,const WellCorpseFact& current){
    const auto canonical=well_cast_plan(claim.plan.skill,claim.plan.rank);
    if(!canonical||*canonical!=claim.plan||!valid_well_scope(claim.caster)||
       !valid_well_scope(claim.corpse)||claim.caster.instance!=claim.corpse.instance||
       claim.caster==claim.corpse||!claim.completed||claim.plane>65535||
       !std::isfinite(claim.x)||!std::isfinite(claim.z)||current.scope!=claim.corpse||
       current.revision!=claim.corpse_revision||!current.dead||!current.eligible||
       current.consumed||current.plane!=claim.plane||current.x!=claim.x||current.z!=claim.z||
       current.revision==UINT64_MAX)return {};
    auto next=current;next.consumed=true;++next.revision;return next;
}
}
