#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_effect_registry.h"
#include <compare>
#include <cmath>
#include <map>
#include <optional>
#include <vector>

namespace fw::gw::skills::effects {
struct PutridBilePlan {
    unsigned rank=0,degeneration_pips=0,death_damage=0;
    std::uint64_t duration_ms=0;
    float range=1248.f,death_radius=252.f;
    auto operator<=>(const PutridBilePlan&)const=default;
};

// The archived wiki progression gives rank 0/15/20 values 5/17/20 seconds,
// 1/3/3 pips and 25/73/85 damage. This catalog's upper fields encode rank 20.
inline std::optional<PutridBilePlan> putrid_bile(unsigned rank){
    const auto* row=find_skill(2058);
    if(rank>20||!row||row->campaign!=4||row->profession!=4||row->attribute!=5||
       row->type!=4||row->energy_cost()!=10||row->activation!=1.f||
       row->aftercast!=.75f||row->recharge!=12||row->target!=5||
       row->scale0!=1||row->scale15!=3||row->bonus0!=25||row->bonus15!=85||
       row->duration0!=5||row->duration15!=20||
       row->caster_body_resource!=2077||row->target_body_resource!=1728)return {};
    const auto progression=[](unsigned low,unsigned at15,unsigned at20,unsigned r){
        if(r<=15)return (low*15+(at15-low)*r+7)/15;
        return (at15*5+(at20-at15)*(r-15)+2)/5;
    };
    return PutridBilePlan{rank,progression(1,3,3,rank),progression(25,73,85,rank),
                          std::uint64_t(progression(5,17,20,rank))*1000};
}

struct PutridBileDeath {
    std::vector<std::uint32_t> recipient_ids;
    std::vector<combat::Actor> targets;
    std::vector<combat::Hit> hits;
};

struct PutridBileLink {
    Scope caster,target;
    unsigned effect=0,rank=0,caster_level=0;
    std::uint64_t applied=0,expires=0;
    auto operator<=>(const PutridBileLink&)const=default;
};
struct PutridBileRecipient {
    Scope scope;
    combat::Actor actor;
    unsigned plane=0;
    double x=0,y=0;
    bool hostile_to_caster=false;
};

class PutridBileLinks {
    std::map<Scope,PutridBileLink> active_;
public:
    std::optional<PutridBileLink> lookup(Scope target)const{
        const auto found=active_.find(target);
        return found==active_.end()?std::nullopt:std::optional(found->second);
    }
    bool assign(PutridBileLink link){
        if(!putrid_bile(link.rank)||!link.caster.instance||!link.caster.actor||
           !link.caster.generation||link.caster.instance!=link.target.instance||
           !link.target.actor||!link.target.generation||!link.effect||
           !link.caster_level||link.caster_level>255||link.applied>=link.expires)return false;
        active_[link.target]=link;return true;
    }
    bool remove(Scope target,unsigned effect){
        const auto found=active_.find(target);
        if(found==active_.end()||found->second.effect!=effect)return false;
        active_.erase(found);return true;
    }
    std::optional<PutridBileLink> take_death(Scope target,std::uint64_t now){
        const auto found=active_.find(target);
        if(found==active_.end())return {};
        const auto link=found->second;
        active_.erase(found); // one death event can explode at most once
        if(now<link.applied||now>=link.expires)return {};
        return link;
    }
    std::vector<PutridBileLink> pending()const{
        std::vector<PutridBileLink> result;result.reserve(active_.size());
        for(const auto& [scope,link]:active_)result.push_back(link);
        return result; // Snapshot stays valid through death/removal callbacks.
    }
    void clear(){active_.clear();}
    void expire(std::uint64_t now){
        std::erase_if(active_,[&](const auto& item){return item.second.expires<=now;});
    }
    std::size_t size()const{return active_.size();}
};

// World must supply only scoped, living, nearby hostile recipients when the
// hexed victim dies. The copy-only batch rejects rather than partially
// resolving a death explosion if any selected recipient is unpublishable.
inline std::optional<PutridBileDeath> putrid_bile_death(
        const PutridBilePlan& plan,std::vector<combat::Actor> recipients,
        unsigned caster_level){
    if(putrid_bile(plan.rank)!=plan||!caster_level||caster_level>255||
       recipients.size()>64)return {};
    PutridBileDeath result{{},std::move(recipients),{}};
    for(auto& actor:result.targets){
        const auto hit=combat::apply(actor,
            {combat::Kind::armor_ignoring,double(plan.death_damage),caster_level,2058},true);
        if(hit.reason!=combat::Reason::accepted)return {};
        result.hits.push_back(hit);
    }
    return result;
}

// The World owns the authoritative death event, link lifetime, positions and
// hostility. This pure selection/resolve stage cannot be invoked from client
// input. Exact victim generation prevents a prior hex from firing on respawn.
inline std::optional<PutridBileDeath> putrid_bile_explosion(
        const PutridBileLink& link,Scope dead,unsigned plane,double x,double y,
        std::uint64_t now,const std::vector<PutridBileRecipient>& candidates){
    const auto plan=putrid_bile(link.rank);
    if(!plan||!link.caster.instance||!link.caster.actor||!link.caster.generation||
       link.caster.instance!=link.target.instance||!link.target.actor||
       !link.target.generation||!link.effect||dead!=link.target||
       !link.caster_level||link.caster_level>255||
       link.applied>now||now>=link.expires||
       !std::isfinite(x)||!std::isfinite(y)||candidates.size()>4096)return {};
    std::vector<combat::Actor> selected;
    std::vector<std::uint32_t> ids;
    for(const auto& c:candidates){
        if(!c.hostile_to_caster||c.scope.instance!=dead.instance||
           c.scope==dead||c.scope.actor!=c.actor.id||
           c.scope.generation!=c.actor.generation||c.plane!=plane)continue;
        if(!std::isfinite(c.x)||!std::isfinite(c.y))return {};
        if(std::hypot(c.x-x,c.y-y)>plan->death_radius)continue;
        if(!c.actor.damageable)continue;
        if(!c.actor.health)return {};
        if(c.actor.health->current<=0)continue;
        ids.push_back(c.actor.id);selected.push_back(c.actor);
        if(selected.size()>64)return {};
    }
    auto result=putrid_bile_death(*plan,std::move(selected),link.caster_level);
    if(!result)return {};
    result->recipient_ids=std::move(ids);
    return result;
}
}
