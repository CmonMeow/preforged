#pragma once
#include "gw_npc_combat_catalog.h"

namespace fw::gw::combat::ai::replacement {
struct Prepared {
    npcs::LiveActor actor;
    npcs::control::Token ordinary,spell;
    skills::effects::Scope target;
    std::uint64_t at=0;
    npcs::Energy before,after;
    std::optional<std::uint64_t> original_health_clock;
    std::optional<std::int64_t> original_energy_units;
};
// Stage only: the caller owns the World lock across this call, its ordinary/
// effect-ledger preflight and immediate commit. No callback may mutate the
// source between preparation and commit. Rejected proposals change no actor,
// Energy, action epoch, cooldown, stance, projectile or outcome state.
inline std::optional<Prepared> prepare(const npcs::LiveActor& current,const npcs::Lifetime& lifetime,
    const npcs::control::Token& ordinary,skills::effects::Scope ordinary_target,const SpellTarget& target,std::uint64_t now,
    const Spellbook::Availability& available={}){
    const auto active=current.active_action(lifetime,now);
    if(!active||*active!=ordinary||ordinary.kind!=npcs::control::Action::attack||ordinary.skill||
       ordinary.actor!=current.hex_scope()||now>=ordinary.completes||
       target.scope!=ordinary_target||target.scope.instance!=ordinary.actor.instance||target.scope==ordinary.actor||
       !target.scope.actor||target.scope.actor>65535||!target.scope.generation||
       !target.alive||!target.hostile||!target.same_plane||!target.line_of_sight)return {};
    auto staged=current;
    if(!staged.cancel_action(lifetime,now))return {};
    const auto choice=staged.choose_spell(lifetime,target,now,available);
    if(!choice)return {};
    const auto before=staged.energy(lifetime);if(!before)return {};
    const auto spell=staged.begin_spell(lifetime,*choice,target,now,available);
    if(!spell)return {};
    const auto after=staged.energy(lifetime);if(!after)return {};
    return Prepared{std::move(staged),ordinary,*spell,target.scope,now,*before,*after,
        current.health_clock(lifetime),current.precise_energy_units(lifetime)};
}
// This protects exact-token/source/resource identity against accidental replay.
// It is not a substitute for the caller's same-lock transaction requirement.
inline bool current(const Prepared& prepared,const npcs::LiveActor& actor,const npcs::Lifetime& lifetime,
    skills::effects::Scope target,std::uint64_t now){
    const auto active=actor.active_action(lifetime,now);
    return now==prepared.at&&target==prepared.target&&active&&*active==prepared.ordinary&&
        actor.health_clock(lifetime)==prepared.original_health_clock&&
        actor.precise_energy_units(lifetime)==prepared.original_energy_units;
}
}
