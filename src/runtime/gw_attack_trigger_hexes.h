#pragma once
#include "gw_effect_registry.h"
#include "gw_combat.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct AttackHexPlan {
    unsigned skill=0,rank=0,damage=0;
    Time duration=4000,blind_duration=0;
    float range=1248.f,radius=166.f;
    bool interrupt=false;
    bool consume=true;unsigned energy_loss=0;
    auto operator<=>(const AttackHexPlan&)const=default;
};
inline std::optional<AttackHexPlan> attack_hex(unsigned skill,unsigned rank=0){
    if(rank>20||(skill!=43&&skill!=47&&skill!=66))return {};
    const auto* row=find_skill(skill);if(!row||row->type!=4||row->attribute!=(skill==66?3u:1u))return {};
    if(skill==66)return AttackHexPlan{66,rank,0,static_cast<Time>(std::lround(row->duration0+(double(row->duration15)-row->duration0)*rank/15.))*1000,0,1248.f,0.f,false,false,5};
    return AttackHexPlan{skill,rank,
        static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.)),
        4000,skill==47?10000u:0u,1248.f,166.f,skill==43};
}
struct AttackHexReaction {Scope target;Entry cause;AttackHexPlan plan;Time at=0;};
// The authoritative attack-start path supplies the event. Consume one-shot
// effects before reactive damage/control; persistent effects retain their
// identity for later attacks. Movement and failed admission are not events.
inline std::optional<AttackHexReaction> consume_attack_hex(Registry& registry,
        Scope target,unsigned skill,Time at){
    const auto entry=registry.newest_skill(target,skill,at);
    if(!entry||entry->kind!=Category::hex)return {};
    const auto plan=attack_hex(skill,entry->rank);if(!plan)return {};
    if(plan->consume&&!registry.remove(target,entry->instance))return {};
    return AttackHexReaction{target,*entry,*plan,at};
}
inline std::optional<combat::Hit> attack_hex_damage(const AttackHexReaction& reaction,
        combat::Actor actor,unsigned level){
    const auto plan=attack_hex(reaction.plan.skill,reaction.plan.rank);
    if(!plan||*plan!=reaction.plan||actor.id!=reaction.target.actor||actor.generation!=reaction.target.generation||
       !reaction.target.instance||!reaction.target.generation||!reaction.cause.instance||!reaction.cause.cause||
       reaction.cause.skill!=reaction.plan.skill||reaction.cause.rank!=reaction.plan.rank||reaction.cause.kind!=Category::hex||
       reaction.cause.applied>reaction.at||!reaction.cause.expires||reaction.at>=*reaction.cause.expires)return {};
    const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,double(plan->damage),level,plan->skill},true);
    return hit.reason==combat::Reason::accepted?std::optional(hit):std::nullopt;
}
}
