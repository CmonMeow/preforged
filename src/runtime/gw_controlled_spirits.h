#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace fw::gw::skills::effects::spirits {
// A detached authority record. World must still create actors and publish their
// 2022 spawn/attack/death packets before any spirit skill can be source-bound.
struct Record {
 Scope spirit,owner;
 unsigned definition=0,level=0,attack_damage=0;
 Time expires=0;
 unsigned step_resource=0;
 unsigned attack_skill=0;
 auto operator<=>(const Record&)const=default;
};
inline constexpr Time signet_attack_release_windup=900;
inline constexpr Time signet_attack_start_period=2000;
struct SignetPlan {
 unsigned rank=0,level=0,attack_damage=0;
 Time lifetime=60000;
 unsigned count=3;
 // Six spirits in the pinned map34 receipt repeatedly start attacks at an
 // approximately 2000ms cadence. Attack scheduling and target choice remain open.
 Time nominal_attack_period=signet_attack_start_period;
 // Fixed map34 starts launch their projectiles about 0.9s later. The 2s
 // catalogue activation is the repeated start cadence, not release windup.
 Time nominal_release_windup=signet_attack_release_windup;
 bool operator==(const SignetPlan&)const=default;
};
inline constexpr std::array<unsigned,3> signet_attack_skills{3153,3154,3155};
inline bool signet_attack_skill(unsigned id){
 return std::find(signet_attack_skills.begin(),signet_attack_skills.end(),id)!=signet_attack_skills.end();
}
// Six fixed map-34 launch/arrival pairs measure 1596 world units per second.
// This computes a flight duration only; World still owns launch, collision,
// target choice, arrival and damage. No generic spirit speed is inferred.
inline std::optional<float> signet_attack_flight_seconds(float from_x,float from_y,float to_x,float to_y){
 if(!std::isfinite(from_x)||!std::isfinite(from_y)||!std::isfinite(to_x)||!std::isfinite(to_y))return {};
 const double distance=std::hypot(double(to_x)-from_x,double(to_y)-from_y);
 const double seconds=distance/1596.0;
 if(!std::isfinite(seconds)||seconds<=0||seconds>30)return {};
 return float(seconds);
}
inline std::optional<SignetPlan> signet_plan(unsigned rank){
 const auto* r=find_skill(1239);
 if(rank>20||!r||r->profession!=8||r->attribute!=34||r->type!=7||r->special!=4||
    r->energy_cost()!=0||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=30||
    r->scale0!=1||r->scale15!=12||r->bonus0!=5||r->bonus15!=20||
    r->duration0!=60||r->duration15!=60||r->caster_body_resource!=1096)return {};
 for(const auto id:signet_attack_skills){const auto* attack=find_skill(id);
  if(!attack||attack->profession!=8||attack->attribute!=34||attack->type!=14||
     attack->activation!=2.f||attack->aftercast!=0.f||attack->scale0!=1||
     attack->scale15!=8||attack->bonus0!=5||attack->bonus15!=20)return {};
 }
 return SignetPlan{rank,unsigned(interpolate(1,12,rank)),unsigned(interpolate(5,20,rank))};
}
struct AttackResult {
 combat::Actor target;
 std::optional<combat::Hit> hit;
};
// Original 2022 opcode159/kind50 queues the targeted attack event. The three
// internal attack skill IDs are present in the pinned 2022 catalogue and its
// body consumer; this does not establish projectile or rendered acceptance.
inline combat::Packet attack_start_2022(std::uint32_t spirit,std::uint32_t target,
                                        std::uint32_t attack_skill){
 if(!spirit||spirit>65535||!target||target>65535||spirit==target||
    !signet_attack_skill(attack_skill)||!signet_plan(0))
  throw std::invalid_argument("invalid controlled spirit attack start");
 combat::Packet packet{159,0};
 combat::detail::put(packet,50);combat::detail::put(packet,spirit);
 combat::detail::put(packet,target);combat::detail::put(packet,attack_skill);
 return packet;
}
// Six pinned modern spirit releases use projectile model2, an opaque live
// synchronization key, and arrival kind7. The original 2022 descriptor and
// consumers accept this layout; the caller still owns flight and key lifetime.
inline combat::Packet attack_projectile_2022(std::uint32_t spirit,float x,float y,
                                              std::uint16_t plane,float seconds,
                                              std::uint32_t synchronization){
 if(!spirit||spirit>65535||!synchronization||!std::isfinite(x)||!std::isfinite(y)||
    !std::isfinite(seconds)||seconds<=0||seconds>30)
  throw std::invalid_argument("invalid controlled spirit projectile");
 combat::Packet packet{163,0x80};combat::detail::put(packet,spirit);
 combat::detail::put(packet,std::bit_cast<std::uint32_t>(x));
 combat::detail::put(packet,std::bit_cast<std::uint32_t>(y));
 packet.push_back(std::uint8_t(plane));packet.push_back(std::uint8_t(plane>>8));
 combat::detail::put(packet,std::bit_cast<std::uint32_t>(seconds));
 combat::detail::put(packet,2);combat::detail::put(packet,synchronization);
 packet.push_back(0);return packet;
}
inline combat::Packet attack_arrival_2022(std::uint32_t spirit,std::uint32_t synchronization){
 if(!spirit||spirit>65535||!synchronization)
  throw std::invalid_argument("invalid controlled spirit arrival");
 combat::Packet packet{166,0x80};combat::detail::put(packet,spirit);
 combat::detail::put(packet,synchronization);combat::detail::put(packet,7);
 return packet;
}
// Contact-only calculation. The owning World must still verify the living
// spirit, hostile target, range, obstruction, attack release and block before
// using this result. Spirit attacks use the Signet's fixed armor-ignoring
// damage rather than the ordinary weapon/target-armor resolver.
inline AttackResult attack_target(const SignetPlan& plan,unsigned attack_skill,combat::Actor target,bool hostile){
 AttackResult result{target,{}};
 if(signet_plan(plan.rank)!=plan||!signet_attack_skill(attack_skill)||!hostile)return result;
 const auto hit=combat::apply(result.target,
     {combat::Kind::armor_ignoring,double(plan.attack_damage),plan.level,attack_skill},true);
 if(hit.reason==combat::Reason::accepted)result.hit=hit;
 return result;
}
struct SummonPlan {
 unsigned rank=0,healing=60;
 bool operator==(const SummonPlan&)const=default;
};
inline std::optional<SummonPlan> summon_plan(unsigned rank){
 const auto* r=find_skill(2100);
 if(rank>20||!r||r->profession!=8||r->attribute!=51||r->type!=5||r->special!=524288||
    r->energy_cost()!=5||r->activation!=.25f||r->aftercast!=.75f||r->recharge!=5||
    r->scale0!=60||r->scale15!=100||r->caster_body_resource!=2077||
    r->target_body_resource!=1163)return {};
 return SummonPlan{rank,interpolate(60,100,rank)};
}
class Registry {
 std::map<Scope,Record> by_spirit_;
 Time clock_=0;
 static bool valid(Scope s){return s.instance&&s.actor&&s.generation;}
 bool valid_record(const Record& r,Time now)const{
  return valid(r.spirit)&&valid(r.owner)&&r.spirit.instance==r.owner.instance&&
      r.spirit!=r.owner&&r.definition&&r.level&&r.attack_damage&&
      signet_attack_skill(r.attack_skill)&&r.expires>now;
 }
public:
 bool add(const Record& r,Time now){
  if(now<clock_||!valid_record(r,now)||
     by_spirit_.size()>=4096||by_spirit_.contains(r.spirit))return false;
  clock_=now;by_spirit_.emplace(r.spirit,r);return true;
 }
 // Prepare and commit a multi-spirit cast as one ownership transaction.
 // World must stage the same number of actors and packets before calling this.
 bool add_group(std::span<const Record> records,Time now){
  if(now<clock_||records.empty()||records.size()>32||by_spirit_.size()+records.size()>4096)return false;
  std::map<Scope,Record> next=by_spirit_;
  const auto owner=records.front().owner;
  for(const auto& record:records){
   if(!valid_record(record,now)||record.owner!=owner||!next.emplace(record.spirit,record).second)return false;
  }
  by_spirit_.swap(next);clock_=now;return true;
 }
 std::optional<Record> find(Scope spirit,Time now)const{
  const auto it=by_spirit_.find(spirit);
  if(it==by_spirit_.end()||it->second.expires<=now)return {};
  return it->second;
 }
 std::vector<Record> owned(Scope owner,Time now)const{
  std::vector<Record> result;
  if(!valid(owner))return result;
  for(const auto& [scope,r]:by_spirit_)if(r.owner==owner&&r.expires>now)result.push_back(r);
  return result;
 }
 bool retire(Scope spirit){return by_spirit_.erase(spirit)!=0;}
 std::size_t retire_owner(Scope owner){return std::erase_if(by_spirit_,[&](const auto& entry){return entry.second.owner==owner;});}
 template<class Expire>void advance(Time now,Expire&& expired){
  if(now<clock_)return;
  clock_=now;
  for(auto it=by_spirit_.begin();it!=by_spirit_.end();){
   if(it->second.expires<=now){const auto record=it->second;it=by_spirit_.erase(it);expired(record);}
   else ++it;
  }
 }
 std::size_t size()const{return by_spirit_.size();}
};
}
