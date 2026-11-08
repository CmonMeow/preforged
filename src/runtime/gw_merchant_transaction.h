#pragma once
#include "gw_collector_transaction.h"
#include "gw_merchant_quote.h"

namespace fw::gw::merchant::native {
using Request=collectors::native::Request;
using Observation=collectors::native::Observation;
struct Stock {items::Definition definition;unsigned maximum_quantity=0;bool reviewed_unrestricted=false;};
struct Binding {
 unsigned actor=0,map=0;std::uint64_t world=0,npc_generation=0,revision=0,deadline=0;
 TrustedScale scale;std::vector<Stock> stock;
 // Explicit server sellability policy, snapshotted from owned carrying items.
 std::vector<unsigned> sellable;
 items::Packet inventory_at_open;
};
struct Plan {Quote quote;items::Packet admitted_inventory;std::uint64_t revision=0;
 std::optional<items::Definition> stock;TrustedScale scale;bool stock_unrestricted=false;};
inline bool still_current(const Plan& plan,const inventory::Profile& current,std::uint64_t revision){
 return plan.revision==revision&&plan.admitted_inventory==inventory::encode(current);
}
// Admission only: caller must atomically commit inventory AND gold, publish both,
// then send verified203 completion type1/11. No acknowledgment on rejection.
inline std::optional<Plan> authorize(Binding& active,const Request& request,
 const inventory::Profile& owned,const Observation& observed){
 auto pending=std::move(active);active={};
 if(!pending.actor||!pending.world||!pending.npc_generation||!observed.nearby||
    pending.actor!=observed.actor||pending.map!=observed.map||pending.world!=observed.world||
    pending.npc_generation!=observed.npc_generation||pending.revision!=observed.revision||
    observed.now>=pending.deadline||(request.opcode!=72&&request.opcode!=75)||
    pending.inventory_at_open.empty()||pending.inventory_at_open!=inventory::encode(owned))return {};
 std::optional<Quote> quote;std::optional<items::Definition> admitted_stock;bool unrestricted=false;
 if(request.type==1){
  // Original merchant buys use partial75, including quantity one.
  if(request.opcode!=75||!request.give.empty()||request.receive_gold||request.receive.size()!=1)return {};
  const auto selected=request.receive.front();const Stock* stock=nullptr;
  for(const auto& row:pending.stock)if(row.definition.id==selected.item){if(stock)return {};stock=&row;}
  if(!stock||!selected.quantity||selected.quantity>stock->maximum_quantity)return {};
  quote=buy_quote(stock->definition,pending.scale,selected.quantity);
  if(!quote||request.give_gold!=quote->gold||owned.gold<quote->gold)return {};
  admitted_stock=stock->definition;unrestricted=stock->reviewed_unrestricted;
 }else if(request.type==11){
  if(request.give_gold||!request.receive.empty()||request.give.size()!=1)return {};
  const auto selected=request.give.front();
  if(std::count(pending.sellable.begin(),pending.sellable.end(),selected.item)!=1)return {};
  const auto item=owned.items.find(selected.item);if(item==owned.items.end())return {};
  const auto bag=owned.bags.find(item->second.bag);
  if(bag==owned.bags.end()||bag->second.type!=1)return {};
  for(const auto& [id,container]:owned.bags)if(container.container==selected.item)return {};
  const unsigned quantity=request.opcode==72?item->second.definition.quantity:selected.quantity;
  if((request.opcode==72&&selected.quantity)||!quantity||quantity>item->second.definition.quantity)return {};
  quote=sell_quote(item->second.definition,quantity);
  if(!quote||request.receive_gold!=quote->gold||owned.gold>100000||quote->gold>100000-owned.gold)return {};
 }else return {};
 return Plan{*quote,inventory::encode(owned),observed.revision,std::move(admitted_stock),pending.scale,unrestricted};
}
inline std::optional<Plan> authorize_wire(Binding& active,std::span<const std::uint8_t> bytes,
 const inventory::Profile& owned,const Observation& observed){
 try{return authorize(active,collectors::native::decode(bytes),owned,observed);}
 catch(const std::invalid_argument&){active={};return {};}
}
}
