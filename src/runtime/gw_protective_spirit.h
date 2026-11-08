#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <limits>
namespace fw::gw::skills::effects {
struct ProtectiveSpiritPlan {
    unsigned rank=0,maximum_health_percent=10;
    Time duration=5000,activation=250,aftercast=750,recharge=5000;
    unsigned energy=10;
    auto operator<=>(const ProtectiveSpiritPlan&)const=default;
};
inline std::optional<ProtectiveSpiritPlan> protective_spirit(unsigned rank){
    const auto* row=find_skill(245);
    if(rank>20||!row||row->profession!=3||row->attribute!=15||row->type!=6||row->special||
       row->energy_cost()!=10||row->health_sacrifice||row->activation!=.25f||row->aftercast!=.75f||row->recharge!=5||
       row->scale0!=10||row->scale15!=10||row->duration0!=5||row->duration15!=23||
       row->caster_body_resource!=2077||row->target_body_resource!=447)return {};
    ProtectiveSpiritPlan plan;plan.rank=rank;plan.duration=Time(interpolate(5,23,rank))*1000;return plan;
}
inline std::optional<double> protective_spirit_cap(const Registry& registry,Scope target,Time now,unsigned maximum){
    const auto entry=registry.newest_skill(target,245,now);if(!entry)return {};
    const auto plan=protective_spirit(entry->rank);
    if(!plan||!maximum||entry->kind!=Category::enchantment)return std::numeric_limits<double>::quiet_NaN();
    return double(maximum)*plan->maximum_health_percent/100.;
}
}
