#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
namespace fw::gw::skills::effects {
struct DeathCharge {unsigned rank=0,healing=65;float range=1248,compass=5000;auto operator<=>(const DeathCharge&)const=default;};
inline std::optional<DeathCharge> death_charge(unsigned rank){
 const auto* r=find_skill(952);
 if(rank>20||!r||r->profession!=7||r->attribute!=31||r->type!=5||r->special||r->energy_cost()!=5||
    r->activation!=.25f||r->aftercast!=.75f||r->recharge!=30||r->scale0!=65||r->scale15!=200||
    r->caster_body_resource!=951||r->target_body_resource!=2077)return {};
 return DeathCharge{rank,65+9*rank};
}
// Raw quote only; shared healing modifiers/caps and relocation own their commit.
// Current Health is compared here (not a hard-coded50percent threshold).
// Comparison moment/retail absolute-versus-percentage confirmation remain
// integration gates; the World/Runtime binding uses this candidate quote.
inline std::optional<unsigned> death_charge_healing(const DeathCharge& plan,combat::Health caster,combat::Health foe){
 const auto expected=death_charge(plan.rank);
 if(!expected||*expected!=plan||!combat::valid(caster)||!combat::valid(foe)||caster.current<=0||foe.current<=0)return {};
 return foe.current>caster.current?plan.healing:0u;
}
}
