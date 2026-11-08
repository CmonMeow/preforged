#pragma once
#include "gw_attack_outcomes.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct InsidiousParasitePlan {unsigned rank=0,amount=15;Time duration=5000;bool before_attack_damage=true,ends_on_caster_death=true;};
inline std::optional<InsidiousParasitePlan> insidious_parasite(unsigned rank){
    const auto* row=find_skill(123);
    if(rank>20||!row||row->type!=4||row->attribute!=7||row->duration0!=5||row->duration15!=15||row->scale0!=15||row->scale15!=45)return {};
    return InsidiousParasitePlan{rank,15+2*rank,static_cast<Time>(std::lround(5.+10.*rank/15.))*1000};
}
// Call only with a consumed server outcome BEFORE committing attack damage.
// A successful zero-damage hit still
// qualifies;miss/block/dodge/cancellation do not. World ownership and transfer
// checks remain separate from this policy (no client-supplied result accepted).
// The world must end this hex on caster death,not merely suppress its healing.
inline std::optional<InsidiousParasitePlan> insidious_parasite_hit(Scope afflicted,const Entry& entry,
        const combat::weapon::ResolvedAttack& event){
    if(!afflicted.instance||!afflicted.actor||afflicted.actor>65535||!afflicted.generation||event.attacker!=afflicted||
       !event.target.actor||event.target.actor>65535||!event.target.generation||event.target.instance!=afflicted.instance||
       event.target.actor==afflicted.actor||!event.event||event.at<event.released||event.outcome!=combat::weapon::AttackOutcome::hit||
       entry.skill!=123||entry.kind!=Category::hex||!entry.instance||!entry.cause||entry.cause>65535||entry.cause==afflicted.actor||
       entry.applied>event.at||!entry.expires||event.at>=*entry.expires)return {};
    return insidious_parasite(entry.rank);
}
inline std::optional<InsidiousParasitePlan> insidious_parasite_hit(Scope afflicted,const Entry& entry,
        const combat::weapon::QualifiedHit& hit){
    return insidious_parasite_hit(afflicted,entry,combat::weapon::ResolvedAttack{hit.attacker,hit.target,hit.event,hit.released,hit.at,combat::weapon::AttackOutcome::hit});
}
}
