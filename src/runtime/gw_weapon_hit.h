#pragma once
#include "gw_combat.h"

namespace fw::gw::combat::weapon {
// Pinned37130 descriptor60b91c ->8736e0 ->790cf0 ->764720 ->77e750.
// Native body attacks assert when their interval/modifier have not been set.
inline Packet timing_2022(std::uint32_t actor,float interval,float modifier){
 if(!actor||actor>65535||!std::isfinite(interval)||interval<=0||!std::isfinite(modifier)||modifier<=0)
  throw std::invalid_argument("invalid ordinary attack timing");
 Packet out{53,0};detail::put(out,actor);detail::put(out,std::bit_cast<std::uint32_t>(interval));
 detail::put(out,std::bit_cast<std::uint32_t>(modifier));return out;
}
inline Packet start_2022(std::uint32_t attacker,std::uint32_t target){
 if(!attacker||attacker>65535||!target||target>65535||attacker==target)
  throw std::invalid_argument("invalid ordinary attack presentation actors");
 Packet out{159,0};detail::put(out,4);detail::put(out,attacker);
 detail::put(out,target);detail::put(out,0);return out;
}
inline Packet finish_2022(std::uint32_t attacker,bool ranged){
 if(!attacker||attacker>65535)throw std::invalid_argument("invalid ordinary attack actor");
 return detail::integer(ranged?2:1,attacker,0);
}
inline Packet stop_2022(std::uint32_t attacker){
 if(!attacker||attacker>65535)throw std::invalid_argument("invalid ordinary attack actor");
 return detail::integer(3,attacker,0);
}
// Damage is already resolved against reviewed weapon/body armor by the caller.
// Scope, hostility and attack release must be checked under the world lock.
// This path never invents a skill attribution for an ordinary attack.
inline std::optional<Hit> apply_resolved(Actor& target,double damage,bool hostile){
 if(!target.id||target.id>65535||!hostile||!target.damageable||!target.health||
    !valid(*target.health)||!target.health->current||(target.effects&0x10)||
    !std::isfinite(damage)||damage<0||damage>16000000)return {};
 const auto capped=capped_damage(damage,target.incoming_damage_cap);if(!capped)return {};damage=*capped;
 Hit hit;hit.reason=Reason::accepted;hit.target=target.id;hit.generation=target.generation;
 hit.skill=0;hit.before=*target.health;hit.damage=damage;hit.cumulative_reduction=target.cumulative_reduction;
 hit.applied=std::min(damage,hit.before.current);hit.after=hit.before;
 hit.after.current=std::max(0.0,hit.before.current-damage);
 hit.killed=!hit.after.current&&hit.applied>0;hit.effects=target.effects|(hit.killed?0x10u:0u);
 target.health=hit.after;target.effects=hit.effects;return hit;
}
inline std::vector<Packet> packets_2022(std::uint32_t cause,const Hit& hit,bool critical=false){
 if(hit.reason!=Reason::accepted||!hit.damage)return {};
 if(!cause||cause>65535||!hit.target||hit.target>65535||cause==hit.target||hit.skill||
    !valid(hit.before)||!valid(hit.after)||hit.before.maximum!=hit.after.maximum||
    !std::isfinite(hit.damage)||hit.damage<0||hit.damage>16000000||
    hit.applied!=std::min(hit.damage,hit.before.current)||
    !valid_damage_health(hit)||
    hit.killed!=(!hit.after.current&&hit.applied>0)||(hit.killed&&!(hit.effects&0x10)))
     throw std::invalid_argument("invalid ordinary hit publication");
 // Original794f60 accepts zero skill attribution and clears its scratch.
 // Queue atomically: zero attribution158, signed damage162, absolute161.
 Packet modifier{162,0};detail::put(modifier,critical?17:16);detail::put(modifier,hit.target);
 detail::put(modifier,cause);detail::put(modifier,std::bit_cast<std::uint32_t>(
     static_cast<float>(-hit.damage/hit.after.maximum)));
 std::vector<Packet> out{detail::integer(10,cause,0),std::move(modifier),detail::fraction(hit.target,hit.after)};
 if(hit.killed)out.push_back(detail::effects(hit.target,hit.effects));return out;
}
}
