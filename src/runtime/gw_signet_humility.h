#pragma once
#include "gw_power_block.h"

namespace fw::gw::skills::effects {
struct HumilityPlan {
    unsigned rank=0;
    Time target_disable=1000,self_disable=10000;
    auto operator<=>(const HumilityPlan&)const=default;
};
inline std::optional<HumilityPlan> signet_humility(unsigned rank){
    const auto* row=find_skill(62);
    if(rank>20||!row||row->type!=7||row->attribute!=3||row->scale0!=1||row->scale15!=16||row->bonus0!=10||row->bonus15!=10)return {};
    return HumilityPlan{rank,Time(1+rank)*1000};
}
struct HumilityDeadlines {DisableDeadlines caster,target;};
// Candidate paired transaction:World establishes a valid hostile cast and
// both actor lifetimes. Commit both proposed maps together under owner
// synchronization;never apply the self penalty before target validation.
inline std::optional<HumilityDeadlines> humility_deadlines(const HumilityPlan& plan,
        const DisableDeadlines& caster,const DisableDeadlines& target,const Slots& caster_bar,const Slots& target_bar,Time now){
    const auto canonical=signet_humility(plan.rank);
    if(!canonical||*canonical!=plan||now>UINT64_MAX-std::max(plan.target_disable,plan.self_disable))return {};
    HumilityDeadlines proposed{caster,target};
    for(const auto* map:{&caster,&target})for(const auto&[key,deadline]:*map)if(!key.first||!find_skill(key.first))return {};
    for(bool self:{true,false}){
        const auto& bar=self?caster_bar:target_bar;auto& deadlines=self?proposed.caster:proposed.target;
        const auto until=now+(self?plan.self_disable:plan.target_disable);std::set<std::pair<unsigned,unsigned>> seen;
        for(const auto& slot:bar){
            if(!slot.skill){if(slot.copy)return {};continue;}
            const auto* row=find_skill(slot.skill);if(!row||!seen.emplace(slot.skill,slot.copy).second)return {};
            if(self?row->profession!=5:row->elite()){
                auto& deadline=deadlines[{slot.skill,slot.copy}];deadline=std::max(deadline,until);
            }
        }
    }
    return proposed;
}
}
