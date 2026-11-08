#pragma once
#include "gw_effect_registry.h"
namespace fw::gw::combat::weapon {
struct ArcingShotPlan {unsigned rank=0,bonus=0,speed_percent=50;bool operator==(const ArcingShotPlan&)const=default;};
inline std::optional<ArcingShotPlan> arcing_shot(unsigned rank){
 const auto* r=skills::find_skill(1467);if(rank>20||!r||r->profession!=2||r->type!=14||r->attribute!=25||
  r->special||r->health_sacrifice||r->adrenaline||r->energy_cost()!=5||r->activation!=0||r->aftercast!=0||
  r->recharge!=6||r->weapon_requirement!=2||r->scale0!=10||r->scale15!=25||r->bonus0!=50||r->bonus15!=50||
  r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 return ArcingShotPlan{rank,10+rank};
}
inline bool forked_arrow(){
 const auto* r=skills::find_skill(1722);return r&&r->profession==2&&r->type==14&&r->attribute==51&&
  !r->special&&!r->health_sacrifice&&!r->adrenaline&&r->energy_cost()==10&&r->activation==0&&r->aftercast==0&&
  r->recharge==5&&r->weapon_requirement==2&&!r->scale0&&!r->scale15&&!r->bonus0&&!r->bonus15&&
  r->caster_body_resource==2077&&r->target_body_resource==2077;
}
// Query real scoped effects at weapon release. Unrelated conditions, stances
// and a previous action do not stand in for an active enchantment or hex.
inline std::optional<unsigned> forked_arrow_count(const skills::effects::Registry& registry,skills::effects::Scope owner,skills::effects::Time now){
 if(!forked_arrow()||!registry.matches(owner))return {};
 return registry.has(owner,skills::effects::Category::enchantment,now)||registry.has(owner,skills::effects::Category::hex,now)?1u:2u;
}
}
