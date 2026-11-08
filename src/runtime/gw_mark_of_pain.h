#pragma once
#include "gw_barbs.h"
namespace fw::gw::skills::effects {
struct MarkOfPainPlan {unsigned rank=0,damage=10;Time duration=30000;double radius=166.;};
inline std::optional<MarkOfPainPlan> mark_of_pain(unsigned rank){
    const auto* row=find_skill(150);
    if(rank>20||!row||row->type!=4||row->attribute!=7||row->duration0!=30||row->duration15!=30||row->scale0!=10||row->scale15!=40)return {};
    return MarkOfPainPlan{rank,10+2*rank};
}
// Qualification only. The world must still validate stored caster/target
// lifetimes,select adjacent hostile recipients excluding the hexed victim,
// and deliver separate shadow damage through the common damage pipeline.
inline std::optional<MarkOfPainPlan> mark_of_pain_physical(const Entry& entry,
        std::optional<unsigned> raw,double applied,Time now){
    if(!barbs_physical(raw,applied)||entry.skill!=150||entry.kind!=Category::hex||!entry.instance||!entry.cause||
       entry.applied>now||!entry.expires||now>=*entry.expires)return {};
    return mark_of_pain(entry.rank);
}
}
