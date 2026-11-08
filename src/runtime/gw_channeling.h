#pragma once
#include "gw_effect_registry.h"
#include "gw_aura_restoration.h"
#include <cmath>
#include <set>
#include <span>
namespace fw::gw::skills::effects {
struct ChannelingPlan {
    unsigned rank=0;Time duration=0;float radius=322.f;unsigned per_foe=1;
    auto operator<=>(const ChannelingPlan&)const=default;
};
inline std::optional<ChannelingPlan> channeling(unsigned rank=0){
    if(rank>20)return {};
    const auto* row=find_skill(38);if(!row||row->type!=6||row->attribute!=3)return {};
    return ChannelingPlan{rank,static_cast<Time>(std::lround(row->duration0+(double(row->duration15)-row->duration0)*rank/15.))*1000};
}
struct ChannelingFoe {Scope actor;double x=0,y=0;unsigned plane=0;bool alive=false,hostile=false;};
// The world supplies its authoritative actor population at successful spell
// completion. Player and NPC entries share one instance/actor ID namespace.
inline std::optional<unsigned> channeling_energy(const ChannelingPlan& plan,
        Scope caster,double x,double y,unsigned plane,std::span<const ChannelingFoe> population){
    const auto expected=channeling(plan.rank);
    if(!expected||*expected!=plan||!caster.instance||!caster.actor||!caster.generation||!std::isfinite(x)||!std::isfinite(y))return {};
    std::set<std::uint32_t> counted;
    for(const auto& foe:population){
        if(!foe.alive||!foe.hostile||foe.actor.instance!=caster.instance||!foe.actor.actor||!foe.actor.generation||
           foe.actor.actor==caster.actor||foe.plane!=plane||!std::isfinite(foe.x)||!std::isfinite(foe.y))continue;
        if(std::hypot(foe.x-x,foe.y-y)<=plan.radius)counted.insert(foe.actor.actor);
    }
    return static_cast<unsigned>(counted.size())*plan.per_foe;
}
// Prepared from an authoritative pre-effect snapshot, then consumed only after
// the spell result is known. This stores attribution and replay protection;
// it neither mutates Energy nor chooses self-cast/renewal ordering policy.
class ChannelingCompletion {
    struct Pending {std::uint64_t event;Time at;unsigned energy;};
    Scope owner_;std::uint64_t last_event_=0;Time clock_=0;
    std::optional<Pending> pending_;
public:
    explicit ChannelingCompletion(Scope owner):owner_(owner){}
    bool prepare(Scope caller,std::uint64_t event,unsigned skill,Time at,
            const std::optional<Entry>& observer,double x,double y,unsigned plane,
            std::span<const ChannelingFoe> population){
        const auto* row=find_skill(skill);
        if(caller!=owner_||!owner_.instance||!owner_.actor||!owner_.generation||!event||event<=last_event_||
           at<clock_||pending_||!row||!skill||!is_spell(*row))return false;
        unsigned energy=0;
        if(observer){
            if(observer->skill!=38||observer->kind!=Category::enchantment||!observer->instance||observer->cause!=owner_.actor||
               observer->applied>at||!observer->expires||at>=*observer->expires)return false;
            const auto plan=channeling(observer->rank);if(!plan)return false;
            const auto amount=channeling_energy(*plan,owner_,x,y,plane,population);if(!amount)return false;energy=*amount;
        }
        pending_=Pending{event,at,energy};last_event_=event;clock_=at;return true;
    }
    std::optional<unsigned> complete(Scope caller,std::uint64_t event,bool succeeded,Time at){
        if(caller!=owner_||!pending_||event!=pending_->event||at<pending_->at)return {};
        const auto result=succeeded?pending_->energy:0;pending_.reset();clock_=at;return result;
    }
};
}
