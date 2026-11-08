#pragma once
#include "gw_hex_breaker.h"
#include "gw_combat.h"

namespace fw::gw::skills::effects {
struct HexBreakerTransaction {
    Registry registry;
    HexBreakerReaction reaction;
    combat::Actor caster;
    combat::Hit retaliation;
};
// Selected-recipient proposal only:the hex is prevented even when retaliation
// cannot damage its caster. Preserve the rejection reason instead of inventing
// NPC Health or treating damage immunity as permission to apply the hex.
// nullopt is NOT permission to apply a hex:World must distinguish an absent
// stance from invalid/stale authoritative combat state before committing.
inline std::optional<HexBreakerTransaction> hex_breaker_transaction(const Registry& original,
        Scope owner,Scope caster,combat::Actor actor,unsigned incoming,unsigned owner_level,Time now){
    if(!owner_level||owner_level>255||actor.id!=caster.actor||actor.generation!=caster.generation||
       (actor.health&&!combat::valid(*actor.health)))return {};
    auto registry=original;const auto reaction=take_hex_breaker(registry,owner,caster,incoming,now);
    if(!reaction)return {};
    const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,double(reaction->damage),owner_level,10},true);
    if(hit.reason==combat::Reason::invalid||hit.reason==combat::Reason::unknown_armor)return {};
    return HexBreakerTransaction{std::move(registry),*reaction,std::move(actor),hit};
}
}
