#pragma once
#include "gw_effect_registry.h"
#include <cmath>
#include <map>
#include <optional>

namespace fw::gw::skills::effects {
struct ArcaneConundrumPlan {
    unsigned rank=0,energy=0;
    Time duration=0;
    float range=1248.f,radius=166.f;
};
inline std::optional<ArcaneConundrumPlan> arcane_conundrum(unsigned rank){
    if(rank>20)return {};
    const auto* row=find_skill(36);
    if(!row||row->type!=4||row->attribute!=1)return {};
    return ArcaneConundrumPlan{rank,
        static_cast<unsigned>(std::lround(row->bonus0+(double(row->bonus15)-row->bonus0)*rank/15.)),
        static_cast<Time>(std::lround(row->duration0+(double(row->duration15)-row->duration0)*rank/15.))*1000};
}
// Deferred rewards retain both actor lifetimes. Actor IDs alone are insufficient:
// reconnecting actors must never inherit Energy from an earlier map/session.
// The world owner decides which end reasons earn a reward; this ledger does not
// infer removal, renewal or death semantics from an expired clock alone.
class ArcaneConundrumReturns {
public:
    struct Link {Scope caster,target;std::uint32_t effect=0;unsigned energy=0;Time applied=0,expires=0;};
    struct Reward {Scope caster;unsigned energy=0;};
private:
    std::map<std::pair<Scope,std::uint32_t>,Link> links_;
    static bool valid(Scope s){return s.instance&&s.actor&&s.generation;}
public:
    bool add(const Link& link){
        if(!valid(link.caster)||!valid(link.target)||link.caster.instance!=link.target.instance||
           !link.effect||!link.energy||link.energy>9||link.expires<=link.applied)return false;
        const auto key=std::pair{link.target,link.effect};
        if(links_.contains(key)||links_.size()>=4096)return false;
        links_.emplace(key,link);return true;
    }
    std::optional<Reward> end(Scope target,std::uint32_t effect,Time now,bool reward){
        const auto it=links_.find({target,effect});
        if(it==links_.end()||now<it->second.applied)return {};
        const auto link=it->second;links_.erase(it);
        if(!reward)return {};
        return Reward{link.caster,link.energy};
    }
    // Leaving the instance invalidates attribution. Ordinary caster death must
    // not call this: the documented hex persists through caster death.
    void discard(Scope scope){
        for(auto it=links_.begin();it!=links_.end();)
            if(it->second.caster==scope||it->second.target==scope)it=links_.erase(it);else ++it;
    }
    std::vector<Link> pending()const{std::vector<Link> result;for(const auto& [key,link]:links_)result.push_back(link);return result;}
    std::size_t size()const{return links_.size();}
};
}
