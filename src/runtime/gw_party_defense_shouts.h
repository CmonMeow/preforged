#pragma once
#include "gw_shields_up.h"
#include "gw_dont_trip.h"
namespace fw::gw::skills::effects {
inline unsigned party_defense_skill(const ShieldsUpPlan&){return 367;}
inline unsigned party_defense_skill(const DontTripPlan&){return 2216;}
inline auto party_defense_canonical(const ShieldsUpPlan& plan){return shields_up(plan.rank);}
inline auto party_defense_canonical(const DontTripPlan& plan){return dont_trip(plan.rank);}
inline auto prepare_party_defense_shout(const ShieldsUpPlan& plan,Scope source,Scope target,unsigned effect,
 Time now,bool member,bool alive,float distance){return prepare_shields_up(plan,source,target,effect,now,member,alive,distance);}
inline auto prepare_party_defense_shout(const DontTripPlan& plan,Scope source,Scope target,unsigned effect,
 Time now,bool member,bool alive,float distance){return prepare_dont_trip(plan,source,target,effect,now,member,alive,distance);}
}
