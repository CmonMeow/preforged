#pragma once
#include "gw_attack_outcomes.h"
#include "gw_skill_effect_plan.h"
#include <span>

namespace fw::gw::skills::effects {
struct TigerStancePlan {unsigned rank=0;Time duration=0;};
inline std::optional<TigerStancePlan> tiger_stance(unsigned rank){
    const auto* row=find_skill(995);
    if(rank>20||!row||row->type!=3||row->attribute!=17||row->energy_cost()!=5||
       row->activation!=0||row->aftercast!=0||row->recharge!=20||
       row->duration0!=4||row->duration15!=10||row->scale0!=33||row->scale15!=33||
       row->caster_body_resource!=1271)return {};
    return TigerStancePlan{rank,1000ull*interpolate(4,10,rank)};
}
inline bool tiger_stance_active(std::span<const Entry> entries,Time now){
    for(const auto& e:entries)
        if(e.skill==995&&e.kind==Category::stance&&e.applied<=now&&
           e.expires&&now<*e.expires&&tiger_stance(e.rank))return true;
    return false;
}
inline std::optional<Entry> tiger_stance_failed_hit(const Registry& registry,Scope owner,
        const combat::weapon::ResolvedAttack& attack){
    if(attack.attacker!=owner||!attack.failed_hit())return {};
    const auto e=registry.newest_skill(owner,995,attack.at);
    if(!e||e->kind!=Category::stance||e->cause!=owner.actor||!tiger_stance(e->rank))return {};
    return e;
}
}
