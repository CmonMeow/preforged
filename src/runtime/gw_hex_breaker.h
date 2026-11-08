#pragma once
#include "gw_effect_registry.h"
#include <cmath>

namespace fw::gw::skills::effects {
struct HexBreakerPlan {
    unsigned rank=0,damage=10;
    Time duration=5000;
    auto operator<=>(const HexBreakerPlan&)const=default;
};
inline std::optional<HexBreakerPlan> hex_breaker(unsigned rank){
    const auto* row=find_skill(10);
    if(rank>20||!row||row->type!=3||row->attribute!=2||row->duration0!=5||row->duration15!=80||row->scale0!=10||row->scale15!=46)return {};
    return HexBreakerPlan{rank,static_cast<unsigned>(std::lround(10.+36.*rank/15.)),Time(5+5*rank)*1000};
}
struct HexBreakerReaction {Scope owner,caster;unsigned stance_instance=0,incoming_skill=0,damage=0;};
// Direct selected-recipient application only. World applies initial effects
// normally,then rejects this recipient's hex and must not register its end
// effects. Other area recipients and normal cast completion remain independent.
// Call on a proposed registry copy while staging retaliation damage;commit
// together only after validating both current actor lifetimes and damage state.
// Secondary-area triggering still needs evidence before extending this API.
inline std::optional<HexBreakerReaction> take_hex_breaker(Registry& registry,Scope owner,Scope caster,unsigned incoming,Time now){
    const auto valid=[](Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;};
    const auto* row=find_skill(incoming);
    if(!valid(owner)||!valid(caster)||owner.instance!=caster.instance||owner.actor==caster.actor||!incoming||!row||row->type!=4)return {};
    const auto entry=registry.newest_skill(owner,10,now);
    if(!entry||entry->kind!=Category::stance||entry->cause!=owner.actor||!entry->instance||
       entry->applied>now||!entry->expires||now>=*entry->expires)return {};
    const auto plan=hex_breaker(entry->rank);if(!plan)return {};
    if(!registry.remove(owner,entry->instance))return {};
    return HexBreakerReaction{owner,caster,entry->instance,incoming,plan->damage};
}
}
