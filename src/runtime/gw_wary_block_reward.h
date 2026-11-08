#pragma once
#include "gw_wary_stance.h"
#include "gw_block_attribution.h"
namespace fw::gw::skills::effects {
// A proposal only: the World must commit this after the attributed attack
// outcome is final, using the same event exactly once. The catalog's 35-unit
// adrenaline parameter credits each eligible bar pool through shared authority.
struct WaryBlockReward {
 Scope owner,attacker;
 unsigned effect_instance=0,energy=5,adrenaline=35;
 Time at=0;
};
inline std::optional<WaryBlockReward> wary_block_reward(const Registry& registry,
 Scope owner,Scope attacker,const AttributedBlock& contact,unsigned attack_skill,Time now){
 const auto eligible=wary_attack_skill(attack_skill);
 if(!eligible||!*eligible||!attacker.actor||!attacker.generation||
    attacker.instance!=owner.instance||attacker.actor==owner.actor||
    contact.outcome!=combat::weapon::AttackOutcome::block||!contact.source||
    contact.source->skill!=377||contact.source->probability!=1.)return {};
 const auto effect=active_wary_stance(registry,owner,now);
 if(!effect||effect->instance!=contact.source->instance)return {};
 const auto plan=wary_stance(effect->rank);
 if(!plan)return {};
 return WaryBlockReward{owner,attacker,effect->instance,plan->energy_reward,
                         plan->raw_adrenaline_parameter,now};
}
}
