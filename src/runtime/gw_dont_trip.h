#pragma once
#include "gw_effect_registry.h"
#include <cmath>
#include <algorithm>
namespace fw::gw::skills::effects {
struct DontTripPlan {
 unsigned rank=0,energy=5;Time duration=3000,recharge=20000;float range=1000;
 auto operator<=>(const DontTripPlan&)const=default;
};
inline std::optional<DontTripPlan> dont_trip(unsigned rank){
 const auto* row=find_skill(2216);
 if(rank>10||!row||row->campaign!=4||row->profession||row->attribute!=51||row->type!=15||
    row->energy_cost()!=5||row->activation!=0||row->aftercast!=0||row->recharge!=20||row->adrenaline||
    row->duration0!=3||row->duration15!=5||row->aoe_range!=1000||
    row->caster_body_resource!=1901||row->target_body_resource!=2077)return {};
 return DontTripPlan{rank,5,1000ull*std::min(5u,3u+rank/2)};
}
inline std::optional<Entry> prepare_dont_trip(const DontTripPlan& plan,Scope source,Scope target,
 unsigned effect,Time now,bool party_member,bool alive,float distance){
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(dont_trip(plan.rank)!=plan||!valid(source)||!valid(target)||source.instance!=target.instance||!effect||
    !alive||(!party_member&&source!=target)||!std::isfinite(distance)||distance<0||distance>plan.range||now>UINT64_MAX-plan.duration)return {};
 return Entry{effect,source.actor,2216,plan.rank,Category::other,now,now+plan.duration};
}
inline bool dont_trip_active(const Registry& registry,Scope owner,Time now){
 const auto entry=registry.newest_skill(owner,2216,now);if(!entry)return false;
 const auto plan=dont_trip(entry->rank);
 return plan&&entry->kind==Category::other&&entry->cause&&entry->cause<=65535&&entry->expires&&
   *entry->expires>=entry->applied&&*entry->expires-entry->applied==plan->duration;
}
}
