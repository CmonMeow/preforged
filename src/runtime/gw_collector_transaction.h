#pragma once
#include "gw_collector_exchange.h"
#include "gw_inventory_identity.h"
#include "gw_outgoing_frame.h"
#include <set>

namespace fw::gw::collectors::native {
struct Entry {std::uint32_t item=0,quantity=0;};
struct Request {unsigned opcode=0,type=0,give_gold=0,receive_gold=0;std::vector<Entry> give,receive;};
// Quantity zero in a72 request is an internal full-stack sentinel. A75 wire
// quantity zero is invalid and never acquires those semantics.
inline Request decode(std::span<const std::uint8_t> bytes){
 const auto frame=world::outgoing_frame_2022(bytes);
 if(frame.state!=world::FrameState::complete||frame.size!=bytes.size())throw std::invalid_argument("invalid collector transaction frame");
 std::size_t at=0;auto get=[&](unsigned width){std::uint32_t v=0;
  if(width>bytes.size()-at)throw std::invalid_argument("truncated collector transaction");
  for(unsigned n=0;n<width;++n)v|=std::uint32_t(bytes[at++])<<(8*n);return v;};
 Request out;out.opcode=get(2)&0x7fff;
 if(out.opcode!=72&&out.opcode!=75)throw std::invalid_argument("not a collector transaction opcode");
 out.type=get(1);out.give_gold=get(4);
 const auto entries=[&]{std::vector<Entry> result;const auto count=get(2);
  if(count>16)throw std::invalid_argument("collector transaction ID bound");
  for(unsigned i=0;i<count;++i)result.push_back({get(4),0});
  if(out.opcode==75){if(get(2)!=count)throw std::invalid_argument("collector quantity count differs from ID count");
   for(auto& entry:result){entry.quantity=get(1);if(!entry.quantity)throw std::invalid_argument("zero collector quantity");}}
  return result;};
 out.give=entries();out.receive_gold=get(4);out.receive=entries();return out;
}
struct Binding {
 unsigned actor=0,map=0;std::uint64_t world=0,npc_generation=0,revision=0,deadline=0;
 std::vector<std::pair<unsigned,unsigned>> offers; // menu item ID -> trusted offer key
};
struct Observation {
 unsigned actor=0,map=0,profession=0;std::uint64_t world=0,npc_generation=0,revision=0,now=0;
 bool nearby=false; // caller's authoritative same-NPC lifetime/range check
};
struct Plan {
 const Offer* offer=nullptr;std::vector<Entry> debit;
 items::Packet admitted_inventory;
 unsigned map=0,profession=0;std::uint64_t revision=0;
};
// Every attempt consumes the menu, including malformed requests handled by
// the caller before decode. The caller must also clear Binding on NPC change,
// zone, disconnect, or decode failure. IDs alone never establish authority.
inline std::optional<Plan> authorize(Binding& active,const Request& request,
 std::span<const Offer> catalog,const inventory::Profile& owned,const Observation& observed){
 auto pending=std::move(active);active={};
 if(!pending.actor||!pending.world||!pending.npc_generation||!observed.nearby||
    pending.actor!=observed.actor||pending.map!=observed.map||pending.world!=observed.world||
    pending.npc_generation!=observed.npc_generation||pending.revision!=observed.revision||
    observed.now>=pending.deadline||(request.opcode!=72&&request.opcode!=75)||request.type!=2||
    request.give_gold||request.receive_gold||request.give.empty()||request.give.size()>16||request.receive.size()!=1||
    !request.receive[0].item||(request.opcode==75?request.receive[0].quantity!=1:request.receive[0].quantity!=0))return {};
 const Offer* offer=nullptr;unsigned matched=0;
 for(const auto& [item,key]:pending.offers)if(item==request.receive[0].item){
  ++matched;for(const auto& candidate:catalog)if(candidate.key==key){if(offer)return {};offer=&candidate;}}
 if(matched!=1||!offer||offer->map!=observed.map||!offer->cost||offer->cost>250||
    (offer->profession&&offer->profession!=observed.profession))return {};
 Plan out{offer,{}};std::set<unsigned> seen;unsigned total=0;
 for(const auto& entry:request.give){
  if(!entry.item||!seen.insert(entry.item).second)return {};
  const auto item=owned.items.find(entry.item);if(item==owned.items.end())return {};
  const auto bag=owned.bags.find(item->second.bag);
  if(bag==owned.bags.end()||bag->second.type!=1||!trophy_matches(item->second.definition,offer->trophy))return {};
  const auto quantity=request.opcode==72?item->second.definition.quantity:entry.quantity;
  if((request.opcode==72&&entry.quantity)||!quantity||quantity>250||quantity>item->second.definition.quantity||
     quantity>offer->cost-total)return {};
  total+=quantity;out.debit.push_back({entry.item,quantity});
 }
 if(total!=offer->cost)return {};
 out.admitted_inventory=inventory::encode(owned);out.map=observed.map;
 out.profession=observed.profession;out.revision=observed.revision;return out;
}
inline std::optional<Plan> authorize_wire(Binding& active,std::span<const std::uint8_t> bytes,
 std::span<const Offer> catalog,const inventory::Profile& owned,const Observation& observed){
 try{return authorize(active,decode(bytes),catalog,owned,observed);}
 catch(const std::invalid_argument&){active={};return {};}
}
// Never fall back to exchange()'s automatic trophy selection: the native
// request identified particular owned stacks, which must be the stacks debited.
inline std::optional<Proposal> exchange_selected(const Plan& plan,const inventory::Profile& current,
 unsigned map,unsigned profession,std::uint64_t revision,std::u16string_view owner){
 inventory::validate_character(current);
 if(!plan.offer||plan.map!=map||plan.profession!=profession||plan.revision!=revision||
    plan.admitted_inventory!=inventory::encode(current)||plan.debit.empty()||plan.debit.size()>16)return {};
 const auto& offer=*plan.offer;
 if(!offer.key||!offer.map||map!=offer.map||!offer.cost||offer.cost>250||
    offer.trophy.type!=30||!offer.trophy.file||offer.trophy.encoded_name.empty()||
    (offer.profession&&offer.profession!=profession)||offer.reward.definition.quantity!=1||
    (offer.customize&&(owner.empty()||owner.size()>19||owner.find(u'\0')!=owner.npos)))return {};
 Proposal out{current,{},{}};unsigned total=0;std::set<unsigned> seen;
 for(const auto& debit:plan.debit){
  if(!seen.insert(debit.item).second||!debit.quantity||debit.quantity>offer.cost-total)return {};
  const auto item=out.profile.items.find(debit.item);if(item==out.profile.items.end())return {};
  const auto bag=out.profile.bags.find(item->second.bag);
  if(bag==out.profile.bags.end()||bag->second.type!=1||!trophy_matches(item->second.definition,offer.trophy)||
     debit.quantity>item->second.definition.quantity)return {};
  total+=debit.quantity;auto& quantity=item->second.definition.quantity;quantity-=debit.quantity;
  if(quantity)out.packets.push_back(items::quantity_2022(debit.item,quantity));
  else{out.packets.push_back(items::detach_2022(current.inventory,debit.item));
   out.packets.push_back(items::delete_2022(debit.item));out.profile.items.erase(item);}
 }
 if(total!=offer.cost)return {};
 return issue_reward(current,offer,owner,std::move(out));
}
struct CommittedExchange {inventory::Batch packets;unsigned reward=0;};
template<class Save>
std::optional<CommittedExchange> commit_selected_exchange(const Plan& plan,inventory::identity::Session& identities,
 inventory::Profile& character,inventory::Profile& bank,std::uint64_t& revision,
 unsigned map,unsigned profession,std::u16string_view owner,Save&& save){
 auto proposed=exchange_selected(plan,character,map,profession,revision,owner);if(!proposed)return {};
 if(proposed->reward>inventory::identity::owned_last)return {};
 auto prepared=identities.prepare(proposed->profile,bank);
 if(!identities.preserves_bank(prepared))throw std::runtime_error("native collector proposal changed durable bank");
 const auto saved=save(prepared.durable.character,revision);if(!saved)return {};
 if(!identities.commit(std::move(prepared)))throw std::runtime_error("obsolete committed native collector projection");
 revision=*saved;character=std::move(prepared.wire.character);bank=std::move(prepared.wire.bank);
 return CommittedExchange{std::move(proposed->packets),proposed->reward};
}
}
