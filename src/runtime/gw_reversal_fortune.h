#pragma once
#include "gw_effect_registry.h"
#include "gw_combat.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <cmath>
#include <compare>
#include <optional>
namespace fw::gw::skills::effects {
struct ReversalFortunePlan {
    unsigned rank=0,maximum_healing=15;
    Time duration=8000,activation=250,aftercast=750,recharge=2000;
    unsigned energy=5;float range=1248;
    auto operator<=>(const ReversalFortunePlan&)const=default;
};
inline std::optional<ReversalFortunePlan> reversal_fortune(unsigned rank){
    const auto* row=find_skill(307);
    if(rank>20||!row||row->campaign||row->profession!=3||row->attribute!=15||row->type!=6||row->special||
       row->energy_cost()!=5||row->health_sacrifice||row->adrenaline||row->activation!=.25f||row->aftercast!=.75f||
       row->recharge!=2||row->target!=3||row->scale0!=15||row->scale15!=80||row->bonus0||row->bonus15||
       row->duration0!=8||row->duration15!=8||row->caster_body_resource!=2077||row->target_body_resource!=545)return {};
    return ReversalFortunePlan{rank,interpolate(15,80,rank)};
}
// Shared damage-to-healing arithmetic. The caller supplies the amount AFTER
// damage reduction,BEFORE clamping to remaining Health. It must send healing
// through the real healing pipeline BEFORE applying remaining loss. This is
// not a final Health assignment and is not damage prevention equal to2*limit.
struct DamageHealingSplit {
    double prevented=0,healing=0,remaining=0;
    auto operator<=>(const DamageHealingSplit&)const=default;
};
inline std::optional<DamageHealingSplit> damage_to_healing(double reduced_amount,unsigned limit){
    if(!std::isfinite(reduced_amount)||reduced_amount<0||reduced_amount>16000000||!limit||limit>1000000)return {};
    const double converted=std::min(reduced_amount,double(limit));
    return DamageHealingSplit{converted,converted,reduced_amount-converted};
}
enum class ReversalInput {damage,life_steal};
struct ReversalFortuneQuote {
    Scope target;unsigned effect=0,cause=0,rank=0;
    std::uint64_t application=0;Time at=0;
    ReversalInput input=ReversalInput::damage;
    double reduced_amount=0;DamageHealingSplit split;
    auto operator<=>(const ReversalFortuneQuote&)const=default;
};
struct ReversalDamageReceipt {
    Entry removed;combat::Health before_healing,after_healing;
    std::optional<combat::Hit> damage;ReversalFortuneQuote quote;
};
// Recipient-only life-steal conversion. Deliberately no caster-gain claim:
// the caller must separately resolve source cap/overkill and actual transfer.
struct ReversalLifeStealReceipt {
    Entry removed;combat::Health before_healing,after_healing,after_loss;
    ReversalFortuneQuote quote;double lost=0;
};
// Only positive reviewed damage/life-steal inputs are admitted here. Whether
// zero-damage events consume the enchantment remains a retail evidence gap.
// Health loss,degeneration,sacrifice and blocked/missed attacks are not inputs.
inline std::optional<ReversalFortuneQuote> prepare_reversal_fortune(
        const Registry& effects,Scope target,ReversalInput input,double reduced_amount,Time now){
    if((input!=ReversalInput::damage&&input!=ReversalInput::life_steal)||reduced_amount<=0)return {};
    const auto entry=effects.newest_skill(target,307,now);
    if(!entry||entry->kind!=Category::enchantment||!entry->expires)return {};
    const auto plan=reversal_fortune(entry->rank);
    const auto sequence=effects.entry_sequence(target,entry->instance);
    const auto split=plan?damage_to_healing(reduced_amount,plan->maximum_healing):std::nullopt;
    if(!sequence||!split)return {};
    return ReversalFortuneQuote{target,entry->instance,entry->cause,entry->rank,*sequence,now,input,reduced_amount,*split};
}
// Consume exactly the preflighted application while holding its World lock.
// The caller owns the same transaction's healing/loss/status publication and
// full caster lifetime credit. Never call with client-supplied quote fields.
inline std::optional<Entry> consume_reversal_fortune(Registry& effects,const ReversalFortuneQuote& quote,Time now){
    if(now!=quote.at)return {};
    const auto current=prepare_reversal_fortune(effects,quote.target,quote.input,quote.reduced_amount,now);
    if(!current||*current!=quote)return {};
    return effects.remove(quote.target,quote.effect);
}
}
