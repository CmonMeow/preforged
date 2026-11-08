#pragma once
#include "gw_attack_outcomes.h"
#include <span>
namespace fw::gw::combat::weapon {
struct AttackRelease {AttackScope target;std::uint64_t event=0;AttackDelivery delivery=AttackDelivery::unknown;};
// Plan all recipients before replacing the world's ledger or inflicting any
// reactive damage. A late invalid recipient must not partially release an AoE.
// The caller determines retail hit/failure cardinality;this helper does not.
template<class Ledger>
inline std::optional<Ledger> stage_attack_outcome_batch(const Ledger& current,
        AttackScope source,std::span<const AttackRelease> releases,std::uint64_t at){
    if(releases.empty()||releases.size()>4096)return {};
    auto staged=current;
    for(const auto& release:releases)if(!staged.release(source,release.target,release.event,at,release.delivery))return {};
    return staged;
}
}
