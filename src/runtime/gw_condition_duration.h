#pragma once
#include <cstdint>
#include <optional>
namespace fw::gw::combat::conditions {
// Archived Rune.html239..240: each reduction uses the original duration,
// rounds independently to seconds, then subtracts. Repeated identical runes
// do not stack; equipment decoding must collapse them before this boundary.
// This is not a raw item-modifier decoder or a skill-duration multiplier.
struct EquipmentReductions {
 bool rune=false,inscription=false;
};
inline std::optional<std::uint64_t> equipment_reduced_duration(
        std::uint64_t base_ms,EquipmentReductions equipment){
 if(!base_ms||base_ms>43200000||base_ms%1000)return {};
 const auto seconds=base_ms/1000;
 const auto each=(seconds*20+50)/100;
 const auto reduction=each*(unsigned(equipment.rune)+unsigned(equipment.inscription));
 // Zero is a valid computed result, not an active State timer. Callers must
 // preserve native zero-duration application/reaction semantics separately.
 return (seconds>reduction?seconds-reduction:0)*1000;
}
// Poisonous applies to any skill, with the weapon equipped at infliction.
// Duration.html84 rounds to whole seconds. Simultaneous extension/reduction
// ordering is deliberately not inferred by composing these two functions.
inline std::optional<std::uint64_t> prefix_extended_duration(std::uint64_t base_ms){
 if(!base_ms||base_ms>43200000||base_ms%1000)return {};
 const auto result=((base_ms/1000)*133+50)/100*1000;
 return result<=43200000?std::optional(result):std::nullopt;
}
inline std::optional<std::uint64_t> poisonous_duration(std::uint64_t base_ms){return prefix_extended_duration(base_ms);}
}
