#pragma once
#include "gw_effect_registry.h"
#include <limits>

namespace fw::gw::skills::effects {
// Mallyx's captured skill 1885. This is a read-only offer, not a completed
// cast. World must settle enchantment end reactions and choose/publish the
// skill disables atomically before committing any removal.
struct BanishEnchantmentOffer {
    Scope caster,target;
    std::vector<Entry> removed_newest_first;
    Time disable_until=0;
    unsigned disable_events_per_foe=0;
};

inline std::optional<BanishEnchantmentOffer> banish_enchantment_offer(
        Scope caster,Scope target,const Registry& target_effects,bool hostile,Time now){
    const auto* row=find_skill(1885);
    if(!row||row->type!=5||row->profession!=0||row->attribute!=51||
       row->energy_cost()!=5||row->activation!=.75f||row->aftercast!=.75f||
       row->recharge!=4||row->target!=5||
       !caster.instance||caster.instance!=target.instance||
       !caster.actor||caster.actor>65535||!caster.generation||
       !target.actor||target.actor>65535||!target.generation||
       caster==target||!hostile||!target_effects.matches(target)||
       now>std::numeric_limits<Time>::max()-6000)return {};
    auto preview=target_effects;
    BanishEnchantmentOffer result{caster,target,{},now+6000,0};
    while(const auto entry=preview.newest(target,Category::enchantment,now)){
        if(!preview.remove(target,entry->instance))return {};
        result.removed_newest_first.push_back(*entry);
    }
    result.disable_events_per_foe=static_cast<unsigned>(result.removed_newest_first.size());
    return result;
}
}
