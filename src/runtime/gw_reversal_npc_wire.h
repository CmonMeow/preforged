#pragma once
#include "gw_npc_combat_catalog.h"
#include "gw_skill_particle_wire.h"
#include "gw_weapon_hit.h"
namespace fw::gw::combat::npcs {
// World may suppress attributed healing feedback when the original caster's
// generation is no longer present. Absolute Health still follows the receipt.
struct ReversalHealingSource {std::optional<unsigned> actor;};
// Receipt serialization only. World must retain the NPC/caster lifetimes and
// own death,reactive effects and actual publication under its lock.
inline std::vector<Packet> reversal_damage_packets_2022(unsigned target,std::optional<unsigned> attacker,const LiveActor::ReversalDamage& receipt,
        std::optional<ReversalHealingSource> healing={}){
 if(!target||target>65535||(attacker&&(!*attacker||*attacker>65535||target==*attacker))||
    receipt.removed.skill!=307||!receipt.removed.instance||!receipt.removed.cause||
    !valid(receipt.before_healing)||!valid(receipt.after_healing)||
    receipt.before_healing.maximum!=receipt.after_healing.maximum)
  throw std::invalid_argument("invalid NPC Reversal receipt");
 Packet removed{67,0};combat::detail::put(removed,target);combat::detail::put(removed,receipt.removed.instance);
 std::vector<Packet> out{removed};
 const auto gained=receipt.after_healing.current-receipt.before_healing.current;
 const auto healer=healing?healing->actor:std::optional<unsigned>(receipt.removed.cause);
 if(healer&&(*healer!=receipt.removed.cause||!*healer||*healer>65535))throw std::invalid_argument("invalid Reversal healer attribution");
 if(gained>0&&healer)out.push_back(skills::particles::health_change_2022(target,*healer,gained,receipt.after_healing.maximum));
 out.push_back(combat::detail::fraction(target,receipt.after_healing));
 if(receipt.damage){
  if(receipt.damage->target!=target||receipt.damage->before.current!=receipt.after_healing.current||
     receipt.damage->before.maximum!=receipt.after_healing.maximum)
   throw std::invalid_argument("NPC Reversal damage does not follow healing");
  const auto packets=attacker?damage_packets_2022(*attacker,*receipt.damage):health_packets_2022(
      {target,receipt.quote.target.generation,receipt.damage->after,{},receipt.damage->effects,true});out.insert(out.end(),packets.begin(),packets.end());
 }
 return out;
}
// Weapon conversion keeps the original healing prefix while preserving
// zero attribution and critical notices in the independent damage event.
inline std::vector<Packet> reversal_weapon_packets_2022(unsigned target,unsigned attacker,
        const LiveActor::ReversalDamage& receipt,bool critical=false,std::optional<ReversalHealingSource> healing={}){
 auto prefix=receipt;prefix.damage.reset();
 auto out=reversal_damage_packets_2022(target,attacker,prefix,healing);
 if(receipt.damage){
  const auto& hit=*receipt.damage;
  if(hit.target!=target||hit.before.current!=receipt.after_healing.current||hit.before.maximum!=receipt.after_healing.maximum)
   throw std::invalid_argument("NPC weapon conversion does not follow healing");
  const auto packets=hit.skill?damage_packets_2022(attacker,hit,critical):weapon::packets_2022(attacker,hit,critical);
  out.insert(out.end(),packets.begin(),packets.end());
 }
 return out;
}
// Separate stages leave room for the full transfer's caster feedback between
// retirement and recipient loss. This API asserts no caster payout policy.
struct ReversalLifeStealPackets {std::vector<Packet> before_transfer,remaining_loss;};
inline ReversalLifeStealPackets reversal_life_steal_recipient_packets_2022(unsigned target,unsigned attacker,
        const skills::effects::ReversalLifeStealReceipt& receipt,std::optional<ReversalHealingSource> healing={}){
 const auto plan=skills::effects::reversal_fortune(receipt.removed.rank);
 const auto split=plan?skills::effects::damage_to_healing(receipt.quote.reduced_amount,plan->maximum_healing):std::nullopt;
 if(!target||target>65535||!attacker||attacker>65535||target==attacker||!plan||!split||
    receipt.removed.skill!=307||!receipt.removed.instance||!receipt.removed.cause||receipt.removed.cause>65535||
    receipt.quote.input!=skills::effects::ReversalInput::life_steal||receipt.quote.target.actor!=target||!receipt.quote.target.instance||!receipt.quote.target.generation||!receipt.quote.application||
    receipt.quote.effect!=receipt.removed.instance||receipt.quote.cause!=receipt.removed.cause||receipt.quote.rank!=receipt.removed.rank||
    receipt.quote.reduced_amount<=0||receipt.quote.split!=*split||!receipt.removed.expires||
    receipt.quote.at<receipt.removed.applied||receipt.quote.at>=*receipt.removed.expires||
    !valid(receipt.before_healing)||!valid(receipt.after_healing)||!valid(receipt.after_loss)||receipt.before_healing.current<=0||
    receipt.before_healing.maximum!=receipt.after_healing.maximum||receipt.after_healing.maximum!=receipt.after_loss.maximum||
    !std::isfinite(receipt.lost)||receipt.lost<0||
    receipt.after_loss.current!=std::max(0.,receipt.after_healing.current-receipt.quote.split.remaining)||
    receipt.lost!=receipt.after_healing.current-receipt.after_loss.current)
  throw std::invalid_argument("invalid recipient life-steal conversion receipt");
 ReversalLifeStealPackets packets;
 const auto gained=receipt.after_healing.current-receipt.before_healing.current;
 const auto healer=healing?healing->actor:std::optional<unsigned>(receipt.removed.cause);
 if(healer&&(*healer!=receipt.removed.cause||!*healer||*healer>65535))throw std::invalid_argument("invalid Reversal healer attribution");
 if(gained>0&&healer)packets.before_transfer.push_back(skills::particles::health_change_2022(target,*healer,gained,receipt.after_healing.maximum));
 packets.before_transfer.push_back(combat::detail::fraction(target,receipt.after_healing));
 Packet retired{67,0};combat::detail::put(retired,target);combat::detail::put(retired,receipt.removed.instance);packets.before_transfer.push_back(std::move(retired));
 if(receipt.lost>0){
  packets.remaining_loss.push_back(skills::particles::health_change_2022(target,attacker,-receipt.lost,receipt.after_loss.maximum));
  packets.remaining_loss.push_back(combat::detail::fraction(target,receipt.after_loss));
 }
 return packets;
}

}

