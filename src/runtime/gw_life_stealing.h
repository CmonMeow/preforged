#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_angorodons_gaze.h"
#include <compare>
namespace fw::gw::skills::effects {
struct LifeStealPlan{unsigned skill=0,rank=0,amount=0;float range=1248;std::uint64_t weakness_if_attacking_ms=0;unsigned energy_if_condition=0;auto operator<=>(const LifeStealPlan&)const=default;};
inline std::optional<LifeStealPlan> life_steal(unsigned skill,unsigned rank=0){
 if(skill==2189){const auto plan=angorodons_gaze(rank);if(!plan)return {};return LifeStealPlan{2189,rank,plan->amount,plan->range,0,plan->energy};}
 if(rank>20)return {};if(skill==153)return LifeStealPlan{153,rank,interpolate(18,60,rank),1248};
 if(skill==902){const auto* row=find_skill(skill);
  if(!row||row->type!=5||row->profession!=4||row->attribute!=4||row->energy_cost()!=5||row->health_sacrifice||
     row->activation!=1.f||row->aftercast!=.75f||row->recharge!=5||row->scale0!=5||row->scale15!=45||
     row->bonus0!=3||row->bonus15!=12||row->caster_body_resource!=2077||row->target_body_resource!=170)return {};
  return LifeStealPlan{skill,rank,interpolate(5,45,rank),1248,std::uint64_t(interpolate(3,12,rank))*1000};}
 if(skill==1076){const auto* row=find_skill(skill);
  if(!row||row->type!=5||row->profession!=4||row->attribute!=4||row->energy_cost()!=5||row->health_sacrifice||
     row->activation!=2.f||row->aftercast!=.75f||row->recharge!=8||row->scale0!=20||row->scale15!=65||
     row->bonus0!=50||row->bonus15!=50||row->duration0!=10||row->duration15!=10||
     row->caster_body_resource!=1177||row->target_body_resource!=1178)return {};
  return LifeStealPlan{skill,rank,interpolate(20,65,rank),1248};}
 if(skill==156||skill==1077)return LifeStealPlan{skill,rank,interpolate(29,74,rank),144};return {};
}
// Blood Drinker tests the caster before the transfer, not after its gain.
inline std::optional<bool> blood_drinker_bleeds(const combat::Health& source){
 if(!combat::valid(source)||source.current<=0)return {};
 return source.current>double(source.maximum)/2;
}
inline bool valid_life_steal(const LifeStealPlan& plan){if(plan.skill==110)return plan.rank<=20&&plan.amount==interpolate(10,65,plan.rank)&&plan.range==322.f&&!plan.weakness_if_attacking_ms&&!plan.energy_if_condition;const auto expected=life_steal(plan.skill,plan.rank);return expected&&*expected==plan;}
// Unholy Feast is caster-centered and capped; it is not an unlimited AoE.
// This plan remains separate from single-target dispatch until world selection
// and transactional per-recipient transfers are connected.
struct AreaLifeStealPlan{unsigned skill,rank,amount,maximum_targets;float radius;auto operator<=>(const AreaLifeStealPlan&)const=default;};
inline std::optional<AreaLifeStealPlan> area_life_steal(unsigned skill,unsigned rank=0){
 if(skill!=110||rank>20)return {};
 return AreaLifeStealPlan{skill,rank,interpolate(10,65,rank),interpolate(1,4,rank),322.f};
}
struct LifeStealResult{combat::Health source,target;double stolen=0,gained=0;bool killed=false;};
// Wiki Life_stealing: overkill removes target's remaining Health plus one;
// caster gains that amount, independent of armor and healing modifiers.
inline std::optional<LifeStealResult> steal_life_reduced(double reduced,const combat::Health& source,const combat::Health& target){
 if(!std::isfinite(reduced)||reduced<0||reduced>1000000||!combat::valid(source)||!combat::valid(target)||source.current<=0||target.current<=0)return {};
 LifeStealResult result{source,target};result.stolen=std::min(reduced,target.current+1);result.gained=std::min(result.stolen,source.maximum-source.current);
 result.source.current+=result.gained;result.target.current=std::max(0.0,target.current-result.stolen);result.killed=!result.target.current;return result;
}
inline std::optional<LifeStealResult> steal_life_amount(unsigned amount,const combat::Health& source,const combat::Health& target,double reduction=0){
 if(amount>1000000||!std::isfinite(reduction)||reduction<0||reduction>1000000)return {};
 return steal_life_reduced(std::max(0.,double(amount)-reduction),source,target);
}
inline std::optional<LifeStealResult> steal_life(const LifeStealPlan& plan,const combat::Health& source,const combat::Health& target,double reduction=0){
 if(!valid_life_steal(plan))return {};return steal_life_amount(plan.amount,source,target,reduction);
}
inline std::vector<combat::Packet> life_steal_packets(unsigned caster,unsigned target,const LifeStealResult& result){
 if(!caster||!target||caster>65535||target>65535||caster==target||!combat::valid(result.source)||!combat::valid(result.target)||!std::isfinite(result.stolen)||!std::isfinite(result.gained)||result.stolen<0||result.gained<0||result.gained>result.stolen)throw std::invalid_argument("invalid life stealing publication");
 if(!result.stolen)return {};
 const auto delta=[](unsigned owner,unsigned cause,double amount,unsigned maximum){combat::Packet packet{162,0};combat::detail::put(packet,55);combat::detail::put(packet,owner);combat::detail::put(packet,cause);combat::detail::put(packet,std::bit_cast<std::uint32_t>(static_cast<float>(amount/maximum)));return packet;};
 std::vector<combat::Packet> packets{delta(target,caster,-result.stolen,result.target.maximum),combat::detail::fraction(target,result.target)};
 if(result.gained)packets.push_back(delta(caster,caster,result.gained,result.source.maximum));packets.push_back(combat::detail::fraction(caster,result.source));return packets;
}
}
