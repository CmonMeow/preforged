#pragma once
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct EndurePainPlan {
    unsigned rank=0,health_bonus=90,energy=5;std::uint64_t duration=7000,recharge=30000;
    auto operator<=>(const EndurePainPlan&)const=default;
};
inline std::optional<EndurePainPlan> endure_pain(unsigned rank){
    const auto* row=find_skill(347);
    if(rank>20||!row||row->profession!=1||row->attribute!=17||row->type!=16||row->special||
       row->energy_cost()!=5||row->health_sacrifice||row->adrenaline||
       row->activation!=0||row->aftercast!=0||row->recharge!=30||row->scale0!=90||row->scale15!=300||
       row->duration0!=7||row->duration15!=18||row->caster_body_resource!=595||row->target_body_resource!=2077)return {};
    return EndurePainPlan{rank,interpolate(90,300,rank),5,std::uint64_t(interpolate(7,18,rank))*1000,30000};
}
}

