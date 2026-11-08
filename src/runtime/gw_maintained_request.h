#pragma once
#include "gw_mending_wire.h"
namespace fw::gw::skills::effects {
enum class DropMaintainedResult {invalid_frame,session_not_ready,not_owned,removed};
inline const char* drop_maintained_reason(DropMaintainedResult result){
 switch(result){case DropMaintainedResult::invalid_frame:return "invalid_frame";case DropMaintainedResult::session_not_ready:return "session_not_ready";case DropMaintainedResult::not_owned:return "not_owned_or_stale";case DropMaintainedResult::removed:return "removed";}return "invalid_result";
}
// Opcode39 supplies a buff ID only. The session's actor scope and eligibility
// are supplied by the host;no caster/recipient can be selected by this packet.
// Complete framed requests only;stream fragmentation belongs to outgoing_frame.
template<class World> DropMaintainedResult dispatch_drop_maintained(World& world,
 std::optional<Scope> actor,bool ready,std::span<const std::uint8_t> frame,Time now){
 const auto effect=decode_drop_maintained(frame);if(!effect)return DropMaintainedResult::invalid_frame;
 if(!ready||!actor)return DropMaintainedResult::session_not_ready;
 return world.cancel_maintained_enchantment(*actor,*effect,now)?DropMaintainedResult::removed:DropMaintainedResult::not_owned;
}
}
