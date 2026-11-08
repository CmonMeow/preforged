#pragma once
#include "gw_merchant_transaction.h"

namespace fw::gw::merchant::native {
struct Proposal {inventory::Profile profile;inventory::Batch packets;};
inline std::optional<Proposal> propose(const Plan& plan,const inventory::Profile& current,
 std::uint64_t revision,std::span<const Stock> stock,std::u16string_view owner){
 inventory::validate_character(current);
 if(!still_current(plan,current,revision)||!plan.quote.quantity||plan.quote.quantity>255)return {};
 Proposal out{current,{}};
 if(plan.quote.type==1){
  const Stock* selected=nullptr;
  for(const auto& row:stock)if(row.definition.id==plan.quote.item){if(selected)return {};selected=&row;}
  if(!selected||!plan.stock||items::definition_2022(selected->definition)!=items::definition_2022(*plan.stock)||
     selected->reviewed_unrestricted!=plan.stock_unrestricted||plan.quote.quantity>selected->maximum_quantity||plan.quote.gold>current.gold)return {};
  const auto quoted=buy_quote(*plan.stock,plan.scale,plan.quote.quantity);
  if(!quoted||quoted->gold!=plan.quote.gold)return {};
  collectors::Offer reward;reward.reward.definition=selected->definition;reward.reward.definition.quantity=1;
  if(plan.stock_unrestricted){reward.reward.profession=0;reward.reward.customized=std::u16string{};}
  // Use the same metadata-aware, carrying-bag stack/slot issuer as collectors.
  // Every unit is prepared on copies; failure on the last unit discards all.
  collectors::Proposal issued{current,{},0};
  for(unsigned n=0;n<plan.quote.quantity;++n){
   auto before=issued.profile;
   auto next=collectors::issue_reward(before,reward,owner,std::move(issued));if(!next)return {};
   if(next->reward>inventory::identity::owned_last)return {};
   issued=std::move(*next);
  }
  out.profile=std::move(issued.profile);out.packets=std::move(issued.packets);out.profile.gold-=plan.quote.gold;
 }else if(plan.quote.type==11){
  const auto item=out.profile.items.find(plan.quote.item);if(item==out.profile.items.end())return {};
  const auto bag=out.profile.bags.find(item->second.bag);
  if(bag==out.profile.bags.end()||bag->second.type!=1||plan.quote.quantity>item->second.definition.quantity||
     current.gold>100000||plan.quote.gold>100000-current.gold)return {};
  const auto quoted=sell_quote(item->second.definition,plan.quote.quantity);
  if(!quoted||quoted->gold!=plan.quote.gold)return {};
  auto& quantity=item->second.definition.quantity;quantity-=plan.quote.quantity;
  if(quantity)out.packets.push_back(items::quantity_2022(plan.quote.item,quantity));
  else{
   out.packets.push_back(items::detach_2022(current.inventory,plan.quote.item));
   out.packets.push_back(items::delete_2022(plan.quote.item));out.profile.items.erase(item);
   out.profile.weapons=weapon_sets::erased(out.profile.weapons,plan.quote.item);
   if(out.profile.weapons!=current.weapons){auto packets=inventory::weapon_packets(out.profile);
    out.packets.insert(out.packets.end(),packets.begin(),packets.end());}
  }
  out.profile.gold+=plan.quote.gold;
 }else return {};
 inventory::validate_character(out.profile);return out;
}
template<class Save>
std::optional<inventory::Batch> commit(const Plan& plan,inventory::identity::Session& identities,
 inventory::Profile& character,inventory::Profile& bank,std::uint64_t& revision,
 std::span<const Stock> stock,std::u16string_view owner,Save&& save){
 auto proposed=propose(plan,character,revision,stock,owner);if(!proposed)return {};
 auto prepared=identities.prepare(proposed->profile,bank);
 if(!identities.preserves_bank(prepared))throw std::runtime_error("merchant proposal changed durable bank");
 const auto saved=save(prepared.durable.character,revision);if(!saved)return {};
 if(!identities.commit(std::move(prepared)))throw std::runtime_error("obsolete committed merchant projection");
 revision=*saved;character=std::move(prepared.wire.character);bank=std::move(prepared.wire.bank);
 return std::move(proposed->packets);
}
}
