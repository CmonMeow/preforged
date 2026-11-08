#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <cmath>
#include <compare>
#include <optional>
#include <vector>

namespace fw::gw::skills::effects {

struct HealingSeedPlan {
    unsigned rank=0,healing=3;
    Time duration=10000,activation=2000,aftercast=750,recharge=25000;
    float range=1248.f,adjacent=166.f;
    auto operator<=>(const HealingSeedPlan&)const=default;
};

inline std::optional<HealingSeedPlan> healing_seed(unsigned rank){
    const auto* row=find_skill(274);
    if(rank>20||!row||row->campaign!=0||row->profession!=3||row->attribute!=13||
       row->type!=6||row->special||row->energy_cost()!=10||row->health_sacrifice||
       row->adrenaline||row->activation!=2.f||row->aftercast!=.75f||
       row->recharge!=25||row->target!=4||row->scale0!=3||row->scale15!=30||
       row->bonus0||row->bonus15||row->duration0!=10||row->duration15!=10||
       row->caster_body_resource!=2077||row->target_body_resource!=489)return {};
    return HealingSeedPlan{rank,interpolate(3,30,rank)};
}

struct HealingSeedRecipient {
    Scope owner;
    bool allied=false,alive=false,same_plane=false;
    float distance_from_damaged=0;
};
struct HealingSeedTarget {
    Scope owner;
    bool allied=false,alive=false,same_plane=false;
    float distance_from_caster=0;
};

// Cast admission uses a World-reviewed ally relation and measured distance.
// The client-supplied target ID alone cannot establish either property.
inline bool healing_seed_target(const HealingSeedPlan& plan,Scope caster,
        const HealingSeedTarget& target){
    return healing_seed(plan.rank)==plan&&caster.instance&&caster.actor&&caster.generation&&
        target.owner.instance==caster.instance&&target.owner.actor&&target.owner.generation&&
        target.owner!=caster&&target.allied&&target.alive&&target.same_plane&&
        std::isfinite(target.distance_from_caster)&&
        target.distance_from_caster>=0&&target.distance_from_caster<=plan.range;
}

struct HealingSeedReaction {
    unsigned effect=0,cause=0,healing=0;
    std::vector<Scope> recipients;
};

// World supplies a positive direct-damage proposal and potential recipients.
// It publishes only after actual positive loss, rechecking recipient survival
// and scope. The source effect must be active at the impact tick. World owns
// replay protection, Health authority and the spatial snapshot.
inline std::optional<HealingSeedReaction> healing_seed_reaction(
        const Registry& registry,Scope damaged,Time impact,double committed_loss,
        const std::vector<HealingSeedRecipient>& candidates){
    if(!damaged.instance||!damaged.actor||!damaged.generation||
       !std::isfinite(committed_loss)||committed_loss<0||!registry.matches(damaged))return {};
    const auto entry=registry.newest_skill(damaged,274,impact);
    if(!entry||committed_loss==0)return HealingSeedReaction{};
    const auto plan=healing_seed(entry->rank);
    if(!plan||entry->kind!=Category::enchantment||!entry->instance||
       !entry->cause||!entry->expires||*entry->expires<=entry->applied)return {};
    HealingSeedReaction result{entry->instance,entry->cause,plan->healing,{}};
    bool damaged_present=false;
    for(const auto& person:candidates){
        if(person.owner.instance!=damaged.instance||!person.owner.actor||
           !person.owner.generation||!std::isfinite(person.distance_from_damaged)||
           person.distance_from_damaged<0)return {};
        if(person.owner==damaged)damaged_present=true;
        if(person.allied&&person.alive&&person.same_plane&&
           person.distance_from_damaged<=plan->adjacent){
            if(std::find(result.recipients.begin(),result.recipients.end(),person.owner)!=result.recipients.end())return {};
            result.recipients.push_back(person.owner);
        }
    }
    if(!damaged_present)return {};
    return result;
}
}
