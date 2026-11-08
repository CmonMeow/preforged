#pragma once
#include "gw_collector_exchange.h"
#include <algorithm>
#include <iterator>
#include <span>

namespace fw::gw::merchant::native {
// Bounded private support: exact trusted collector trophies, not a universal
// item-type or interaction-bit interpretation. Merchant.html lists trophies as
// sellable. The native warning mask is a confirmation rule, not this policy.
inline bool canonical_trophy_sell(const inventory::Profile& owned,const inventory::Item& item,
 std::span<const collectors::Offer> catalog){
 constexpr unsigned models[]{429,433,432,428,424,427,422};
 const auto& definition=item.definition;
 if(definition.type!=30||definition.price!=3||!definition.quantity||definition.quantity>250||
    !(definition.interaction&0x80000)||
    std::find(std::begin(models),std::end(models),definition.model)==std::end(models)||
    (item.customized&&!item.customized->empty())||(item.profession&&*item.profession)||item.reviewed_armor)return false;
 const auto bag=owned.bags.find(item.bag);
 if(bag==owned.bags.end()||bag->second.type!=1)return false;
 for(const auto& [id,container]:owned.bags)if(container.container==definition.id)return false;
 return std::any_of(catalog.begin(),catalog.end(),[&](const auto& offer){
  return collectors::trophy_matches(definition,offer.trophy);
 });
}
}
