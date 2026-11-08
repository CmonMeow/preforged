#pragma once
#include "gw_temporary_skill_slots.h"
namespace fw::gw::skills::effects {
// Compatibility names for the original Inspired/Revealed Hex integration.
// All temporary copies share one root-identity/lifetime authority.
using TemporaryHexSlot=TemporarySkillSlot;
using TemporaryHexCastGrant=TemporarySkillCastGrant;
using TemporaryHexRestoration=TemporarySkillRestoration;
using TemporaryHexSlots=TemporarySkillSlots;
}