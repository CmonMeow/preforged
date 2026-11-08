#pragma once
#include "gw_cooldown_bar_authority.h"
#include <cmath>
namespace fw::gw::combat::weapon {
struct BowSelfDisablePlan {skills::Slot trigger;unsigned rank=0,bonus=0;std::uint64_t duration=0;bool operator==(const BowSelfDisablePlan&)const=default;};
inline std::optional<BowSelfDisablePlan> bow_self_disable(unsigned skill,unsigned rank,unsigned copy=0){
 const auto* r=skills::find_skill(skill);if(rank>20||copy>65535||(skill!=908&&skill!=909)||!r)return {};
 const bool marauder=skill==908;
 if(r->profession!=2||r->type!=14||r->attribute!=25||r->special||r->health_sacrifice||r->adrenaline||
    r->activation!=0||r->aftercast!=0||r->energy_cost()!=(marauder?10u:5u)||r->recharge!=(marauder?6u:2u)||
    r->weapon_requirement!=2||r->scale0!=10||r->scale15!=(marauder?35u:25u)||
    r->bonus0!=(marauder?10u:5u)||r->bonus15!=(marauder?10u:3u)||
    r->caster_body_resource!=(marauder?1058u:1062u)||r->target_body_resource!=2077)return {};
 return BowSelfDisablePlan{{static_cast<std::uint16_t>(skill),copy},rank,
  static_cast<unsigned>(std::lround(10.+(marauder?25.:15.)*rank/15.)),
  1000ull*(marauder?10u:static_cast<unsigned>(std::lround(5.-2.*rank/15.)))};
}
inline std::optional<std::vector<skills::Slot>> bow_self_disabled_slots(const BowSelfDisablePlan& plan,const skills::Slots& bar){
 const auto p=bow_self_disable(plan.trigger.skill,plan.rank,plan.trigger.copy);if(!p||*p!=plan)return {};
 std::set<std::pair<unsigned,unsigned>> seen;std::vector<skills::Slot> result;
 for(const auto slot:bar){if(!slot.skill){if(slot.copy)return {};continue;}
  if(!skills::effects::cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};
  const auto* row=skills::find_skill(slot.skill);const bool attack=row->type==14||row->type==20;
  if(plan.trigger.skill==908?!attack:attack&&slot!=plan.trigger)result.push_back(slot);
 }return result;
}
}
