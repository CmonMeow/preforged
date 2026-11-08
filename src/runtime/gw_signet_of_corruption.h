#pragma once
#include "gw_skill_catalog_generated.h"
#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <cmath>
#include <optional>
#include <set>
#include <span>
#include <vector>

namespace fw::gw::skills::effects {
struct SignetOfCorruptionPlan {
 unsigned skill=0,rank=0,damage=0,energy_cap=0;
 unsigned energy_per_afflicted=2;
 float target_range=1248.0f,nearby_radius=252.0f;
 auto operator<=>(const SignetOfCorruptionPlan&)const=default;
};
inline std::optional<SignetOfCorruptionPlan> signet_of_corruption(unsigned skill,unsigned rank){
 if((skill!=1950&&skill!=2093)||rank>12)return {};
 const auto* row=find_skill(skill);
 if(!row||row->profession!=4||row->attribute!=51||row->type!=7||
    row->special!=524288||row->energy_cost()||row->adrenaline||
    row->activation!=1.0f||row->aftercast!=0.75f||row->recharge!=20||
    row->scale0!=20||row->scale15!=30||row->bonus0!=2||
    row->bonus15!=2||row->duration0!=12||row->duration15!=20||
    row->caster_body_resource!=1621)return {};
 constexpr std::array<unsigned,7> damage{20,21,23,25,27,28,30};
 constexpr std::array<unsigned,7> cap{12,13,15,16,17,18,20};
 const auto n=std::min(rank,6u);
 return SignetOfCorruptionPlan{skill,rank,damage[n],cap[n]};
}
inline std::optional<unsigned> signet_of_corruption_energy(const SignetOfCorruptionPlan& plan,unsigned afflicted_foes){
 const auto canonical=signet_of_corruption(plan.skill,plan.rank);
 if(!canonical||*canonical!=plan)return {};
 return std::min(plan.energy_cap,plan.energy_per_afflicted*std::min(afflicted_foes,plan.energy_cap));
}
struct CorruptionFoe {
 unsigned actor=0;std::uint64_t generation=0;
 float x=0,y=0;
 bool hostile=false,alive=false,conditioned=false,hexed=false;
};
struct CorruptionContact {unsigned actor=0;std::uint64_t generation=0;unsigned damage=0;bool energy_qualified=false;};
struct CorruptionArea {
 std::vector<CorruptionContact> contacts;
 unsigned energy=0;
};
// World snapshots settled positions/status at cast completion and owns the
// actual damage and Energy transaction. Selection is centered on the foe,
// not on the caster, and a foe with both afflictions counts only once.
inline std::optional<CorruptionArea> signet_of_corruption_area(
 const SignetOfCorruptionPlan& plan,CorruptionFoe target,
 std::span<const CorruptionFoe> foes){
 const auto canonical=signet_of_corruption(plan.skill,plan.rank);
 if(!canonical||*canonical!=plan||
    !target.actor||!target.generation||!target.hostile||!target.alive||
    !std::isfinite(target.x)||!std::isfinite(target.y)||foes.size()>4096)return {};
 CorruptionArea result;bool found=false;unsigned afflicted=0;std::set<unsigned> seen;
 for(const auto& foe:foes){
  if(!foe.actor||!foe.generation||!seen.insert(foe.actor).second||
     !std::isfinite(foe.x)||!std::isfinite(foe.y))return {};
  if(foe.actor==target.actor){
   if(found||foe.generation!=target.generation||foe.x!=target.x||foe.y!=target.y||
      foe.hostile!=target.hostile||foe.alive!=target.alive||
      foe.conditioned!=target.conditioned||foe.hexed!=target.hexed)return {};
   found=true;
  }
  if(!foe.hostile||!foe.alive||std::hypot(foe.x-target.x,foe.y-target.y)>plan.nearby_radius)continue;
  const bool qualified=foe.conditioned||foe.hexed;
  result.contacts.push_back({foe.actor,foe.generation,plan.damage,qualified});
  afflicted+=qualified;
 }
 if(!found)return {};
 result.energy=*signet_of_corruption_energy(plan,afflicted);
 return result;
}
}
