#pragma once
#include "gw_npc_ordinary_ai.h"
#include "gw_npc_combat_catalog.h"
namespace fw::gw::combat::ai::ordinary {
struct NpcRecipient {Pose pose;std::optional<int> armor;};
inline std::optional<NpcRecipient> opposed_npc_recipient(const npcs::LiveActor& source,const npcs::Lifetime& source_life,
        const npcs::LiveActor& actor,const npcs::Lifetime& expected,float x,float y,unsigned plane,unsigned damage_type,Time now,bool projectile_attack,bool released=false){
    if(!actor.opposed_npc_recipient(expected,source,source_life,released)||!std::isfinite(x)||!std::isfinite(y))return {};
    const auto* state=actor.actor(expected);
    Pose pose{actor.hex_scope(),x,y,plane,state->health->current>0,
        std::max(actor.health_clock(expected).value_or(0),actor.energy_clock(expected).value_or(0))};
    if(pose.clock>now){pose.observed=false;return NpcRecipient{pose,{}};}
    const auto armor=damage_type==3||damage_type==4||damage_type==5||damage_type==11?
        actor.final_caster_armor(expected,damage_type,now,projectile_attack):
        damage_type<=2?actor.final_physical_armor(expected,now,projectile_attack):std::nullopt;
    return NpcRecipient{pose,armor};
}
// Read-only NPC adapter for the existing generic ledger. No player resources,
// duplicate attack clock, Damage record or independent Energy owner is created.
inline std::optional<NpcRecipient> hostile_npc_recipient(const npcs::LiveActor& actor,
        const npcs::Lifetime& expected,float x,float y,unsigned plane,unsigned damage_type,Time now){
    if(!actor.matches(expected)||!actor.hostile_to_players(expected)||!std::isfinite(x)||!std::isfinite(y))return {};
    const auto* state=actor.actor(expected);if(!state||!state->health)return {};
    Pose pose{actor.hex_scope(),x,y,plane,state->health->current>0,
        std::max(actor.health_clock(expected).value_or(0),actor.energy_clock(expected).value_or(0))};
    if(pose.clock>now){pose.observed=false;return NpcRecipient{pose,{}};}
    const auto armor=damage_type==3||damage_type==4||damage_type==5||damage_type==11?
        actor.final_caster_armor(expected,damage_type,now):
        damage_type<=2?actor.final_physical_armor(expected,now):std::nullopt;
    return NpcRecipient{pose,armor};
}
// Actual block/on-hit/reaction callbacks belong to World. This narrow preview
// must be called only after those shared contact gates authorize a hit.
inline std::optional<Hit> apply_reviewed_npc_weapon_contact(npcs::LiveActor& target,
        const npcs::Lifetime& expected,double resolved_damage,unsigned damage_type,Time now){
    if(!std::isfinite(resolved_damage)||resolved_damage<0||!target.matches(expected)||!target.hostile_to_players(expected))return {};
    return weapon::element(damage_type)?target.apply_caster_weapon_damage(expected,resolved_damage,damage_type,now):
        damage_type<=2?target.apply_weapon_damage(expected,resolved_damage,now):std::nullopt;
}
}
