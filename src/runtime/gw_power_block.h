#pragma once
#include "gw_skillbar.h"
#include "gw_aura_restoration.h"
#include "gw_effect_registry.h"

namespace fw::gw::skills::effects {
struct PowerBlockPlan {
    unsigned rank=0;
    Time disable=1000;
    auto operator<=>(const PowerBlockPlan&)const=default;
};
inline std::optional<PowerBlockPlan> power_block(unsigned rank){
    const auto* row=find_skill(5);
    if(rank>20||!row||row->type!=5||!(row->special&4)||row->attribute!=2||row->scale0!=1||row->scale15!=12)return {};
    return PowerBlockPlan{rank,static_cast<Time>(std::lround(1.+11.*rank/15.))*1000};
}
// World must supply the currently activating spell/chant and current bar.
// Disable selection is independent of whether interrupt protection prevents
// the interruption. No-attribute and PvE-only skills disable only themselves.
// This chooses affected slots;it does not invent a successful interrupt or
// change cooldowns. Runtime must extend,never shorten,existing deadlines.
inline std::optional<std::vector<Slot>> power_block_disabled_slots(const PowerBlockPlan& plan,unsigned active,const Slots& bar){
    const auto canonical=power_block(plan.rank);const auto* interrupted=find_skill(active);
    if(!canonical||*canonical!=plan||!active||!interrupted||(!is_spell(*interrupted)&&interrupted->type!=27))return {};
    const bool group=interrupted->attribute<51&&!(interrupted->special&0x80000u);
    std::set<std::pair<unsigned,unsigned>> seen;std::vector<Slot> result;
    for(const auto& slot:bar){
        if(!slot.skill){if(slot.copy)return {};continue;}
        const auto* row=find_skill(slot.skill);
        if(!row||!seen.emplace(slot.skill,slot.copy).second)return {};
        if(slot.skill==active||(group&&row->attribute==interrupted->attribute&&!(row->special&0x80000u)))result.push_back(slot);
    }
    return result;
}
using DisableDeadlines=std::map<std::pair<unsigned,unsigned>,Time>;
// Pure transaction:caller commits the complete proposed map under the target
// Runtime's lock and publishes only after success. Existing unrelated entries
// and longer deadlines are preserved. No recharge reset can be inferred from
// disable expiry. Active-cast identity/control validation remains the owner's job.
inline std::optional<DisableDeadlines> power_block_deadlines(const DisableDeadlines& current,
        const PowerBlockPlan& plan,unsigned active,const Slots& bar,Time now){
    const auto selected=power_block_disabled_slots(plan,active,bar);
    if(!selected||now>UINT64_MAX-plan.disable)return {};
    for(const auto&[key,deadline]:current)if(!key.first||!find_skill(key.first))return {};
    auto proposed=current;const auto until=now+plan.disable;
    for(const auto& slot:*selected){auto& deadline=proposed[{slot.skill,slot.copy}];deadline=std::max(deadline,until);}
    return proposed;
}
}
