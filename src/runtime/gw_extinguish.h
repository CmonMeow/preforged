#pragma once
#include "gw_conditions.h"
#include "gw_skill_catalog_generated.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct ExtinguishPlan {unsigned rank=0,divine_rank=0,burning_heal=10,caster_heal=0;float range=5000;auto operator<=>(const ExtinguishPlan&)const=default;};
inline std::optional<ExtinguishPlan> extinguish(unsigned rank,unsigned divine){
 const auto* row=find_skill(943);
 if(rank>20||divine>20||!row||row->type!=5||row->attribute!=15||row->scale0!=10||row->scale15!=100||row->energy_cost()!=15||row->recharge!=12)return {};
 return ExtinguishPlan{rank,divine,10+6*rank,static_cast<unsigned>(std::lround(3.2*divine)),5000};
}
struct ExtinguishRecipient {std::optional<combat::conditions::Kind> removed;unsigned healing=0;};
// Party/lifetime/range and real removal/Health callbacks belong to World.
// Select before callbacks: Burning merely being present does not earn healing.
inline std::optional<ExtinguishRecipient> extinguish_recipient(const ExtinguishPlan& plan,
 const combat::conditions::State& state,bool caster,std::uint64_t now){
 const auto canonical=extinguish(plan.rank,plan.divine_rank);if(!canonical||*canonical!=plan)return {};
 const auto selected=state.newest(now);
 return ExtinguishRecipient{selected,(selected==combat::conditions::Kind::burning?plan.burning_heal:0)+(caster?plan.caster_heal:0)};
}
}
