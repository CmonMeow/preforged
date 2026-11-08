#pragma once
#include "gw_condition_spells.h"
#include <compare>
namespace fw::gw::skills::effects {
struct AllyCleansingPlan{unsigned skill=0,rank=0,health_gain=0,divine_rank=0,divine_heal=0;bool draw=false,self=true;float range=1248;unsigned energy_gain=0;bool disease_half_recharge=false,health_is_healing=false;auto operator<=>(const AllyCleansingPlan&)const=default;};
// Returned by the same authoritative transaction that transfers conditions.
// Recharge must not inspect a condition after the transaction removed it.
struct CleansingOutcome{unsigned conditions_transferred=0;bool removed_disease=false;};
inline std::optional<AllyCleansingPlan> ally_cleansing(unsigned skill,unsigned rank=0,unsigned divine=0){
 if(rank>20||divine>20)return {};
 if(skill==311)return AllyCleansingPlan{skill,rank,interpolate(6,26,rank),divine,static_cast<unsigned>(std::lround(3.2*divine)),true,false};
 // Wiki behavior notes: despite the description, Foul Feast heals. Its
 // Disease branch has a fixed2s recharge regardless of recharge modifiers.
 if(skill==2057)return AllyCleansingPlan{skill,rank,interpolate(0,45,rank),0,0,true,false,1248,interpolate(0,2,rank),true,true};
 if(skill==295)return AllyCleansingPlan{skill,rank,0,0,0,false,true};return {};
}
inline bool valid_ally_cleansing(const AllyCleansingPlan& p){auto expected=ally_cleansing(p.skill,p.rank,p.divine_rank);return expected&&*expected==p;}
}
