#pragma once
#include <optional>
#include <compare>
#include <algorithm>
namespace fw::gw::skills::effects {
struct ResurrectionPlan {unsigned skill,health_percent,energy_percent;float range;bool morale_recharge;unsigned rank=0;bool caster_health=false;bool caster_half_health=false;auto operator<=>(const ResurrectionPlan&)const=default;};
// Local wiki Resurrection_Signet.html and Resurrect.html, with pinned37130
// skill rows2/305 and legacy GWCA SkillID enum. Neither removes death penalty.
inline std::optional<ResurrectionPlan> resurrection(unsigned skill,unsigned rank=0){
    if(rank>20)return {};
    if(skill==2)return ResurrectionPlan{2,100,25,1248.f,true};
    if(skill==305)return ResurrectionPlan{305,25,0,1248.f,false};
    if(skill==314)return ResurrectionPlan{314,20+3*rank,42+(48*rank+7)/15,144.f,false,rank};
    if(skill==1128)return ResurrectionPlan{1128,0,5+2*rank,624.f,false,rank,true};
    // The ally receives the caster's current Health after the caster loses
    // half of it. PvP split2866 requires separate review.
    if(skill==791)return ResurrectionPlan{791,0,5+std::min(rank,15u),1248.f,false,rank,true,true};
    // Sunspear Rebirth Signet: full Health, 10% Energy per earned Sunspear
    // rank, target brought to the caster, once per mission/morale boost.
    if(skill==1816&&rank<=10)return ResurrectionPlan{1816,100,10*rank,1248.f,true,rank};
    return {};
}
inline bool valid_resurrection(const ResurrectionPlan& plan){const auto expected=resurrection(plan.skill,plan.rank);return expected&&*expected==plan;}
struct PartyResurrectionPlan {unsigned skill,health_percent,energy_percent;float radius;auto operator<=>(const PartyResurrectionPlan&)const=default;};
inline std::optional<PartyResurrectionPlan> party_resurrection(unsigned skill){
    if(skill==304)return PartyResurrectionPlan{304,25,0,322.f};return {};
}
inline bool valid_party_resurrection(const PartyResurrectionPlan& plan){const auto expected=party_resurrection(plan.skill);return expected&&*expected==plan;}
}
