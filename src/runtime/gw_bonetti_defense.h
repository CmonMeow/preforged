#pragma once
#include "gw_bonetti_defense_policy.h"
#include "gw_block_attribution.h"
namespace fw::gw::skills::effects {
struct BonettiBlockReward {Scope owner,attacker;unsigned energy=5;Time at=0;};
inline std::optional<BonettiBlockReward> bonetti_block_reward(const Registry& registry,
 Scope owner,Scope attacker,const AttributedBlock& contact,bool melee,Time now){
 if(!melee||!attacker.actor||!attacker.generation||attacker.instance!=owner.instance||attacker.actor==owner.actor||
    contact.outcome!=combat::weapon::AttackOutcome::block||!contact.source||
    contact.source->skill!=380||contact.source->probability!=.75)return {};
 const auto e=active_bonetti_defense(registry,owner,now);
 if(!e||e->instance!=contact.source->instance)return {};
 return BonettiBlockReward{owner,attacker,5,now};
}
}
