#pragma once
#include "gw_energy_enchantment.h"
namespace fw::gw::skills::effects {
using BloodPowerPlan=EnergyEnchantmentPlan;
inline std::optional<BloodPowerPlan> blood_is_power(unsigned rank){return energy_enchantment(119,rank);}
}
