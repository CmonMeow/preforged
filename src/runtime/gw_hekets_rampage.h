#pragma once
#include "gw_cooldown_bar_authority.h"
#include "gw_skill_effect_plan.h"

namespace fw::gw::skills::effects {
struct HeketsRampagePlan {
    unsigned rank=0;Time duration=5000;
    unsigned energy=5,recharge_ms=10000,attack_speed_percent=33,caster_body=765;
    bool operator==(const HeketsRampagePlan&)const=default;
};
inline std::optional<HeketsRampagePlan> hekets_rampage(unsigned rank){
    const auto* r=find_skill(1728);
    if(rank>20||!r||r->campaign!=3||r->profession!=2||r->attribute!=22||
       r->type!=3||r->special||r->energy_cost()!=5||r->health_sacrifice||r->adrenaline||
       r->activation!=0||r->aftercast!=0||r->recharge!=10||r->target||
       r->duration0!=5||r->duration15!=11||r->scale0!=33||r->scale15!=33||
       r->bonus0||r->bonus15||r->caster_body_resource!=765||r->target_body_resource!=2077)
        return {};
    return HeketsRampagePlan{rank,1000ull*interpolate(5,11,rank)};
}
inline std::optional<Entry> active_hekets_rampage(const Registry& registry,Scope owner,Time now){
    if(!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||!registry.matches(owner))return {};
    const auto e=registry.newest_skill(owner,1728,now);
    if(!e||e->cause!=owner.actor||e->kind!=Category::stance||!e->expires||
       !hekets_rampage(e->rank))return {};
    return e;
}
inline bool hekets_rampage_active(std::span<const Entry> entries,Time now){
    for(const auto& entry:entries)
        if(entry.skill==1728&&entry.kind==Category::stance&&entry.instance&&entry.cause&&
           entry.applied<=now&&entry.expires&&now<*entry.expires&&hekets_rampage(entry.rank))
            return true;
    return false;
}
struct HeketsRampageApplication {Registry registry;Entry applied;std::vector<Entry> removed;};
inline std::optional<HeketsRampageApplication> prepare_hekets_rampage(
        const Registry& current,Scope owner,unsigned effect,const HeketsRampagePlan& plan,Time now){
    if(hekets_rampage(plan.rank)!=plan||!owner.instance||!owner.actor||owner.actor>65535||
       !owner.generation||!current.matches(owner)||!effect||now>UINT64_MAX-plan.duration)return {};
    Registry next=current;
    const Entry entry{effect,owner.actor,1728,plan.rank,Category::stance,now,now+plan.duration};
    const auto result=next.apply(owner,entry,Replace::category);
    if(!result.accepted)return {};
    return HeketsRampageApplication{std::move(next),entry,result.removed};
}
struct HeketsRampageEnd {Registry registry;std::optional<Entry> ended;};
// Feed committed attack-skill completions, not request/start admission.
// Frozen Triple Shot1953 begins while1728 is active and its first release
// shares the tick of the stance removal. This preserves IAS during activation.
// World must validate the completion identity and atomically commit this copy
// with the attack release. Ordinary attacks never call this capability.
inline std::optional<HeketsRampageEnd> prepare_hekets_completed_skill(
        const Registry& current,Scope owner,Slot completed,
        const CooldownBarSnapshot& bar,Time now){
    if(!current.matches(owner)||!cooldown_slot(completed)||bar.published.owner!=owner||
       !bar.published.order||!bar.revision||bar.published.at>now||
       std::find(bar.bar.begin(),bar.bar.end(),completed)==bar.bar.end())return {};
    const auto* used=find_skill(completed.skill);if(!used)return {};
    Registry next=current;std::optional<Entry> ended;
    if(used->type==14)if(const auto active=active_hekets_rampage(current,owner,now)){
        ended=next.remove(owner,active->instance);if(!ended)return {};
    }
    return HeketsRampageEnd{std::move(next),ended};
}
}
