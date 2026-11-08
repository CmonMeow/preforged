#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include <compare>
namespace fw::gw::skills::effects {
enum class ChilblainsOrder {damage_then_remove,remove_then_damage};
// Reviewed official windows: damage precedes matched, non-expiring
// enchantment removal, including a Watchful Healing end-heal sequence.
inline constexpr ChilblainsOrder captured_chilblains_order=ChilblainsOrder::damage_then_remove;
// Staged capability only. The World transaction must settle cold damage,
// enchantment-removal reactions and self-poison before enabling normal casting.
struct ChilblainsPlan {
    unsigned rank=0,enchantments=1;
    double damage=10;
    float range=1248,radius=322;
    unsigned energy=25;
    std::uint64_t activation=2000,aftercast=750,recharge=8000,poison=10000;
    auto operator<=>(const ChilblainsPlan&)const=default;
};
inline std::optional<ChilblainsPlan> chilblains(unsigned rank){
    const auto* row=find_skill(144);
    if(rank>20||!row||row->profession!=4||row->attribute!=7||row->type!=5||row->special||
       row->energy_cost()!=25||row->health_sacrifice||
       row->activation!=2.f||row->aftercast!=.75f||row->recharge!=8||
       row->scale0!=10||row->scale15!=44||row->bonus0!=1||row->bonus15!=2||
       row->caster_body_resource!=256||row->target_body_resource!=2077)return {};
    return ChilblainsPlan{rank,interpolate(1,2,rank),damage_scale(10,44,rank)};
}
// Copy-only condition proposal: a rejected World transaction must not leave
// the caster poisoned. Existing longer Poison is preserved by shared rules.
inline std::optional<combat::conditions::State> chilblains_poison(
        const ChilblainsPlan& plan,const combat::conditions::State& conditions,
        unsigned caster,std::uint64_t now){
    const auto canonical=chilblains(plan.rank);
    if(!canonical||*canonical!=plan||!caster||caster>65535)return {};
    auto proposed=conditions;
    if(!proposed.apply(combat::conditions::Kind::poison,caster,now,plan.poison))return {};
    return proposed;
}
}

