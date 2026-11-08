#pragma once
#include "gw_temporary_health.h"
#include "gw_shield_absorption.h"
#include "gw_reversal_fortune.h"
#include "gw_healing_modifiers.h"
namespace fw::gw::skills::effects {
// Incoming Hit must come from a reviewed combat resolver. This checks scoped
// Health/counter continuity, not the provenance of armor or equipment facts.
// Discardable admission state. World retains ownership of real end reactions,
// damage/Seed/death publication, and fresh authoritative commit.
struct PlayerDamageAdmission {
    Registry registry;
    TemporaryHealthSnapshot health;
    ShieldAbsorption absorption;
};
struct PlayerDamageStep {
    PlayerDamageAdmission state;
    // Absent only when conversion healing itself kills. An accepted remaining
    // event is present even when its damage is zero. This Hit retains ordinary
    // damage arithmetic for strict packet validation. Deep Wound debt can make
    // state.health differ from actual->after; survival comes from state.health.
    // World must publish that settled Health separately. No World receipt is issued.
    std::optional<combat::Hit> actual;
    std::optional<ReversalDamageReceipt> reversal;
};
inline std::optional<PlayerDamageStep> project_player_protected_hit(
        const PlayerDamageAdmission& before,const combat::Hit& incoming,Time now){
    const auto target=before.health.owner;const auto& health=before.health;
    if(!before.registry.matches(target)||target.actor>65535||health.at!=now||
       !health.capacity.base||health.capacity.base>1000000||
       health.capacity.reduction!=(health.capacity.active?std::min(100u,health.capacity.base/5):0u)||
       !std::isfinite(health.current)||health.current<=0||health.current>health.capacity.maximum()||
       !std::isfinite(health.capacity.debt)||health.capacity.debt<0||
       incoming.reason!=combat::Reason::accepted||incoming.target!=target.actor||incoming.generation!=target.generation||
       !incoming.skill||incoming.skill>3432||static_cast<unsigned>(incoming.kind)>static_cast<unsigned>(combat::Kind::holy)||
       !std::isfinite(incoming.damage)||incoming.damage<0||incoming.damage>16000000||
       incoming.before.current!=health.current||incoming.before.maximum!=health.capacity.maximum())return {};
    PlayerDamageStep result{before,{},{}};auto& next=result.state;
    const bool protected_now=bool(next.registry.newest_skill(target,1399,now));
    if(incoming.cumulative_reduction){
        if(!protected_now||!next.absorption.commit_receipt(next.registry,target,*incoming.cumulative_reduction,now))return {};
    }else if(protected_now)return {};
    auto remaining=incoming.damage;
    if(remaining>0&&next.registry.newest_skill(target,307,now)){
        const auto quote=prepare_reversal_fortune(next.registry,target,ReversalInput::damage,remaining,now);
        if(!quote)return {};
        const auto removed=consume_reversal_fortune(next.registry,*quote,now);if(!removed)return {};
        const combat::Health prior{next.health.current,next.health.capacity.maximum()};
        const auto multiplier=healing_hex_multiplier(next.registry.active(target,now),now);
        if(!std::isfinite(multiplier)||multiplier<=0||multiplier>1)return {};
        next.health.capacity.event(next.health.current,quote->split.healing,true,multiplier);
        const combat::Health after{next.health.current,next.health.capacity.maximum()};
        result.reversal=ReversalDamageReceipt{*removed,prior,after,{},*quote};
        remaining=quote->split.remaining;
        if(next.health.current<=0)return result;
    }
    auto hit=incoming;hit.damage=remaining;
    hit.before={next.health.current,next.health.capacity.maximum()};
    hit.applied=std::min(remaining,next.health.current);
    hit.after={std::max(0.,hit.before.current-remaining),hit.before.maximum};
    hit.killed=hit.applied>0&&hit.after.current<=0;
    next.health.capacity.event(next.health.current,-remaining);
    hit.effects=next.registry.presentation(target,hit.effects&~0x10u,now)|(hit.killed?0x10u:0u);
    result.actual=hit;if(result.reversal)result.reversal->damage=hit;
    return result;
}
}
