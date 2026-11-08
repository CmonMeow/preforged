#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include <compare>

namespace fw::gw::skills::effects {
struct EarBitePlan {
    unsigned deldrimor_rank=0,damage=50;
    std::uint64_t bleeding_ms=15000;
    float range=144.f;
    auto operator<=>(const EarBitePlan&)const=default;
};
inline std::optional<EarBitePlan> ear_bite(unsigned deldrimor_rank){
    const auto* row=find_skill(2213);
    if(deldrimor_rank>10||!row||row->profession!=0||row->attribute!=51||
       row->type!=10||row->special!=524290||row->energy_cost()!=0||
       row->adrenaline!=100||row->activation!=.75f||row->aftercast!=.75f||
       row->recharge!=0||row->scale0!=15||row->scale15!=25||
       row->bonus0!=50||row->bonus15!=70||row->caster_body_resource!=1905||
       row->target_body_resource!=2077)return {};
    return EarBitePlan{deldrimor_rank,50+2*deldrimor_rank,
        (15u+deldrimor_rank)*1000ull};
}
struct EarBiteResult {
    bool accepted=false;
    combat::Actor target;
    std::optional<combat::Hit> hit;
};
// Damage is armor-respecting piercing. Bleeding is a separate World-owned
// condition commit after a landed, living hit; immunity must not erase damage.
inline EarBiteResult ear_bite_target(const EarBitePlan& plan,
        combat::Actor target,unsigned caster_level,const combat::DamageProjection& projection={}){
    if(ear_bite(plan.deldrimor_rank)!=plan||!caster_level||caster_level>255)return {};
    const auto step=combat::project_damage(target,{combat::Kind::piercing,
        double(plan.damage),caster_level,2213},projection);
    if(!step)return {};
    return {true,target,step->incoming};
}
}
