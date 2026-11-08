#pragma once
namespace fw::gw::movement {
// Official opcode39 capture: actor25, map280, tick8359828, 00009043.
// See haste-speed-capture-evidence.json in the fixed20261015 checkpoint.
inline constexpr float player_run_speed=288.f;
// One100ms prediction lead; repeated steering starts must not replenish it.
inline constexpr double player_checkpoint_lead=double(player_run_speed)*.1;
}
