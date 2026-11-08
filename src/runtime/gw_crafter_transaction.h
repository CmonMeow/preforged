#pragma once
#include "gw_merchant_transaction.h"
#include "gw_collector_panel.h"
#include <type_traits>
namespace fw::gw::crafter {
using Request=collectors::native::Request;using Observation=collectors::native::Observation;
struct Material {unsigned element=0,units=0;};
// Trusted recipe data: original5890f0 table row, not client-provided prices.
// Item materials is one-based recipe index; unknown2 scales materials only.
struct Recipe {inventory::Item reward;unsigned native_index=0,gold=0,maximum_quantity=10;std::vector<Material> materials;};
inline bool valid_recipe(const Recipe& r){
 const auto& d=r.reward.definition;
 if(r.native_index>=0x5da||d.materials!=r.native_index+1||!d.unknown2||d.quantity!=1||
    !r.maximum_quantity||r.maximum_quantity>250||r.gold>100000||r.materials.empty()||r.materials.size()>16||
    ((d.interaction&2)&&r.maximum_quantity>1)||(!(d.interaction&0x80000)&&r.maximum_quantity>10))return false;
 auto checked=d;checked.id=1;(void)items::definition_2022(checked);std::set<unsigned> slots;
 for(auto m:r.materials)if(!inventory::materials::crafting_slot(m.element)||!m.units||m.units>250||!slots.insert(m.element).second)return false;
 return true;
}
struct Binding {Observation observed;std::uint64_t deadline=0;items::Packet inventory;std::vector<Recipe> recipes;};
// No panel publication here. The caller must separately review/install
// recipe list membership before creating an opening. Fresh shared menu IDs
// prevent old receive IDs from authorizing another opening.
inline bool bind(Binding& active,collectors::native::MenuIds& ids,const Observation& at,std::string_view name,
 const inventory::Profile& owned,std::span<const Recipe> trusted){
 active={};
 if(at.map!=148||(name!="Arthur Ayala"&&name!="Arthur Ayala [Weapons]")||!at.actor||at.actor>65535||
    !at.world||!at.npc_generation||!at.nearby||at.now>UINT64_MAX-120000||trusted.empty()||trusted.size()>16||
    ids.next<collectors::native::menu_first||ids.next>collectors::native::menu_last||trusted.size()>collectors::native::menu_last-ids.next+1)return false;
 inventory::validate_character(owned);Binding next{at,at.now+120000,inventory::encode(owned),{}};
 unsigned cursor=ids.next;for(auto recipe:trusted){if(!valid_recipe(recipe))return false;recipe.reward.definition.id=cursor++;next.recipes.push_back(std::move(recipe));}
 ids.next=cursor;active=std::move(next);return true;
}
struct Debit {unsigned item=0,quantity=0;};
struct Proposal;
class Plan {
 Recipe recipe_;unsigned quantity_=0,gold_=0;std::vector<Debit> debits_;items::Packet inventory_;std::uint64_t revision_=0;
 Plan(Recipe recipe,unsigned quantity,unsigned gold,std::vector<Debit> debits,items::Packet inventory,std::uint64_t revision):
  recipe_(std::move(recipe)),quantity_(quantity),gold_(gold),debits_(std::move(debits)),inventory_(std::move(inventory)),revision_(revision){}
 friend std::optional<Plan> authorize(Binding&,const Request&,const inventory::Profile&,const Observation&);
 friend std::optional<Proposal> propose(const Plan&,const inventory::Profile&,std::uint64_t);
public:
 const std::vector<Debit>& selected_debits()const{return debits_;}
 unsigned quantity()const{return quantity_;}unsigned gold()const{return gold_;}
};
inline std::optional<Plan> authorize(Binding& active,const Request& request,const inventory::Profile& owned,const Observation& at){
 auto opening=std::move(active);active={};const auto& old=opening.observed;
 if(!old.actor||old.actor!=at.actor||old.map!=at.map||old.world!=at.world||old.npc_generation!=at.npc_generation||
    old.revision!=at.revision||!at.nearby||at.now<old.now||at.now>=opening.deadline||opening.inventory!=inventory::encode(owned)||
    request.opcode!=75||request.type!=3||request.receive_gold||request.receive.size()!=1||request.give.empty()||request.give.size()>16)return {};
 const auto receive=request.receive.front();const Recipe* recipe=nullptr;
 for(const auto& row:opening.recipes)if(row.reward.definition.id==receive.item){if(recipe)return {};recipe=&row;}
 if(!recipe||!valid_recipe(*recipe)||!receive.quantity||receive.quantity>recipe->maximum_quantity)return {};
 const auto gold=std::uint64_t(recipe->gold)*receive.quantity;if(gold>100000||gold!=request.give_gold||gold>owned.gold)return {};
 std::map<unsigned,std::uint64_t> remaining;for(auto m:recipe->materials)remaining[m.element]=std::uint64_t(m.units)*recipe->reward.definition.unknown2*receive.quantity;
 Plan plan{*recipe,receive.quantity,static_cast<unsigned>(gold),{},opening.inventory,at.revision};std::set<unsigned> selected;
 for(auto given:request.give){
  // Original5745f0 passes no give-quantity array; original8768c0 emits1
  // per selected STACK because receive quantity forces75. This is not debit1.
  if(given.quantity!=1||!selected.insert(given.item).second)return {};
  const auto found=owned.items.find(given.item);if(found==owned.items.end())return {};const auto& item=found->second;
  const auto bag=owned.bags.find(item.bag);const auto material=inventory::materials::element(item.definition);
  if(bag==owned.bags.end()||bag->second.type!=1||!material||!item.definition.quantity||item.definition.quantity>250||
     !(item.definition.interaction&0x80000)||(item.customized&&!item.customized->empty())||
     (item.profession&&*item.profession!=0&&*item.profession!=11)||item.reviewed_armor)return {};
  const auto need=remaining.find(material->element);if(need==remaining.end()||!need->second)return {};
  const auto debit=static_cast<unsigned>(std::min<std::uint64_t>(need->second,item.definition.quantity));
  need->second-=debit;plan.debits_.push_back({given.item,debit});
 }
 for(auto [element,need]:remaining)if(need)return {};
 return plan;
}
struct Proposal {inventory::Profile profile;inventory::Batch packets;};
inline std::optional<Proposal> propose(const Plan& plan,const inventory::Profile& current,std::uint64_t revision){
 inventory::validate_character(current);
 if(plan.revision_!=revision||plan.inventory_!=inventory::encode(current)||!plan.quantity_||plan.quantity_>plan.recipe_.maximum_quantity||
    !valid_recipe(plan.recipe_)||std::uint64_t(plan.recipe_.gold)*plan.quantity_!=plan.gold_||plan.gold_>current.gold)return {};
 collectors::Proposal out{current,{},0};std::map<unsigned,std::uint64_t> remaining;
 for(auto m:plan.recipe_.materials)remaining[m.element]=std::uint64_t(m.units)*plan.recipe_.reward.definition.unknown2*plan.quantity_;
 std::set<unsigned> debited;
 for(auto debit:plan.debits_){const auto found=out.profile.items.find(debit.item);
  if(found==out.profile.items.end()||!debit.quantity||debit.quantity>found->second.definition.quantity||!debited.insert(debit.item).second)return {};
  const auto material=inventory::materials::element(found->second.definition);if(!material)return {};
  const auto need=remaining.find(material->element);if(need==remaining.end()||debit.quantity!=std::min<std::uint64_t>(need->second,found->second.definition.quantity))return {};
  need->second-=debit.quantity;found->second.definition.quantity-=debit.quantity;
  if(found->second.definition.quantity)out.packets.push_back(items::quantity_2022(debit.item,found->second.definition.quantity));
  else{out.packets.push_back(items::detach_2022(current.inventory,debit.item));out.packets.push_back(items::delete_2022(debit.item));out.profile.items.erase(found);}
 }
 for(auto [element,need]:remaining)if(need)return {};
 collectors::Offer offer;offer.reward=plan.recipe_.reward;
 for(unsigned i=0;i<plan.quantity_;++i){auto before=out.profile;auto issued=collectors::issue_reward(before,offer,{},std::move(out));
  if(!issued||issued->reward>inventory::identity::owned_last)return {};out=std::move(*issued);}
 out.profile.gold-=plan.gold_;inventory::validate_character(out.profile);return Proposal{std::move(out.profile),std::move(out.packets)};
}
template<class Save>
std::optional<inventory::Batch> commit(const Plan& plan,inventory::identity::Session& identities,inventory::Profile& character,
 inventory::Profile& bank,std::uint64_t& revision,Save&& save){
 if(!revision||revision>=INT64_MAX)return {};auto proposed=propose(plan,character,revision);if(!proposed)return {};
 auto prepared=identities.prepare(proposed->profile,bank);if(!identities.preserves_bank(prepared))throw std::runtime_error("craft changed durable bank");
 static_assert(std::is_nothrow_move_assignable_v<inventory::Profile>);static_assert(std::is_nothrow_move_constructible_v<inventory::Batch>);
 auto packets=std::move(proposed->packets);const auto saved=save(prepared.durable.character,revision);if(!saved)return {};
 if(!identities.commit(std::move(prepared)))throw std::runtime_error("obsolete committed craft projection");
 character=std::move(prepared.wire.character);bank=std::move(prepared.wire.bank);revision=*saved;return std::move(packets);
}
}
