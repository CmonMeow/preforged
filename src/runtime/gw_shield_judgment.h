#pragma once
#include "gw_attack_outcomes.h"
#include "gw_holy_damage.h"
namespace fw::gw::skills::effects {
struct ShieldJudgmentPlan {
 unsigned rank=0,damage=5,knockdown_ms=2000,energy=15;
 Time duration=8000,activation=1000,aftercast=750,recharge=45000;
 auto operator<=>(const ShieldJudgmentPlan&)const=default;
};
inline std::optional<ShieldJudgmentPlan> shield_judgment(unsigned rank){
 const auto holy=holy_damage(262,rank);const auto* row=find_skill(262);
 if(!holy||!row||row->profession!=3||row->health_sacrifice||row->bonus0||row->bonus15||row->caster_body_resource!=2077)return {};
 ShieldJudgmentPlan plan;plan.rank=rank;plan.damage=holy->damage;plan.duration=1000ull*interpolate(8,20,rank);return plan;
}
struct ShieldJudgmentReaction {
 Scope caster,protected_actor,attacker;unsigned effect=0;
 std::uint64_t attack_event=0;Time at=0;combat::Damage damage;unsigned knockdown_ms=0;
};
// Pure proposal from a server-owned TERMINAL qualified hit. World must consume
// that outcome once,validate the stored caster scope/current relationships,
// and atomically preflight damage/control before committing the proposal.
// A zero-damage successful hit qualifies;failed/blocked/cancelled attacks do not.
inline std::optional<ShieldJudgmentReaction> shield_judgment_reaction(Scope caster,Scope protected_actor,
        const Entry& enchantment,const combat::weapon::ResolvedAttack& hit,
        std::optional<bool> attacker_hostile_to_caster,const HolyTraits& traits,unsigned caster_level){
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!valid(caster)||!valid(protected_actor)||!valid(hit.attacker)||caster.instance!=protected_actor.instance||hit.attacker.instance!=protected_actor.instance||
    hit.target!=protected_actor||hit.attacker.actor==protected_actor.actor||!hit.event||hit.at<hit.released||
    !hit.before_damage||*hit.before_damage<hit.released||*hit.before_damage>hit.at||!combat::weapon::valid_attack_delivery(hit.delivery)||
    hit.outcome!=combat::weapon::AttackOutcome::hit||!attacker_hostile_to_caster||enchantment.skill!=262||
    enchantment.kind!=Category::enchantment||!enchantment.instance||enchantment.cause!=caster.actor||enchantment.applied>hit.at||
    !enchantment.expires||hit.at>=*enchantment.expires)return {};
 const auto plan=shield_judgment(enchantment.rank);if(!plan)return {};
 // Allegiance may change after application. The documented allied-attacker
 // anomaly retains knockdown but suppresses damage;unknown allegiance rejects.
 std::optional<combat::Damage> damage;
 if(*attacker_hostile_to_caster)damage=holy_strike(*holy_damage(262,plan->rank),traits,caster_level);
 else {combat::Damage zero{combat::Kind::armor_ignoring,0.,caster_level,262};if(combat::valid(zero))damage=zero;}
 if(!damage)return {};
 return ShieldJudgmentReaction{caster,protected_actor,hit.attacker,enchantment.instance,hit.event,hit.at,*damage,plan->knockdown_ms};
}
}
