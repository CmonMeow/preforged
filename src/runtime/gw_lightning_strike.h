#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_natural_hex_timers.h"
#include <map>
#include <stdexcept>
#include <vector>
namespace fw::gw::skills::effects {
struct LightningStrikePlan {
 unsigned rank=0;double damage=5;unsigned penetration=25;Time duration=3000;
 auto operator<=>(const LightningStrikePlan&)const=default;
};
inline std::optional<LightningStrikePlan> lightning_strike(unsigned rank){
 const auto* row=find_skill(222);
 if(rank>20||!row||row->type!=4||row->attribute!=8||row->scale0!=5||row->scale15!=50||
    row->duration0!=3||row->duration15!=3)return {};
 return LightningStrikePlan{rank,damage_scale(5,50,rank)};
}
// Timing and attribution for the Overcast-only second strike. Registry/world
// admission remains the caller's responsibility. Early removal cancels this
// skill's damage (the documented exception to ordinary hex end effects).
// World integration must dispatch deadlines BEFORE same-time removal/death.
using LightningStrikeTimers=NaturalHexTimers<lightning_strike>;

}
