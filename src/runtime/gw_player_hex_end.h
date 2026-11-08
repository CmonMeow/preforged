#pragma once
#include "gw_degeneration_hexes.h"
#include "gw_wound_spells.h"
#include "gw_temporary_health.h"
#include "gw_fragility.h"
namespace fw::gw::skills::effects {
struct PlayerHexRemovalLease {Scope target;Entry entry;std::uint64_t sequence=0;};
inline std::optional<PlayerHexRemovalLease> capture_player_hex_removal(
        const Registry& registry,Scope target,unsigned effect,Time now){
    if(!registry.matches(target)||target.actor>65535)return {};
    const auto entry=registry.lookup(target,effect);
    const auto sequence=registry.entry_sequence(target,effect);
    if(!entry||!sequence||entry->kind!=Category::hex||entry->cause>65535||
       now<entry->applied||(entry->expires&&now>=*entry->expires))return {};
    return PlayerHexRemovalLease{target,*entry,*sequence};
}
// Local end-state admission data only. This is NOT a World commit receipt:
// external ownership/returns must still be handled, and any listed Fragility
// reactions must settle before this Health can admit subsequent damage.
struct PlayerHexEndView {
    PlayerHexRemovalLease removal;
    Registry registry;
    combat::conditions::State conditions;
    TemporaryHealthSnapshot health;
    std::optional<TemporaryHealth> temporary;
    std::vector<ConditionTransition> transitions;
    std::vector<FragilityReaction> fragility;
    static constexpr bool world_end_required=true;
};
inline std::optional<PlayerHexEndView> prepare_player_hex_end(
        const Registry& registry,const PlayerHexRemovalLease& removal,Time now,
        const combat::conditions::State& conditions,TemporaryHealthSnapshot health,
        std::optional<TemporaryHealth> temporary={}){
    const auto& target=removal.target;const auto& old=removal.entry;
    const auto lease=capture_player_hex_removal(registry,target,old.instance,now);
    if(!lease||lease->sequence!=removal.sequence||lease->entry.cause!=old.cause||
       lease->entry.skill!=old.skill||lease->entry.rank!=old.rank||lease->entry.kind!=old.kind||
       lease->entry.applied!=old.applied||lease->entry.expires!=old.expires||
       health.owner!=target||health.at!=now||!health.capacity.base||
       health.capacity.base>1000000||!std::isfinite(health.current)||health.current<0||
       health.current>health.capacity.maximum()||!std::isfinite(health.capacity.debt)||
       health.capacity.debt<0||(!health.current&&health.capacity.debt)||
       health.capacity.active!=conditions.has(combat::conditions::Kind::deep_wound,now)||
       health.capacity.reduction!=(health.capacity.active?std::min(100u,health.capacity.base/5):0u))return {};
    PlayerHexEndView result{removal,registry,conditions,health,std::move(temporary),{},{}};
    if(!result.registry.remove(target,old.instance))return {};
    if(health.current>0&&(old.skill==44||old.skill==2054)){
        const auto wound=wound_spell(old.skill,old.rank);
        const auto hex=degeneration_hex(old.skill,old.rank);
        if(!wound&&(!hex||!hex->end_condition))return {};
        const auto kind=wound?combat::conditions::Kind::deep_wound:*hex->end_condition;
        const auto duration=wound?wound->wound_duration:hex->end_duration;
        if(!result.conditions.apply(kind,old.cause,now,duration))return {};
    }
    const bool wounded=result.conditions.has(combat::conditions::Kind::deep_wound,now);
    if(result.temporary){
        const auto next=prepare_temporary_health_condition(*result.temporary,result.health,wounded);
        if(!next)return {};
        result.temporary=next->ledger;result.health=next->health;
    }else result.health.capacity.capacity(result.health.current,result.health.capacity.base,wounded);
    result.transitions=immediate_condition_transitions(conditions,result.conditions,now);
    for(const auto& transition:result.transitions)
        if(const auto reaction=fragility_reaction(result.registry,target,transition))result.fragility.push_back(*reaction);
    return result;
}
}
