#pragma once
#include "gw_shockwave.h"
namespace fw::gw::skills::effects {
inline bool valid_aftershock_plan(const AoEPlan& plan) {
 const auto expected=caster_earth_aoe(174,plan.rank);
 if(!expected||plan.skill!=174||plan.count!=1)return false;
 for(unsigned i=0;i<3;++i){const auto& a=plan.strikes[i];const auto& b=expected->strikes[i];
  if(a.radius!=b.radius||a.earth_damage!=b.earth_damage||a.knocked_down_bonus!=b.knocked_down_bonus||a.condition!=b.condition||a.duration_seconds!=b.duration_seconds)return false;}
 return true;
}
// Knockdown is supplied only by authoritative timed state, never animation or
// a client's asserted status. Missing evidence means the ordinary damage hit.
inline ShockwaveResult aftershock_target(const AoEPlan& plan,const ShockwaveTarget& target,double distance,unsigned level,uint32_t caster,uint64_t now,bool hostile,bool knocked_down,const EarthArmorSequence& armor_at_hit={},const EarthDamageResolver& damage_at_hit={}) {
 ShockwaveResult result;result.after=target;
 if(!valid_aftershock_plan(plan)||!caster||!std::isfinite(distance)||distance<0)return result;
 if(!hostile||distance>plan.strikes[0].radius){result.accepted=true;return result;}
 if(!target.actor.earth_armor)return result;
 const auto first_base=armor_at_hit?armor_at_hit(nullptr):target.actor.earth_armor;if(!first_base)return ShockwaveResult{};
 result.after.actor.earth_armor=target.conditions.earth_armor(*first_base,now);
 const auto& strike=plan.strikes[0];
 const combat::Damage incoming{combat::Kind::earth,strike.earth_damage,level,174};
 auto hit=damage_at_hit?damage_at_hit(result.after.actor,incoming):combat::apply(result.after.actor,incoming,true);
 if(hit.reason!=combat::Reason::accepted)return ShockwaveResult{};
 result.hits.push_back(hit);
 // Official checkpoint lines1321/1322: the knockdown bonus is a separate
 // damage event. Flat protection therefore applies independently to each hit.
 if(knocked_down&&result.after.actor.health&&result.after.actor.health->current>0&&strike.knocked_down_bonus){
  const auto next_base=armor_at_hit?armor_at_hit(&result.hits.back()):target.actor.earth_armor;if(!next_base)return ShockwaveResult{};
  result.after.actor.earth_armor=target.conditions.earth_armor(*next_base,now);
  const combat::Damage incoming_bonus{combat::Kind::earth,strike.knocked_down_bonus,level,174};
  const auto bonus=damage_at_hit?damage_at_hit(result.after.actor,incoming_bonus):combat::apply(result.after.actor,incoming_bonus,true);
  if(bonus.reason!=combat::Reason::accepted)return ShockwaveResult{};
  result.hits.push_back(bonus);
 }
 result.after.actor.earth_armor=target.actor.earth_armor;result.accepted=true;return result;
}
}
