#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
struct ShieldDeflectionPlan {
 unsigned rank=0,armor=15,block_percent=75;
 Time duration=3000,activation=250,aftercast=750,recharge=10000;
 unsigned energy=10;
 auto operator<=>(const ShieldDeflectionPlan&)const=default;
};
inline std::optional<ShieldDeflectionPlan> shield_deflection(unsigned rank){
 const auto* row=find_skill(259);
 if(rank>20||!row||row->profession!=3||row->attribute!=15||row->type!=6||row->special!=4||row->energy_cost()!=10||row->health_sacrifice||
    row->activation!=.25f||row->aftercast!=.75f||row->recharge!=10||row->scale0!=75||row->scale15!=75||row->bonus0!=15||row->bonus15!=30||
    row->duration0!=3||row->duration15!=10||row->caster_body_resource!=2077||row->target_body_resource!=471)return {};
 ShieldDeflectionPlan plan;plan.rank=rank;plan.armor=interpolate(15,30,rank);plan.duration=Time(interpolate(3,10,rank))*1000;return plan;
}
inline unsigned shield_deflection_armor(const Registry& registry,Scope target,Time now){
 const auto entry=registry.newest_skill(target,259,now);if(!entry||entry->kind!=Category::enchantment)return 0;
 const auto plan=shield_deflection(entry->rank);return plan?plan->armor:0;
}
}
