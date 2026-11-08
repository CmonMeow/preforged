#pragma once
#include "gw_effect_registry.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct BarbsPlan {unsigned rank=0,damage=1;Time duration=30000;};
inline std::optional<BarbsPlan> barbs(unsigned rank){
    const auto* row=find_skill(101);
    if(rank>20||!row||row->type!=4||row->attribute!=7||row->duration0!=30||row->duration15!=30||row->scale0!=1||row->scale15!=15)return {};
    return BarbsPlan{rank,static_cast<unsigned>(std::lround(1.+14.*rank/15.))};
}
// Raw client damage kinds0/1/2 are blunt/piercing/slashing. A weapon's
// item type is insufficient: converted elemental damage must not trigger it.
inline bool barbs_physical(std::optional<unsigned> raw,double applied){
    return raw&&*raw<=2&&std::isfinite(applied)&&applied>0;
}
inline std::optional<BarbsPlan> active_barbs(const Entry& entry,Time now){
    if(entry.skill!=101||entry.kind!=Category::hex||!entry.instance||!entry.cause||
       entry.applied>now||!entry.expires||now>=*entry.expires)return {};
    return barbs(entry.rank);
}
}
