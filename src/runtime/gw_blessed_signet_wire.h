#pragma once
#include "gw_combat.h"
#include <optional>
namespace fw::gw::skills::effects {
// Pinned native targeted consumers:162/kind52 updates normalized Energy;
// 159/kind54 dispatches an integer gain event. World must supply actual gain
// after debt/ceiling settlement,not the requested skill gain. Fractional popup
// rounding is unverified,so only exact whole gains publish the integer event.
struct BlessedSignetWire {combat::Packet delta;std::optional<combat::Packet> popup;};
inline std::optional<BlessedSignetWire> blessed_signet_wire(unsigned actor,std::uint64_t gained_units,unsigned nominal_max,std::uint64_t units_per_energy){
 if(!actor||actor>65535||!nominal_max||!units_per_energy||!gained_units||gained_units/units_per_energy>UINT32_MAX)return {};
 const float normalized=static_cast<float>(static_cast<double>(gained_units)/static_cast<double>(units_per_energy)/nominal_max);
 if(!std::isfinite(normalized)||normalized<=0)return {};
 const auto packet=[&](unsigned opcode,unsigned kind,unsigned value){combat::Packet result{static_cast<std::uint8_t>(opcode),0};combat::detail::put(result,kind);combat::detail::put(result,actor);combat::detail::put(result,actor);combat::detail::put(result,value);return result;};
 BlessedSignetWire result{packet(162,52,std::bit_cast<std::uint32_t>(normalized)),{}};
 if(gained_units%units_per_energy==0)result.popup=packet(159,54,static_cast<unsigned>(gained_units/units_per_energy));
 return result;
}
}
