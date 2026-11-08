#pragma once
#include "gw_npc_combat_catalog.h"
#include "gw_skill_particle_wire.h"
namespace fw::gw::skills::effects {
struct WatchfulOwnership {Scope caster,target;std::uint64_t applied=0;};
// World consumes ownership once; this copy-only renderer cannot attribute a
// popup to a replacement actor that happens to reuse the original caster ID.
inline std::optional<std::vector<combat::Packet>> watchful_npc_end_packets(
        const combat::npcs::EnchantmentEnd& end,Scope target,const WatchfulOwnership& owner,
        std::optional<Scope> current_caster){
    const auto allowed=watchful_early_gain(end.entry,end.at,end.after.current>0);
    if(!allowed||owner.target!=target||!target.instance||!target.actor||!target.generation||
       !owner.caster.actor||!owner.caster.generation||owner.caster.instance!=target.instance||
       owner.applied!=end.entry.applied||owner.caster.actor!=end.entry.cause||
       !std::isfinite(end.after.current)||!std::isfinite(end.after.maximum)||
       end.after.maximum<1||end.after.maximum>UINT32_MAX||std::floor(end.after.maximum)!=end.after.maximum||
       end.after.current<0||end.after.current>end.after.maximum||!std::isfinite(end.gained)||
       end.gained<0||end.gained>*allowed||end.gained>end.after.current)return {};
    std::vector<combat::Packet> result{combat::detail::fraction(target.actor,end.after)};
    if(end.gained>0&&current_caster==owner.caster)
        result.push_back(particles::health_change_2022(target.actor,owner.caster.actor,end.gained,end.after.maximum));
    combat::Packet removed{67,0};combat::detail::put(removed,target.actor);combat::detail::put(removed,end.entry.instance);
    result.push_back(std::move(removed));return result;
}
}
