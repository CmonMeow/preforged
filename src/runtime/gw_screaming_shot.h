#pragma once
#include "gw_skill_effect_plan.h"
#include <cmath>
namespace fw::gw::combat::weapon {
struct ScreamingShotPlan {unsigned rank=0,bonus=0;std::uint64_t bleeding_ms=0;double earshot=1012;bool operator==(const ScreamingShotPlan&)const=default;};
inline std::optional<ScreamingShotPlan> screaming_shot(unsigned rank){
 const auto* r=skills::find_skill(1719);if(rank>20||!r||r->profession!=2||r->type!=14||r->attribute!=25||
  r->special||r->health_sacrifice||r->adrenaline||r->energy_cost()!=10||r->activation!=0||r->aftercast!=0||
  r->recharge!=8||r->weapon_requirement!=2||r->scale0!=10||r->scale15!=25||r->bonus0!=5||r->bonus15!=20||
  r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 return ScreamingShotPlan{rank,10+rank,1000ull*(5+rank)};
}
inline std::optional<bool> screaming_earshot(const ScreamingShotPlan& p,double sx,double sy,double tx,double ty,bool same_plane){
 const auto canonical=screaming_shot(p.rank);if(!canonical||*canonical!=p||!std::isfinite(sx)||!std::isfinite(sy)||!std::isfinite(tx)||!std::isfinite(ty))return {};
 return same_plane&&std::hypot(tx-sx,ty-sy)<=p.earshot;
}
}
