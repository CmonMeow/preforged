#pragma once
#include "gw_cooldown_bar_authority.h"
#include "gw_combat.h"
namespace fw::gw::skills::effects {
// World retains the owner's return publication until its session has merged
// these exact disabled copies with the Runtime's ordinary recharge authority.
struct RecallFeedbackNotice {
 Scope owner;std::uint64_t event=0;Time at=0;Slots bar{};
 bool operator==(const RecallFeedbackNotice&)const=default;
};
struct RecallFeedbackSlot {
 Slot slot;Time ordinary_until=0,cached_until=0,disabled_until=0;
};
struct RecallFeedbackReply {
 RecallFeedbackNotice notice;Time at=0;Slots current{};
 // Session-attested presentation inputs,not authority to modify World timers.
 std::vector<RecallFeedbackSlot> slots;std::vector<combat::Packet> packets;
};
inline combat::Packet recall_feedback_packet(unsigned actor,Slot slot,unsigned seconds){
 combat::Packet result{static_cast<std::uint8_t>(seconds?228:229),0};combat::detail::put(result,actor);
 result.push_back(static_cast<std::uint8_t>(slot.skill));result.push_back(static_cast<std::uint8_t>(slot.skill>>8));
 combat::detail::put(result,slot.copy);if(seconds)combat::detail::put(result,seconds);return result;
}
}
