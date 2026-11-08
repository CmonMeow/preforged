#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_combat.h"
namespace fw::gw::skills::effects {
// Local Soul_Barbs revision2642394 + pinned2022 row100.
struct SoulBarbsPlan {
    unsigned rank=0,damage=0;Time duration=30000;float range=1248;
    auto operator<=>(const SoulBarbsPlan&)const=default;
};
inline std::optional<SoulBarbsPlan> soul_barbs(unsigned rank){
    const auto* row=find_skill(100);
    if(rank>20||!row||row->type!=4||row->attribute!=7)return {};
    return SoulBarbsPlan{rank,interpolate(row->scale0,row->scale15,rank)};
}
inline bool valid_soul_barbs(const SoulBarbsPlan& plan){const auto e=soul_barbs(plan.rank);return e&&*e==plan;}
struct SoulBarbsReaction {Scope target;Entry cause;SoulBarbsPlan plan;};
enum class BarbsTrigger {spell_applied,enchantment_renewed};
// Capture BEFORE committing the triggering spell. Thus the first Soul Barbs
// does not trigger itself; replacing an existing Soul Barbs triggers the old
// copy once. Reapplying an enchantment (without a cast) also qualifies. The
// dispatcher calls once per successfully applied spell per recipient, never
// for cast start, failed casts, packet replay, effect expiry or removal.
inline std::optional<SoulBarbsReaction> soul_barbs_reaction(const Registry& before,
        Scope target,unsigned applied_skill,BarbsTrigger trigger,bool succeeded,Time now){
    const auto* row=find_skill(applied_skill);
    if(!succeeded||!row||!applied_skill)return {};
    if(trigger==BarbsTrigger::enchantment_renewed){if(row->type!=6)return {};}
    else if(trigger==BarbsTrigger::spell_applied){if(row->type!=4&&row->type!=6)return {};}
    else return {};
    const auto entry=before.newest_skill(target,100,now);
    if(!entry||entry->kind!=Category::hex)return {};
    const auto plan=soul_barbs(entry->rank);if(!plan)return {};
    return SoulBarbsReaction{target,*entry,*plan};
}
inline std::optional<Applied> apply_soul_barbs(Registry& registry,Scope target,
        unsigned effect,unsigned caster,const SoulBarbsPlan& plan,Time now){
    if(!valid_soul_barbs(plan)||now>UINT64_MAX-plan.duration)return {};
    if(!registry.observe_applications(target,100))return {};
    const auto result=registry.apply(target,{effect,caster,100,plan.rank,Category::hex,now,now+plan.duration},Replace::same_skill);
    return result.accepted?std::optional(result):std::nullopt;
}
inline std::optional<combat::Hit> soul_barbs_damage(const SoulBarbsReaction& reaction,
        combat::Actor actor,unsigned source_level){
    if(!valid_soul_barbs(reaction.plan)||reaction.cause.skill!=100||reaction.cause.kind!=Category::hex||
       reaction.cause.rank!=reaction.plan.rank||!reaction.cause.cause||
       actor.id!=reaction.target.actor||actor.generation!=reaction.target.generation)return {};
    const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,double(reaction.plan.damage),source_level,100},true);
    return hit.reason==combat::Reason::accepted?std::optional(hit):std::nullopt;
}
}
