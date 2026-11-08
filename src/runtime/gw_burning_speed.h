#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_conditions.h"
#include <cmath>
#include <optional>
#include <set>
#include <span>
#include <vector>

namespace fw::gw::skills::effects {
struct BurningSpeedPlan {
 unsigned rank=0, energy=10, activation_ms=250, aftercast_ms=750, recharge_ms=0;
 unsigned duration_ms=7000, movement_bonus_percent=0, ending_burning_ms=0;
 bool self_burning=true;
 bool operator==(const BurningSpeedPlan&)const=default;
};

// Detached, pinned 2022 policy. Runtime still needs an enchantment end event,
// authoritative movement and condition publication before this can be bound.
inline std::optional<BurningSpeedPlan> burning_speed(unsigned rank) {
 const auto* r=find_skill(823);
 if(rank>20||!r||r->campaign!=2||r->profession!=6||r->attribute!=10||
    r->type!=6||r->energy_cost()!=10||r->adrenaline||
    r->activation!=0.25f||r->aftercast!=0.75f||r->recharge||
    r->scale0!=30||r->scale15!=45||r->bonus0!=3||r->bonus15!=9||
    r->duration0!=7||r->duration15!=7||r->target_body_resource!=339)
  return {};
 return BurningSpeedPlan{rank,10,250,750,0,7000,
  interpolate(30,45,rank),1000u*interpolate(3,9,rank),true};
}

struct BurningSpeedFoe {
 Scope scope;
 double x=0,y=0;
 bool eligible=false; // World verifies current life, hostility and layer.
 combat::conditions::State conditions;
};
struct BurningSpeedBurn {
 Scope scope;
 combat::conditions::State conditions;
};

// Read-only preflight for the end event. The caller must commit the proposed
// states only after rechecking these exact actor generations and positions.
// Adjacent is the 166-unit game range; this does not assume that all actors
// inside it are foes or that a removal has already been published.
inline std::optional<std::vector<BurningSpeedBurn>> burning_speed_end(
 const Entry& ending,Scope caster,Time reviewed_duration,double x,double y,
 std::span<const BurningSpeedFoe> candidates,Time now){
 const auto plan=burning_speed(ending.rank);
 if(!plan||ending.skill!=823||ending.kind!=Category::enchantment||
    !ending.instance||ending.cause!=caster.actor||!caster.instance||
    !caster.actor||caster.actor>65535||!caster.generation||
    reviewed_duration<plan->duration_ms||reviewed_duration>plan->duration_ms*2||
    ending.applied>UINT64_MAX-reviewed_duration||
    ending.expires!=ending.applied+reviewed_duration||now<ending.applied||
    !std::isfinite(x)||!std::isfinite(y)||candidates.size()>4096)return {};
 std::set<unsigned> seen;
 std::vector<BurningSpeedBurn> result;
 for(const auto& foe:candidates){
  if(foe.scope.instance!=caster.instance||!foe.scope.actor||
     foe.scope.actor>65535||!foe.scope.generation||
     !seen.insert(foe.scope.actor).second||!std::isfinite(foe.x)||
     !std::isfinite(foe.y))return {};
  if(!foe.eligible||foe.scope.actor==caster.actor||
     std::hypot(foe.x-x,foe.y-y)>166.)continue;
  auto proposed=foe.conditions;
  if(!proposed.apply(combat::conditions::Kind::burning,caster.actor,now,
                     plan->ending_burning_ms))return {};
  result.push_back({foe.scope,std::move(proposed)});
 }
 return result;
}
}
