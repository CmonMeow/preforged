#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <span>
#include <set>

namespace fw::gw::skills::effects {
// Local Deathly_Swarm.html revision2607586 (2016), and pinned2022 row105.
// This is a delayed spell, NOT a projectile: no flight speed, interception,
// obstruction, or dodge test belongs here. Three distinct foes, one per second.
struct DeathlySwarmPlan {
    unsigned rank=0;double damage=0;float range=1248, radius=322;
    auto operator<=>(const DeathlySwarmPlan&)const=default;
};
inline std::optional<DeathlySwarmPlan> deathly_swarm(unsigned rank){
    const auto* row=find_skill(105);
    if(rank>20||!row||row->attribute!=5||row->type!=5)return {};
    return DeathlySwarmPlan{rank,damage_scale(row->scale0,row->scale15,rank)};
}
inline bool valid_deathly_swarm(const DeathlySwarmPlan& p){const auto e=deathly_swarm(p.rank);return e&&*e==p;}
struct SwarmCandidate {Scope scope;double x=0,y=0;bool alive=false,hostile=false;};
struct SwarmStrike {Scope target;Time due=0;unsigned ordinal=0;};
class DeathlySwarm {
    Scope caster_{},primary_{},anchor_{};DeathlySwarmPlan plan_{};
    std::set<std::uint32_t> hit_;Time next_=0,last_=0;unsigned count_=0;
    double anchor_x_=0,anchor_y_=0;bool pending_=false;
public:
    bool launch(Scope caster,Scope target,const DeathlySwarmPlan& plan,Time now){
        if(pending_||!valid_deathly_swarm(plan)||!caster.instance||!caster.actor||!caster.generation||
           target.instance!=caster.instance||!target.actor||!target.generation||target.actor==caster.actor||now>UINT64_MAX-3000)return false;
        caster_=caster;primary_=target;plan_=plan;hit_.clear();count_=0;next_=now+1000;last_=now;pending_=true;return true;
    }
    bool pending()const{return pending_;}
    Scope caster()const{return caster_;}
    const DeathlySwarmPlan& plan()const{return plan_;}
    Time due()const{return next_;}
    void cancel(){pending_=false;}
    // Call once per due strike, apply that strike, then refresh candidates
    // before advancing again. This preserves deaths when a tick spans >1 hit.
    // Candidate order is caller policy: retail tie selection is undocumented.
    // Radius is centered on the preceding victim, not the original target.
    std::optional<SwarmStrike> advance(Scope current_caster,Time now,std::span<const SwarmCandidate> candidates){
        if(!pending_)return {};
        if(current_caster!=caster_||now<last_||candidates.size()>4096){cancel();return {};}
        last_=now;if(now<next_)return {};
        std::set<std::uint32_t> seen;
        for(const auto& c:candidates){
            if(c.scope.instance!=caster_.instance||!c.scope.actor||!c.scope.generation||
               !std::isfinite(c.x)||!std::isfinite(c.y)||!seen.insert(c.scope.actor).second){cancel();return {};}
            // Follow the previous victim's current position, including its corpse.
            if(count_&&c.scope==anchor_){anchor_x_=c.x;anchor_y_=c.y;}
        }
        const SwarmCandidate* chosen=nullptr;
        for(const auto& c:candidates){
            if(!c.alive||!c.hostile||c.scope.actor==caster_.actor||hit_.contains(c.scope.actor))continue;
            if(!count_?c.scope==primary_:std::hypot(c.x-anchor_x_,c.y-anchor_y_)<=plan_.radius){chosen=&c;break;}
        }
        if(!chosen){cancel();return {};}
        hit_.insert(chosen->scope.actor);anchor_=chosen->scope;anchor_x_=chosen->x;anchor_y_=chosen->y;
        SwarmStrike strike{chosen->scope,next_,++count_};if(count_==3)cancel();else next_+=1000;return strike;
    }
};
inline std::optional<combat::Hit> deathly_swarm_target(const DeathlySwarmPlan& plan,
        combat::Actor actor,const combat::conditions::State& conditions,unsigned level,Time now){
    if(!valid_deathly_swarm(plan)||!actor.cold_armor)return {};
    actor.cold_armor=conditions.armor(*actor.cold_armor,now);
    const auto hit=combat::apply(actor,{combat::Kind::cold,plan.damage,level,105,0},true);
    if(hit.reason!=combat::Reason::accepted)return {};return hit;
}
}
