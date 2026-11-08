#pragma once
#include "gw_combat.h"
namespace fw::gw::combat::weapon {
// Local GWToolbox Utils/ToolboxUtils.h DamageType enum and ItemDrops.cpp
// 24b8 arg1 decoding. Chaos/dark/holy are not elemental conversions.
inline std::optional<Kind> element(std::optional<unsigned> raw){
    if(!raw)return {};
    switch(*raw){case 3:return Kind::cold;case 4:return Kind::lightning;
    case 5:return Kind::fire;case 11:return Kind::earth;default:return {};}
}
}
