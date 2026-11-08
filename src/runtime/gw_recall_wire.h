#pragma once
#include "gw_recall.h"
#include "gw_combat.h"
namespace fw::gw::skills::effects {
struct RecallWire {combat::Packet upkeep,effect;};
inline std::optional<RecallWire> recall_wire(const RecallAnchor& anchor,const Entry& entry){
 if(!recall_shadow()||!recall_scope(anchor.caster)||!recall_scope(anchor.target)||
    anchor.caster.instance!=anchor.target.instance||anchor.caster.actor==anchor.target.actor||
    !anchor.effect||entry.instance!=anchor.effect||entry.cause!=anchor.caster.actor||entry.skill!=925||
    entry.rank||entry.kind!=Category::enchantment||entry.expires||entry.applied!=anchor.applied)return {};
 const auto encode=[&](unsigned opcode){combat::Packet p{static_cast<std::uint8_t>(opcode),0};
  combat::detail::put(p,anchor.caster.actor);combat::detail::put(p,anchor.caster.actor);
  p.push_back(925&255);p.push_back(925>>8);combat::detail::put(p,0);combat::detail::put(p,anchor.effect);return p;};
 return RecallWire{encode(62),encode(64)};
}
inline std::optional<RecallWire> recall_end_wire(const RecallAnchor& anchor,const Entry& entry){
 if(!recall_wire(anchor,entry))return {};
 const auto encode=[&](unsigned opcode){combat::Packet p{static_cast<std::uint8_t>(opcode),0};
  combat::detail::put(p,anchor.caster.actor);combat::detail::put(p,anchor.effect);return p;};
 return RecallWire{encode(63),encode(67)};
}
}
