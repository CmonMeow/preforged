#pragma once
#include "gw_attack_outcomes.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct PriceOfFailurePlan {unsigned rank=0,damage=1,miss_percent=25;Time duration=30000;};
inline std::optional<PriceOfFailurePlan> price_of_failure(unsigned rank){
    const auto* row=find_skill(103);
    if(rank>20||!row||row->type!=4||row->attribute!=7||row->duration0!=30||row->duration15!=30||
       row->scale0!=1||row->scale15!=46||row->bonus0!=25||row->bonus15!=25)return {};
    return PriceOfFailurePlan{rank,1+3*rank};
}
// The owner must obtain this terminal event from AttackOutcomes once,and read
// the victim's current scoped hex registry. Admission failure or a discarded
// target lifetime is not evidence that an attack failed to hit.
inline std::optional<PriceOfFailurePlan> price_of_failure_reaction(Scope victim,const Entry& entry,const combat::weapon::ResolvedAttack& event){
    if(event.attacker!=victim||!victim.instance||!victim.actor||!victim.generation||!event.event||
       event.target.instance!=victim.instance||!event.target.actor||!event.target.generation||event.target.actor==victim.actor||
       event.at<event.released||!event.failed_hit()||entry.skill!=103||entry.kind!=Category::hex||
       !entry.instance||!entry.cause||entry.cause==victim.actor||entry.applied>event.at||!entry.expires||event.at>=*entry.expires)return {};
    return price_of_failure(entry.rank);
}
}
