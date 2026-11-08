#pragma once
#include "gw_effect_registry.h"
#include <algorithm>
#include <cmath>
#include <tuple>
#include <vector>
#include <map>
#include <set>
#include <optional>
namespace fw::gw::skills::effects {
struct ParasiticBondPlan {unsigned rank=0,healing=30;Time duration=20000;unsigned pips=1;};
inline std::optional<ParasiticBondPlan> parasitic_bond(unsigned rank){
    const auto* row=find_skill(99);
    if(rank>20||!row||row->type!=4||row->attribute!=7||row->scale0!=1||row->scale15!=1||row->duration0!=20||row->duration15!=20)return {};
    return ParasiticBondPlan{rank,static_cast<unsigned>(std::lround(row->bonus0+(double(row->bonus15)-row->bonus0)*rank/15.))};
}
// Attribution/timing only. The world decides whether removal/replacement earns
// healing, applies modifiers and checks recipient Health. No target presence
// polling: removal/death must provide their actual event time.
class ParasiticBonds {
public:
    struct Link {Scope caster,target;unsigned effect=0,rank=0;Time applied=0,expires=0;auto operator<=>(const Link&)const=default;};
    struct Return {Scope caster,target;unsigned effect=0,healing=0;Time at=0;};
private:
    using Key=std::pair<Scope,unsigned>;
    using Links=std::map<Key,Link>;
    Links links_;
    std::map<std::tuple<Time,Scope,unsigned>,Key> deadlines_;
    std::optional<Time> dispatching_;
    static bool valid(Scope s){return s.instance&&s.actor&&s.generation;}
    static Return reward(const Link& l,Time at){return {l.caster,l.target,l.effect,parasitic_bond(l.rank)->healing,at};}
    bool insert(const Link& link){
        const auto [it,added]=links_.emplace(Key{link.target,link.effect},link);
        if(!added)return false;
        try{deadlines_.emplace(std::tuple{link.expires,link.target,link.effect},it->first);}
        catch(...){links_.erase(it);throw;}
        return true;
    }
    Links::iterator erase(Links::iterator it){
        deadlines_.erase(std::tuple{it->second.expires,it->second.target,it->second.effect});
        return links_.erase(it);
    }
public:
    bool add(const Link& link){
        const auto plan=parasitic_bond(link.rank);
        if(!plan||!valid(link.caster)||!valid(link.target)||link.caster.instance!=link.target.instance||
           link.caster==link.target||!link.effect||link.applied>UINT64_MAX-plan->duration||
           link.expires!=link.applied+plan->duration||links_.size()>=4096||
           (dispatching_&&link.applied<*dispatching_))return false;
        return insert(link);
    }
    std::optional<Link> lookup(Scope target,unsigned effect)const{
        const auto found=links_.find({target,effect});
        return found==links_.end()?std::nullopt:std::optional(found->second);
    }
    // Reapplying one's own hex refreshes it without an end heal. Cross-caster
    // replacement has separate documented behavior; this operation cannot
    // silently discard another caster's return.
    bool renew_same_caster(Scope target,unsigned previous,const Link& replacement,Time now){
        const auto old=links_.find({target,previous});const auto plan=parasitic_bond(replacement.rank);
        if(old==links_.end()||!plan||replacement.target!=target||replacement.caster!=old->second.caster||
           !replacement.effect||replacement.effect==previous||replacement.applied!=now||
           now<old->second.applied||now>=old->second.expires||now>UINT64_MAX-plan->duration||
           replacement.expires!=now+plan->duration||links_.contains({target,replacement.effect})||
           (dispatching_&&now<*dispatching_))return false;
        // Allocate first: failure must leave the old timer and return intact.
        if(!insert(replacement))return false;erase(old);return true;
    }
    std::optional<Return> end(Scope target,unsigned effect,Time now,bool heal){
        const auto found=links_.find({target,effect});
        if(found==links_.end()||now<found->second.applied||(dispatching_&&now<*dispatching_))return {};
        const auto link=found->second;erase(found);
        return heal?std::optional(reward(link,std::min(now,link.expires))):std::nullopt;
    }
    // Consume before dispatch so callbacks cannot repeat healing. Natural end
    // retains its deadline even if the world is sampled late.
    std::vector<Return> expire(Time now){
        std::vector<Return> out;
        dispatch_due(now,[&](const Return& returned){out.push_back(returned);});
        return out;
    }
    std::optional<Time> next_deadline()const{
        return deadlines_.empty()?std::nullopt:std::optional(std::get<0>(deadlines_.begin()->first));
    }
    std::set<Scope> participants()const{
        std::set<Scope> out;for(const auto& [key,link]:links_){out.insert(link.caster);out.insert(link.target);}return out;
    }
    // Value snapshots let the world forecast NPC boundaries without retaining
    // iterators across death/removal callbacks. They do not consume returns.
    std::vector<Link> pending()const{
        std::vector<Link> out;out.reserve(links_.size());
        for(const auto& [key,link]:links_)out.push_back(link);return out;
    }
    // World integration must use streaming dispatch, not bulk expire(). Its
    // first callback can kill a caster or renew/remove a later target hex.
    // Later entries must remain cancellable until their own callback starts.
    // Nested advancement cannot overtake the callback currently being applied.
    template<class Apply> std::size_t dispatch_due(Time now,Apply&& apply){
        if(dispatching_)return 0;
        struct Reset {std::optional<Time>& slot;~Reset(){slot.reset();}} reset{dispatching_};
        std::size_t count=0;
        while(!deadlines_.empty()&&std::get<0>(deadlines_.begin()->first)<=now){
            const auto it=links_.find(deadlines_.begin()->second);
            const auto returned=reward(it->second,it->second.expires);
            dispatching_=returned.at;erase(it);++count;apply(returned);
        }
        return count;
    }
    // Caster death requires the owner to remove each returned target hex.
    // All links are invalidated first, before that removal can invoke callbacks.
    std::vector<Link> take_caster(Scope caster){
        std::vector<Link> out;
        for(auto it=links_.begin();it!=links_.end();)if(it->second.caster==caster){out.push_back(it->second);it=erase(it);}else ++it;
        return out;
    }
    void discard(Scope actor){
        for(auto it=links_.begin();it!=links_.end();)if(it->second.caster==actor||it->second.target==actor)it=erase(it);else ++it;
    }
    std::size_t size()const{return links_.size();}
};
}
