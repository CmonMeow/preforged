#pragma once
#include "gw_ward_foes_route.h"
#include "gw_movement_effects.h"
#include "gw_player_movement.h"

namespace fw::gw::skills::effects {
// Direction zero denotes click movement; 1..8 are World's held-input flags.
// World must supply only wards for which this player is an authoritative foe,
// and navigation-validated legs. Keyboard turns/slides must be split at the
// same midpoint/collision boundaries used by the held-input controller.
inline std::optional<WardFoeRouteTravel> ward_foes_player_route_travel(
 const WardFoeRecipient& player,std::span<const WardAgainstFoes> eligible_wards,
 std::span<const WardFoeRouteLeg> route,Time from,Time through,
 const MovementEffects& effects,unsigned direction=0){
 if(direction>8||through<from)return {};
 constexpr Time exact_time_limit=9007199254740991ULL;
 if(through>exact_time_limit)return {};
 const double input_rate=direction<=3?1.:direction<=6?.66:.75;
 std::vector<double> boundaries;
 for(const auto at:effects.boundaries(from,through))boundaries.push_back(double(at));
 return ward_foes_route_travel(player,eligible_wards,route,double(from),double(through),boundaries,
  [&](double at,float external)->std::optional<double>{
   // All effect boundaries are integral milliseconds, while spatial entry/exit
   // may occur between them. Flooring preserves the active half-open slice.
   return double(movement::player_run_speed)*input_rate*
       effects.at_with_external(static_cast<Time>(std::floor(at)),external);
  });
}
}
