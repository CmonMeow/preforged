#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include <compare>

namespace fw::gw::skills::effects {
struct NecrosisPlan {
    unsigned sunspear_rank=0,damage=60;
    float range=1248.f;
    auto operator<=>(const NecrosisPlan&)const=default;
};
// Sunspear rank is an earned title snapshot, not the unassigned native
// attribute 51 and not the caster's profession rank.
inline std::optional<NecrosisPlan> necrosis(unsigned sunspear_rank){
    const auto* row=find_skill(2103);
    if(sunspear_rank>10||!row||row->profession!=4||row->attribute!=51||
       row->type!=10||row->special!=524288||row->energy_cost()!=5||
       row->activation!=1.f||row->aftercast!=.75f||row->recharge!=2||
       row->scale0!=60||row->scale15!=90||row->caster_body_resource!=2077||
       row->target_body_resource!=838)return {};
    return NecrosisPlan{sunspear_rank,60+3*sunspear_rank};
}
struct NecrosisResult {
    bool accepted=false;
    combat::Actor target;
    std::optional<combat::Hit> hit;
};
// A valid cast on an unafflicted foe still completes without damage. World
// supplies the authoritative, settled condition/hex predicate at completion.
inline NecrosisResult necrosis_target(const NecrosisPlan& plan,combat::Actor target,
        bool afflicted,unsigned caster_level){
    if(necrosis(plan.sunspear_rank)!=plan||!target.health||
       !combat::valid(*target.health)||target.health->current<=0||
       !target.damageable||!caster_level||caster_level>255)return {};
    if(!afflicted)return {true,target,{}};
    const auto hit=combat::apply(target,{combat::Kind::armor_ignoring,
        double(plan.damage),caster_level,2103},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    return {true,target,hit};
}
}
