#pragma once
#include "gw_shield_deflection.h"
#include "gw_dolyak_signet.h"
namespace fw::gw::skills::effects {
struct ShieldRegenerationPlan {
 unsigned rank=0,armor=40,pips=3;
 Time duration=5000,activation=250,aftercast=750,recharge=8000;
 unsigned energy=15;
 auto operator<=>(const ShieldRegenerationPlan&)const=default;
};
inline std::optional<ShieldRegenerationPlan> shield_regeneration(unsigned rank){
 const auto* row=find_skill(261);
 if(rank>20||!row||row->profession!=3||row->attribute!=15||row->type!=6||row->special!=4||row->energy_cost()!=15||row->health_sacrifice||
    row->activation!=.25f||row->aftercast!=.75f||row->recharge!=8||row->scale0!=3||row->scale15!=10||row->bonus0!=40||row->bonus15!=40||
    row->duration0!=5||row->duration15!=13||row->caster_body_resource!=2077||row->target_body_resource!=474)return {};
 ShieldRegenerationPlan plan;plan.rank=rank;plan.pips=interpolate(3,10,rank);plan.duration=Time(interpolate(5,13,rank))*1000;return plan;
}
inline unsigned protection_enchantment_armor(const Registry& registry,Scope target,Time now){
 unsigned regeneration=0;
 if(const auto entry=registry.newest_skill(target,261,now);entry&&entry->kind==Category::enchantment)
  if(const auto plan=shield_regeneration(entry->rank))regeneration=plan->armor;
 return stacked_skill_armor(shield_deflection_armor(registry,target,now),regeneration);
}
}
