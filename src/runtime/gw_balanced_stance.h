#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct BalancedPlan {unsigned rank=0;Time duration=8000;auto operator<=>(const BalancedPlan&)const=default;};
inline std::optional<BalancedPlan> balanced_stance(unsigned rank){
 const auto* row=find_skill(371);
 if(rank>20||!row||row->type!=3||row->attribute!=21||row->duration0!=8||row->duration15!=20||row->energy_cost()!=5)return {};
 return BalancedPlan{rank,1000ull*interpolate(8,20,rank)};
}
inline bool balanced_stance_active(const Registry& registry,Scope owner,Time now){
 const auto entry=registry.newest_skill(owner,371,now);
 return entry&&entry->kind==Category::stance&&entry->cause==owner.actor&&entry->expires&&balanced_stance(entry->rank).has_value();
}
}
