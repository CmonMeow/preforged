#pragma once
#include "gw_heal_effects.h"
namespace fw::gw::skills::effects {
struct InfuseHealthPlan {unsigned rank=0,divine_rank=0,percent=100;auto operator<=>(const InfuseHealthPlan&)const=default;};
inline std::optional<InfuseHealthPlan> infuse_health(unsigned rank,unsigned divine=0){
    if(rank>20||divine>20)return {};
    return InfuseHealthPlan{rank,divine,static_cast<unsigned>(std::lround(100.+36.*rank/15.))};
}
// A server-world compound operation must count actual active target statuses
// and commit healing under the same lifetime/instance lock.
struct StatusHealPlan {unsigned skill,rank,divine_heal,base,per_status;float range=1248.f;bool self=false;};
inline std::optional<StatusHealPlan> status_heal(unsigned skill,unsigned rank,unsigned divine){const auto* row=find_skill(skill);if((skill!=283&&skill!=1396)||!row||rank>20||divine>20)return {};return StatusHealPlan{skill,rank,static_cast<unsigned>(std::lround(3.2*divine)),heal_scale(row->scale0,row->scale15,rank),heal_scale(row->bonus0,row->bonus15,rank),1248.f,skill==1396};}
inline std::optional<unsigned> status_heal_amount(const StatusHealPlan& p,unsigned enchantments,unsigned hexes,unsigned conditions=0){const auto reviewed=status_heal(p.skill,p.rank,0);if(!reviewed||p.base!=reviewed->base||p.per_status!=reviewed->per_status||p.range!=reviewed->range||p.self!=reviewed->self||enchantments>128||hexes>128||enchantments+hexes>128||conditions>10||p.divine_heal>64)return {};return p.base+p.per_status*(p.skill==1396?unsigned(conditions!=0):enchantments+hexes)+p.divine_heal;}
}
