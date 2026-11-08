#pragma once
#include "gw_pet_profile.h"
#include <array>
#include <cmath>

namespace fw::gw::pets {
// Exact captured identity, NOT model-based automatic charmability discovery.
// Four-profession archive epoch8/map162, world definition1769 and actor181
// observation1791. First observed coordinates are not proven spawn geometry.
struct StalkerAppearance {
    static constexpr unsigned map=162,observed_actor=181,observed_table=1345;
    static constexpr unsigned model_file=128406,skin_file=0,appearance=0,flags=13;
    static constexpr unsigned primary=2,level=5;
    static constexpr std::array<unsigned,1> model_files{129924};
    static constexpr std::array<std::uint16_t,4> name{3853,42826,48121,9370};
    static constexpr std::array<std::uint8_t,4> visual{0,0,0,100};
    static constexpr float first_x=-15185.521484f,first_y=-2665.847168f;
    static constexpr unsigned first_plane=0;
};
// Species authority, not appearance/name discovery. Reviewed local-wiki inference:
// Melandru's_Stalker is a Charmable animal; Poison_Ground poisons pets;
// Fleshy says non-fleshy creatures are immune to Poison. No direct capture claim.
// Corpse exploitability remains a separate owned-pet lifecycle rule.
struct SpeciesTraits {bool fleshy;const char* evidence;};
inline std::optional<SpeciesTraits> reviewed_species_traits(Species species){
    if(species!=Species::melandrus_stalker)return {};
    return SpeciesTraits{true,"Stalker species wiki inference: Poison_Ground pets + Fleshy immunity; not capture"};
}
struct BaseStats {unsigned maximum_health,armor;bool operator==(const BaseStats&)const=default;};
// Animal_companion.html441: base Health20*level+80, armor3*level+20.
// Captured exact level5 pet181 independently publishes max Health180. Evolved
// Health changes are deliberately unresolved here; absence must reject spawn.
inline std::optional<BaseStats> base_stats(const Pet& pet){
    if(!valid(pet)||pet.evolution!=Evolution::unevolved)return {};
    return BaseStats{20*pet.level+80,3*pet.level+20};
}
struct Comfort {unsigned rank=0,healing=0,resurrect_percent=0;bool operator==(const Comfort&)const=default;};
inline std::optional<Comfort> comfort(unsigned rank){
    if(rank>20)return {};
    // Pinned436 endpoints20..104 and10..58; local rank12 wiki87/48.
    return Comfort{rank,20+(84*rank+7)/15,10+(48*rank+7)/15};
}
inline bool charm_level_allowed(unsigned player,unsigned animal){
    return player>=1&&player<=20&&animal>=1&&animal<=20&&animal<=player+4;
}
}
