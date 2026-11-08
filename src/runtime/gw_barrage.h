#pragma once
#include "gw_skill_effect_plan.h"
namespace fw::gw::combat::weapon {
// Local Barrage.html SHA256566dc4b0a3a386d61ee6cdded21d84eb45c341c28fe72f8b769ff2ccc7f10227.
// Volley.html SHA256977e71751a8db1b62909d43cdd28ac34059309f85b0701ae0a4986404c97f2e5.
// Talk_Volley documents the glyph-removal quirk on2010-06-07.
// Each recipient is a separate arrow/attack, not one area-damage application.
struct BarragePlan {unsigned bonus=5,maximum_targets=6;double adjacent=166.;bool remove_glyphs=false;};
inline std::optional<BarragePlan> barrage(unsigned rank,unsigned skill=395){
 const auto* row=skills::find_skill(skill);
 if((skill!=395&&skill!=2144)||rank>20||!row||row->type!=14||row->attribute!=25||row->energy_cost()!=5||row->recharge!=(skill==395?1u:2u)||
    row->activation!=0.f||row->aftercast!=0.f||row->scale0!=(skill==395?5u:3u)||row->scale15!=row->scale0||row->bonus0!=(skill==395?5u:1u)||row->bonus15!=(skill==395?20u:10u))return {};
 return BarragePlan{skills::effects::interpolate(row->bonus0,row->bonus15,rank),row->scale0+1,166.,skill==2144};
}
}
