#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct EndurancePlan {unsigned skill=374,rank=0,energy=3,ceiling=10;Time duration=5000;bool stance=false;auto operator<=>(const EndurancePlan&)const=default;};
inline std::optional<EndurancePlan> warriors_endurance(unsigned skill,unsigned rank){
 const auto* r=find_skill(skill);
 if(rank>20||(skill!=374&&skill!=3002)||!r||r->type!=(skill==374?16u:3u)||r->attribute!=17||r->energy_cost()!=5||r->recharge!=30||r->scale0!=3||r->scale15!=3||r->bonus0!=10||r->bonus15!=25||r->duration0!=5||r->duration15!=35)return {};
 return EndurancePlan{skill,rank,3,10+rank,1000ull*(5+2*rank),skill==3002};
}
inline std::optional<EndurancePlan> active_endurance(const Registry& registry,Scope owner,Time now){
 std::optional<Entry> selected;
 for(unsigned id:{374u,3002u}){const auto e=registry.newest_skill(owner,id,now);
  if(e&&e->cause==owner.actor&&e->expires&&e->kind==(id==374?Category::other:Category::stance)&&warriors_endurance(id,e->rank)&&(!selected||e->applied>selected->applied))selected=e;
 }
 return selected?warriors_endurance(selected->skill,selected->rank):std::nullopt;
}

}
