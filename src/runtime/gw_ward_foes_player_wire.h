#pragma once
#include "gw_agent_wire.h"
#include <array>

namespace fw::gw::movement {
// Detached publication adapter, not an installed World consumer. The multiplier
// is the FINAL composed movement value, not the raw Ward factor. Gait remains
// independent (including keyboard backward/diagonal/contact projection).
// Caller settles old motion through the boundary before emitting this pair,
// and renews its navigation-validated destination after the pair when moving.
inline std::array<agent::Packet,2> ward_foes_player_speed_2022(
    std::uint32_t actor,float effective_multiplier,float gait_fraction,
    std::uint8_t gait_direction){
    if(!std::isfinite(effective_multiplier)||effective_multiplier<=0.f)
        throw std::invalid_argument("invalid composed player movement multiplier");
    return {agent::movement_base_speed_2022(actor,player_run_speed*effective_multiplier),
        agent::movement_speed_2022(actor,gait_fraction,gait_direction)};
}
}
