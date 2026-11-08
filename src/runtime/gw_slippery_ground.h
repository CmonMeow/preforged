#pragma once
#include "gw_effect_registry.h"
#include <cmath>
#include <set>
#include <span>

namespace fw::gw::skills::effects {
struct SlipperyGroundPlan {
    unsigned rank=0,energy=5,activation_ms=1000,aftercast_ms=750,recharge_ms=10000;
    unsigned knockdown_ms=2000,failure_percent=50;
    float range=1248.f,radius=166.f;
    auto operator<=>(const SlipperyGroundPlan&)const=default;
};
inline std::optional<SlipperyGroundPlan> slippery_ground(unsigned rank){
    const auto* r=find_skill(2191);
    if(rank>20||!r||r->campaign!=4||r->profession!=6||r->attribute!=11||
       r->type!=5||r->special!=64||r->energy_cost()!=5||r->health_sacrifice||
       r->adrenaline||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=10||
       r->target!=5||r->pvp_skill!=3398||r->caster_body_resource!=2077||
       r->target_body_resource!=1824)return {};
    SlipperyGroundPlan p;p.rank=rank;p.failure_percent=rank<5?50:0;return p;
}
struct SlipperyGroundActor {
    Scope scope;
    double x=0,z=0;
    unsigned plane=0;
    bool alive=true,hostile=false;
    std::optional<bool> blind,moving;
};
struct SlipperyGroundProposal {
    bool failed=false;
    std::vector<Scope> knockdowns;
};
// Completion snapshots and a server-owned uniform draw in [0,99]. One draw is
// for the entire spell, not one per foe. A failed cast still completes and pays
// its ordinary energy/recharge. World must preflight and commit these requests
// through its shared knockdown authority (including immunity and duration mods).
// The caller supplies the complete actor population in the instance; this
// helper cannot prove that an actor omitted from that population does not exist.
inline std::optional<SlipperyGroundProposal> prepare_slippery_ground(
        const SlipperyGroundPlan& p,const SlipperyGroundActor& caster,
        Scope target,std::span<const SlipperyGroundActor> actors,
        std::optional<unsigned> failure_draw={}){
    if(slippery_ground(p.rank)!=p||!caster.scope.instance||!caster.scope.actor||
       !caster.scope.generation||!caster.alive||!std::isfinite(caster.x)||
       !std::isfinite(caster.z)||actors.size()>4096||
       target.instance!=caster.scope.instance||!target.actor||!target.generation||
       target.actor==caster.scope.actor||(failure_draw&&*failure_draw>=100)||
       (p.failure_percent&&!failure_draw))return {};
    const SlipperyGroundActor* center=nullptr;std::set<unsigned> seen;
    for(const auto& a:actors){
        if(a.scope.instance!=caster.scope.instance||!a.scope.actor||!a.scope.generation||
           !std::isfinite(a.x)||!std::isfinite(a.z)||!seen.insert(a.scope.actor).second)return {};
        if(a.scope==target)center=&a;
    }
    if(!center||!center->alive||!center->hostile||center->plane!=caster.plane||
       std::hypot(center->x-caster.x,center->z-caster.z)>p.range)return {};
    SlipperyGroundProposal result;
    if(p.failure_percent&&*failure_draw<p.failure_percent){result.failed=true;return result;}
    // Local May-2021 wiki revision2630035: each foe independently qualifies
    // when Blind OR moving. A stationary primary target does not gate its AoE.
    for(const auto& a:actors){
        if(!a.alive||!a.hostile||a.plane!=center->plane||
           std::hypot(a.x-center->x,a.z-center->z)>p.radius)continue;
        const bool qualifies=a.blind.value_or(false)||a.moving.value_or(false);
        if(!qualifies&&(!a.blind||!a.moving))return {};
        if(qualifies)result.knockdowns.push_back(a.scope);
    }
    return result;
}
}
