#pragma once
#include "gw_collector_transaction.h"
#include <limits>

namespace fw::gw::collectors::native {
// Private policy, not a retail allocation claim. Existing owned, floor and
// placeholder pools end at60004. Keep this cursor across NPC/map changes so
// an old receive ID can never select a newly opened offer in this connection.
inline constexpr unsigned menu_first=inventory::identity::placeholder_last+1,menu_last=65535;
struct MenuIds {unsigned next=menu_first;};
struct Panel {inventory::Batch packets;std::vector<unsigned> menu_items;};
inline bool displayed_collector(std::string_view displayed,std::string_view expected){
 constexpr std::string_view suffix=" [Collector]";
 if(displayed.ends_with(suffix))displayed.remove_suffix(suffix.size());
 while(!displayed.empty()&&displayed.back()==' ')displayed.remove_suffix(1);
 return displayed==expected;
}
// Used prefixes from collector-window-headings.json; no stale native tail.
// The resource numbers and embedded trophy/cost arguments are preserved.
inline std::optional<std::u16string> captured_heading(std::string_view collector,const Offer& recipe){
 struct Row {std::string_view name;unsigned trophy,cost;};
 constexpr Row rows[]={{"Brownlow",10502,5},{"Jacobs",10508,3},{"Kaylee",10506,3},
                       {"Walden",10499,3},{"Savich",10479,3},{"Humphreys",10494,5},
                       {"Hatcher",10499,5},{"Varis",10474,3}};
 for(const auto& row:rows)if(row.name==collector){
  if(recipe.cost!=row.cost||recipe.trophy.encoded_name!=std::u16string{static_cast<char16_t>(row.trophy)})return {};
  return std::u16string{11059,55010,55495,2666,266,static_cast<char16_t>(row.trophy),1,257,static_cast<char16_t>(256+row.cost)};
 }
 return {};
}
inline std::optional<Panel> open_panel(Binding& active,MenuIds& identities,std::span<const Offer> catalog,
 const Observation& observed,std::string_view displayed){
 active={};
 if(!observed.actor||observed.actor>65535||!observed.map||!observed.world||!observed.npc_generation||!observed.nearby||
    observed.now>std::numeric_limits<std::uint64_t>::max()-120000)return {};
 std::vector<const Offer*> offers;std::set<unsigned> keys;
 for(const auto& offer:catalog)if(offer.map==observed.map&&displayed_collector(displayed,offer.collector)&&
    (!offer.profession||offer.profession==observed.profession)){
  if(!offer.key||!keys.insert(offer.key).second)return {};
  offers.push_back(&offer);
 }
 if(offers.empty()||offers.size()>15)return {};
 const auto& first=*offers.front();const auto heading=captured_heading(first.collector,first);if(!heading)return {};
 for(const auto* offer:offers)if(offer->collector!=first.collector||offer->cost!=first.cost||
    !trophy_matches(offer->trophy,first.trophy)||offer->reward.definition.quantity!=1)return {};
 const auto count=static_cast<unsigned>(offers.size()+1);
 if(identities.next<menu_first||identities.next>menu_last||count>menu_last-identities.next+1)return {};
 auto next=identities.next;Panel out;
 Binding pending{observed.actor,observed.map,observed.world,observed.npc_generation,observed.revision,observed.now+120000,{}};
 items::Packet owner;items::put(owner,195,2);items::put(owner,observed.actor,4);out.packets.push_back(std::move(owner));
 auto cost=first.trophy;cost.id=next++;cost.file|=0x80000000u;cost.quantity=first.cost;
 out.menu_items.push_back(cost.id);out.packets.push_back(items::definition_2022(cost));
 for(const auto* offer:offers){
  auto reward=offer->reward.definition;reward.id=next++;reward.file|=0x80000000u;
  out.menu_items.push_back(reward.id);pending.offers.emplace_back(reward.id,offer->key);
  out.packets.push_back(items::definition_2022(reward));
  if(offer->profession)out.packets.push_back(items::profession_2022(reward.id,offer->profession));
 }
 // Original793a80 appends IDs; original795f60 treats first as cost, remaining
 // as offers. This sequence has been executed against those original bodies.
 items::Packet list;items::put(list,131,2);items::put(list,count,2);
 for(auto id:out.menu_items)items::put(list,id,4);out.packets.push_back(std::move(list));
 items::Packet body;items::put(body,196,2);items::put(body,2,1);items::put(body,static_cast<unsigned>(heading->size()),2);
 for(auto unit:*heading)items::put(body,unit,2);out.packets.push_back(std::move(body));
 identities.next=next;active=std::move(pending);return out;
}
}
