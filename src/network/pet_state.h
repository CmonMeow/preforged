#pragma once
#include "skill_state.h"
#include "gw_pet_profile.h"
namespace gwserver {
using PetsResult=ProfileResult<fw::gw::pets::Profile>;
// Trusted server adoption request. No retail packet can supply this profile.
// Later growth/release/rename require their own reviewed mutation contracts.
struct AdoptPet {
 OwnedCharacter owner;std::uint64_t expected_revision;
 fw::gw::pets::Profile expected,profile;
};
}
