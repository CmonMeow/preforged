#pragma once
#include "gw_insidious_parasite.h"
#include "gw_life_stealing.h"
namespace fw::gw::skills::effects {
struct InsidiousParasiteTransfer {Scope caster,afflicted;Time at;LifeStealResult health;};
// Transaction-local computation only. Commit this transfer before calculating
// attack damage from CURRENT recipient Health. The world validates the owned
// caster generation and consumes its pre-damage notification exactly once.
inline std::optional<InsidiousParasiteTransfer> insidious_parasite_transfer(Scope caster,
        Scope afflicted,const Entry& hex,const combat::weapon::QualifiedHit& hit,
        const combat::Health& caster_health,const combat::Health& afflicted_health,double reduction=0){
    if(!caster.instance||caster.instance!=afflicted.instance||!caster.actor||caster.actor>65535||!caster.generation||
       caster.actor==afflicted.actor||hex.cause!=caster.actor)return {};
    const auto plan=insidious_parasite_hit(afflicted,hex,hit);if(!plan)return {};
    const auto transfer=steal_life_amount(plan->amount,caster_health,afflicted_health,reduction);if(!transfer)return {};
    return InsidiousParasiteTransfer{caster,afflicted,hit.at,*transfer};
}
}
