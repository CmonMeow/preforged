#pragma once
#include "gw_combat.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::combat::ai::elemental::wire {
// Fixed checkpoint packets-1360-15821281: Fireball145869 caster21/332;
// Fireball145875..145884 and Flare157074..157075 targeted20/344;
// Immolate150210 targeted20/339. Modern159/160 map to pinned2022 158/159,
// matching the existing integer/targeted-body serializers. No invented flight
// opcode or inferred projectile synchronization key is emitted here.
inline std::optional<combat::Packet> release_body(unsigned caster,unsigned skill){
    const auto* row=skills::find_skill(skill);
    if(!caster||caster>65535||!row)return {};
    // Flame Burst's328 is the pinned native caster-body resource. Its archived
    // damage-ID observations do not establish NPC priorities or timing.
    const unsigned resource=skill==186?332:skill==183?328:0;
    if(!resource||row->caster_body_resource!=resource)return {};
    return combat::detail::integer(21,caster,resource);
}
inline std::optional<combat::Packet> impact_body(unsigned target,unsigned caster,unsigned skill){
    const auto* row=skills::find_skill(skill);
    if(!target||target>65535||!caster||caster>65535||!row)return {};
    unsigned resource=0;
    if((skill==186||skill==194)&&row->target_body_resource==2077)resource=344;
    else if(skill==191&&row->target_body_resource==339)resource=339;
    else return {};
    combat::Packet packet{159,0};combat::detail::put(packet,20);combat::detail::put(packet,target);
    combat::detail::put(packet,caster);combat::detail::put(packet,resource);return packet;
}
}
