#pragma once
#include "gw_pet_catalog.h"
#include "gw_effect_registry.h"
#include "gw_armor_bounds.h"
#include <algorithm>
#include <cmath>

namespace fw::gw::pets::combat_model {
using Scope=skills::effects::Scope;
inline bool scope_valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
// A current World ownership check supplies this value at each transaction.
// This is an exact-lifetime policy input, never a client ownership credential.
struct Owned {
    Scope pet,owner;
    bool pve=false;
    bool valid()const{return scope_valid(pet)&&scope_valid(owner)&&pet!=owner&&pet.instance==owner.instance;}
};
enum class Loss {direct_damage,degeneration,health_loss,life_steal};
inline std::optional<double> incoming_multiplier(const Owned& owned,Scope recipient,Loss loss){
    if(!owned.valid()||recipient!=owned.pet)return {};
    switch(loss){
    case Loss::direct_damage:return owned.pve?.67:1.;
    case Loss::degeneration:case Loss::health_loss:case Loss::life_steal:return 1.;
    }
    return {};
}
struct Strike {
    Owned owned;
    Scope attacker;
    Pet profile;
    unsigned owner_level=0,beast_mastery=0;
    int armor=0;
    double roll=0;
    bool critical=false,critical_immune=false,weakness=false;
};
// Reviewed community model, not a recovered ArenaNet private formula.
// Animal_companion local wiki: unevolved level20/BM12 noncritical17..29,
// pet criticals retain a variable roll (suspected sqrt2), PvE outgoing+33%.
// Original experimenters: https://archive.guildwarslegacy.com/thread?id=89491
// Statistics of pets / testing note2: level scaling approximately level/20.
// Beast_Mastery and Damage_calculation: owner mastery scales like weapons;
// the diminishing-return threshold depends on OWNER level, not pet level.
// Explicit RNG samples and critical eligibility belong to World. Keep double
// precision until the shared protection/Health transaction performs rounding.
inline std::optional<double> ordinary(const Strike& s){
    if(!s.owned.valid()||s.attacker!=s.owned.pet||!pets::valid(s.profile)||
       s.profile.evolution!=Evolution::unevolved||!s.owner_level||s.owner_level>20||s.beast_mastery>20||
       !fw::gw::combat::valid_effective_armor(s.armor)||!std::isfinite(s.roll)||s.roll<0||s.roll>=1)return {};
    const auto threshold=(s.owner_level+4)/2;
    const auto strike=5*std::min(s.beast_mastery,threshold)+2*(s.beast_mastery>threshold?s.beast_mastery-threshold:0);
    const double base=(17.+12.*s.roll)*(double(s.profile.level)/20.);
    const double damage=base*std::exp2((double(strike)-s.armor)/40.)*
        ((s.critical&&!s.critical_immune)?std::sqrt(2.):1.)*(s.weakness?.34:1.)*(s.owned.pve?1.33:1.);
    if(!std::isfinite(damage)||damage<0||damage>16000000)return {};
    return damage;
}
}
