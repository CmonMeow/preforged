#pragma once
#include <cmath>
#include <optional>
#include <cstdint>
namespace fw::gw::skills::effects {
// Crystal Wave217: mirrored2021 wiki and pinned37130 record agree.
// All adjacent living foes lose all conditions; damage ignores armor.
struct CrystalWavePlan {unsigned skill=217,rank=0;float radius=166.f;unsigned base_damage=10,per_condition_damage=5;};
inline std::optional<CrystalWavePlan> crystal_wave(unsigned rank) {
    if(rank>20)return std::nullopt;
    return CrystalWavePlan{217,rank,166.f,10+4*rank,static_cast<unsigned>(std::lround(5.+10.*rank/15.))};
}
inline std::optional<unsigned> crystal_wave_damage(const CrystalWavePlan& plan,unsigned removed_conditions){
    if(plan.skill!=217||plan.rank>20||plan.radius!=166.f||removed_conditions>128)return std::nullopt;
    const auto reviewed=crystal_wave(plan.rank);
    if(plan.base_damage!=reviewed->base_damage||plan.per_condition_damage!=reviewed->per_condition_damage)return std::nullopt;
    return plan.base_damage+removed_conditions*plan.per_condition_damage;
}
}
