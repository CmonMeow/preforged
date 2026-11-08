#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct DolyakPlan {unsigned rank=0,armor=10;Time duration=8000;float movement=.25f;auto operator<=>(const DolyakPlan&)const=default;};
inline std::optional<DolyakPlan> dolyak_signet(unsigned rank){
 const auto* row=find_skill(361);
 if(rank>20||!row||row->type!=7||row->attribute!=17||row->scale0!=10||row->scale15!=40||row->bonus0!=75||row->bonus15!=75||row->duration0!=8||row->duration15!=20)return {};
 return DolyakPlan{rank,interpolate(10,40,rank),1000ull*interpolate(8,20,rank),.25f};
}
inline std::optional<DolyakPlan> active_dolyak(const Registry& registry,Scope owner,Time now){
 const auto entry=registry.newest_skill(owner,361,now);
 if(!entry||entry->kind!=Category::other||entry->cause!=owner.actor||!entry->expires)return {};
 return dolyak_signet(entry->rank);
}
// Skill armor stacks up to25; a stronger single source retains its own value.
inline unsigned stacked_skill_armor(unsigned first,unsigned second){return std::max({first,second,std::min(25u,first+second)});}
}
