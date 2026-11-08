#pragma once
#include "gw_mantra_recall.h"
#include "gw_energy_regeneration.h"
#include "gw_overcast_settlement.h"
namespace fw::gw::skills::effects {
struct RecallEnergyProposal {
 vitals::Resources resources;energy::Regeneration regeneration;
 std::uint32_t debt=0;Time cursor=0;unsigned repaid=0,restored=0;
 energy::Step before_reward;
};
// Pure settlement at the end event. An already-later Energy cursor cannot be
// rewound safely: callers must settle expiry before costs/capacity changes.
inline std::optional<RecallEnergyProposal> prepare_recall_energy(const RecallReward& reward,Scope owner,
 const vitals::Resources& resources,const energy::Regeneration& regeneration,std::uint32_t debt,Time cursor,int pips,
 std::optional<std::int64_t> ceiling={}){
 const auto limit=ceiling.value_or(resources.max_energy);
 if(reward.owner!=owner||!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||!reward.effect||
    reward.energy<10||reward.energy>30||reward.at<cursor||!resources.health||!resources.max_energy||
    resources.max_energy>1000000||resources.energy>resources.max_energy||debt>1000000||(debt&&resources.energy)||pips<0||pips>10||limit< -1000000||limit>resources.max_energy)return {};
 RecallEnergyProposal result{resources,regeneration,debt,reward.at};
 result.before_reward=result.regeneration.advance(result.resources,reward.at-cursor,pips,result.debt,limit);
 const auto gain=energy::gain(result.resources,result.debt,reward.energy,limit);
 result.repaid=gain.repaid;result.restored=gain.restored;
 if(std::int64_t(result.resources.energy)-result.debt>=limit)result.regeneration.reset();
 return result;
}
inline std::optional<overcast::GainProposal> prepare_recall_energy(const RecallReward& reward,Scope owner,
 const vitals::Resources& resources,const energy::Regeneration& regeneration,std::uint32_t debt,
 const overcast::CeilingCurve& curve,int pips,unsigned fractional_tail=0){
 if(reward.owner!=owner||!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||!reward.effect||
    reward.energy<10||reward.energy>30)return {};
 return overcast::prepare_gain(curve,owner,resources,regeneration,debt,reward.energy,reward.at,pips,fractional_tail);
}

}
