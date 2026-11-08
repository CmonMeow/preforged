#pragma once
#include "gw_attack_outcomes.h"
#include "gw_weapon_hit.h"
namespace fw::gw::combat::weapon {
// Reuse reviewed damage,not an old Health snapshot,after a pre-damage reaction.
// This chooses neither recipient order nor reactive-effect cardinality.
inline std::optional<Hit> rebase_attack_damage(const QualifiedHit& hit,AttackScope current_scope,
        Actor& current,const Hit& planned){
    if(!hit.event||!hit.attacker.instance||!hit.attacker.actor||hit.attacker.actor>65535||!hit.attacker.generation||
       hit.at<hit.released||hit.target!=current_scope||current_scope.instance!=hit.attacker.instance||
       current_scope.actor==hit.attacker.actor||!current_scope.generation||current.id!=current_scope.actor||current.generation!=current_scope.generation||
       planned.reason!=Reason::accepted||planned.target!=current.id||planned.generation!=current.generation||planned.skill>3432||
       !valid(planned.before)||!planned.before.current||!valid(planned.after)||planned.before.maximum!=planned.after.maximum||
       (planned.kind!=Kind::earth&&planned.kind!=Kind::armor_ignoring&&planned.kind!=Kind::lightning&&planned.kind!=Kind::fire&&planned.kind!=Kind::cold)||
       !std::isfinite(planned.damage)||planned.damage<0||planned.damage>16000000||
       planned.applied!=std::min(planned.damage,planned.before.current)||
       planned.after.current!=std::max(0.,planned.before.current-planned.damage)||
       planned.killed!=(!planned.after.current&&planned.applied>0)||
       (planned.killed!=bool(planned.effects&0x10)))return {};
    const auto rebased=apply_resolved(current,planned.damage,true);if(!rebased)return {};
    auto result=*rebased;result.skill=planned.skill;result.kind=planned.kind;return result;
}
}
