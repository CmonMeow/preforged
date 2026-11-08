#pragma once
#include "gw_inventory.h"
namespace fw::gw::bundles {
// Calculation-only projection. Never serialize or persist it as an inventory
// update: owned items and all four saved sets remain unchanged.
inline inventory::Profile without_weapon_benefits(const inventory::Profile& owned){
    weapon_sets::validate(owned.weapons,inventory::weapon_lookup(owned));
    auto view=owned;view.weapons={};
    std::erase_if(view.items,[&](const auto& entry){
        const auto& item=entry.second;const auto bag=view.bags.find(item.bag);
        return bag!=view.bags.end()&&bag->second.type==2&&bag->second.index==21&&item.slot<=1;
    });
    return view;
}
}
