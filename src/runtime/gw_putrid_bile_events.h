#pragma once
#include "gw_putrid_bile.h"
#include <limits>
#include <algorithm>
#include <set>
namespace fw::gw::skills::effects {
// Immutable death ownership is independent of the hex's later cleanup/expiry.
struct PutridBileDeathEvent {
    std::uint64_t serial=0;PutridBileLink link;unsigned plane=0;
    double x=0,y=0;std::uint64_t at=0;
    std::vector<Scope> population;
    auto operator<=>(const PutridBileDeathEvent&)const=default;
};
class PutridBileEvents {
    std::map<std::uint64_t,PutridBileDeathEvent> pending_;
    std::uint64_t next_=1;
public:
    std::optional<PutridBileDeathEvent> reserve(PutridBileLink link,unsigned plane,
            double x,double y,std::uint64_t at,std::vector<Scope> population){
        if(!putrid_bile(link.rank)||!link.caster.instance||!link.caster.actor||link.caster.actor>65535||!link.caster.generation||
           link.caster.instance!=link.target.instance||!link.target.actor||link.target.actor>65535||!link.target.generation||
           !link.effect||!link.caster_level||link.caster_level>255||at<link.applied||at>=link.expires||
           !std::isfinite(x)||!std::isfinite(y)||plane>65535||!next_)return {};
        std::set<Scope> seen;
        for(const auto scope:population)if(scope.instance!=link.target.instance||!scope.actor||scope.actor>65535||!scope.generation||!seen.insert(scope).second)return {};
        for(const auto& [serial,event]:pending_)if(event.link.target==link.target&&event.link.effect==link.effect&&event.at==at)
            return event.link==link&&event.plane==plane&&event.x==x&&event.y==y?std::optional(event):std::nullopt;
        PutridBileDeathEvent event{next_,link,plane,x,y,at,std::move(population)};
        if(next_==UINT64_MAX)next_=0;else ++next_;
        pending_.emplace(event.serial,event);return event;
    }
    std::optional<PutridBileDeathEvent> lookup(std::uint64_t serial)const{
        const auto it=pending_.find(serial);return it==pending_.end()?std::nullopt:std::optional(it->second);
    }
    std::optional<PutridBileDeathEvent> first_due(std::uint64_t at,const std::set<std::uint64_t>& excluded={})const{
        std::optional<PutridBileDeathEvent> result;
        for(const auto& [serial,event]:pending_)if(event.at<=at&&!excluded.contains(serial)&&
            (!result||std::pair{event.at,serial}<std::pair{result->at,result->serial}))result=event;
        return result;
    }
    bool consume(const PutridBileDeathEvent& event){
        const auto it=pending_.find(event.serial);if(it==pending_.end()||it->second!=event)return false;
        pending_.erase(it);return true;
    }
    void retire_caster(Scope source){std::erase_if(pending_,[&](const auto& entry){return entry.second.link.caster==source;});}
    std::size_t size()const{return pending_.size();}
};
}
