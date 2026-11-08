#pragma once
#include "gw_effect_registry.h"
#include "gw_conditions.h"
#include "gw_combat.h"
#include <cmath>

namespace fw::gw::skills::effects {
struct FragilityPlan {
    unsigned skill=0,rank=0,damage=0;
    Time duration=0;
    float range=1248.f,radius=0;
    auto operator<=>(const FragilityPlan&)const=default;
};
inline std::optional<FragilityPlan> fragility(unsigned skill,unsigned rank=0,bool pvp=false){
    if(skill==19&&pvp)skill=2998;
    if(rank>20||(skill!=19&&skill!=2998))return {};
    const auto* row=find_skill(skill);if(!row||row->type!=4||row->attribute!=1)return {};
    return FragilityPlan{skill,rank,
        static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.)),
        static_cast<Time>(std::lround(row->duration0+(double(row->duration15)-row->duration0)*rank/15.))*1000,
        1248.f,skill==19?166.f:0.f};
}
struct ConditionTransition {combat::conditions::Kind kind;bool gained;Time at;};
// Immediate commits compare active conditions, never icon refresh packets.
// Expiry is handled separately at its actual deadline, before a later commit.
inline std::vector<ConditionTransition> immediate_condition_transitions(
        const combat::conditions::State& before,const combat::conditions::State& after,Time now){
    std::vector<ConditionTransition> events;
    for(unsigned i=0;i<10;++i){const auto kind=static_cast<combat::conditions::Kind>(i);
        const bool old=before.has(kind,now),current=after.has(kind,now);
        if(old!=current)events.push_back({kind,current,now});
    }
    return events;
}
inline std::vector<ConditionTransition> expired_condition_transitions(
        const combat::conditions::State& before,Time from,Time until){
    std::vector<ConditionTransition> events;if(until<from)return events;
    for(unsigned i=0;i<10;++i){const auto kind=static_cast<combat::conditions::Kind>(i);
        const auto entry=before.lookup(kind);
        if(entry&&entry->expires>from&&entry->expires<=until)events.push_back({kind,false,entry->expires});
    }
    std::stable_sort(events.begin(),events.end(),[](const auto& a,const auto& b){return a.at<b.at;});
    return events;
}
struct FragilityReaction {Scope target;Entry cause;FragilityPlan plan;ConditionTransition transition;};
inline std::optional<FragilityReaction> fragility_reaction(const Registry& registry,
        Scope target,const ConditionTransition& transition){
    if(static_cast<unsigned>(transition.kind)>=10)return {};
    const auto entry=registry.newest_skill(target,19,transition.at,2998);
    if(!entry||entry->kind!=Category::hex)return {};
    const auto plan=fragility(entry->skill,entry->rank);if(!plan)return {};
    return FragilityReaction{target,*entry,*plan,transition};
}
inline std::optional<combat::Hit> fragility_damage(const FragilityReaction& reaction,
        combat::Actor actor,unsigned source_level){
    const auto expected=fragility(reaction.plan.skill,reaction.plan.rank);
    if(!expected||*expected!=reaction.plan||!reaction.target.instance||!reaction.target.actor||!reaction.target.generation||
       actor.id!=reaction.target.actor||actor.generation!=reaction.target.generation||
       !reaction.cause.instance||!reaction.cause.cause||reaction.cause.kind!=Category::hex||
       reaction.cause.skill!=reaction.plan.skill||reaction.cause.rank!=reaction.plan.rank||
       reaction.cause.applied>reaction.transition.at||!reaction.cause.expires||reaction.transition.at>=*reaction.cause.expires||
       static_cast<unsigned>(reaction.transition.kind)>=10)return {};
    const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,double(reaction.plan.damage),source_level,reaction.plan.skill},true);
    return hit.reason==combat::Reason::accepted?std::optional(hit):std::nullopt;
}
}
