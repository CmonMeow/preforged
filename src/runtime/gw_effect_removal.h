#pragma once
#include "gw_effect_registry.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct RemovalPlan {unsigned skill,rank;Category kind=Category::hex;unsigned count=1,conditional_heal=0,divine_heal=0;float range=1248.f;bool self=true;};
inline std::optional<RemovalPlan> removal(unsigned skill,unsigned rank,unsigned divine){
 const auto* row=find_skill(skill);if(!row||rank>20||divine>20||(skill!=301&&skill!=2003))return {};
 const auto healing=skill==2003?static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.)):0u;
 return RemovalPlan{skill,rank,Category::hex,1,healing,static_cast<unsigned>(std::lround(3.2*divine)),1248.f,true};
}
}
