#pragma once
#include "gw_mending.h"
#include "gw_retribution.h"
#include "gw_combat.h"
#include <span>
namespace fw::gw::skills::effects {
struct MendingWire {combat::Packet upkeep,effect;};
// Native62writes the caster's Buff entry;64writes the recipient's Effect.
// Both packets carry recipient first,caster second;neither has a duration.
// Publication/routing and matching upkeep-removal packets belong to World.
inline std::optional<MendingWire> mending_wire(Scope caster,Scope target,const Entry& entry){
 const bool reviewed=entry.skill==290?bool(mending(entry.rank)):entry.skill==248?bool(retribution(entry.rank)):entry.skill==308&&succor()&&!entry.rank&&caster.actor!=target.actor;
 if(!caster.instance||!target.instance||caster.instance!=target.instance||!caster.generation||!target.generation||!caster.actor||caster.actor>65535||!target.actor||target.actor>65535||entry.cause!=caster.actor||!reviewed||entry.kind!=Category::enchantment||entry.expires||!entry.instance)return {};
 const auto packet=[&](unsigned opcode){combat::Packet result{static_cast<std::uint8_t>(opcode),0};combat::detail::put(result,target.actor);combat::detail::put(result,caster.actor);result.push_back(static_cast<std::uint8_t>(entry.skill));result.push_back(static_cast<std::uint8_t>(entry.skill>>8));combat::detail::put(result,entry.rank);combat::detail::put(result,entry.instance);return result;};
 return MendingWire{packet(62),packet(64)};
}
inline std::optional<MendingWire> mending_end_wire(Scope caster,Scope target,const Entry& entry){
 if(!mending_wire(caster,target,entry))return {};
 const auto packet=[&](unsigned opcode,unsigned actor){combat::Packet result{static_cast<std::uint8_t>(opcode),0};combat::detail::put(result,actor);combat::detail::put(result,entry.instance);return result;};
 return MendingWire{packet(63,caster.actor),packet(67,target.actor)};
}
// Session actor is authoritative. The request supplies only a buff ID;World
// must resolve it within that actor's maintained records,never a global ID.
inline std::optional<unsigned> decode_drop_maintained(std::span<const std::uint8_t> packet){
 if(packet.size()!=6||packet[0]!=39||(packet[1]!=0&&packet[1]!=128))return {};
 unsigned effect=0;for(unsigned n=0;n<4;++n)effect|=unsigned(packet[n+2])<<(8*n);
 return effect?std::optional(effect):std::nullopt;
}

}
