#pragma once
#include "gw_arthur_crafter_catalog.h"
#include "gw_equipment_metadata_provider.h"
#include <type_traits>
namespace fw::gw::items::customization {
using Observation=collectors::native::Observation;
inline constexpr unsigned fee=10;
// Deliberately bounded to four exact known Arthur primary weapons. This is
// not a universal customization eligibility classifier or native decoder.
inline bool eligible(const inventory::Profile& profile,const inventory::Item& item){
 const auto bag=profile.bags.find(item.bag);
 if(bag==profile.bags.end()||bag->second.type!=1||item.profession!=0||!item.customized||!item.customized->empty()||item.reviewed_armor)return false;
 for(const auto& row:crafter::arthur_wiki_catalog()){
  if(row.reward.definition.type==12)continue;auto expected=row.reward.definition;expected.id=item.definition.id;
  if(definition_2022(expected)==definition_2022(item.definition))return true;
 }return false;
}
struct Binding {Observation at;std::uint64_t deadline=0;Packet snapshot;std::u16string owner;};
inline bool bind(Binding& active,const Observation& at,std::string_view npc,const inventory::Profile& owned,std::u16string_view owner){
 active={};
 if(at.map!=148||(npc!="Elias"&&npc!="Elias [Weapons]")||!at.actor||!at.world||!at.npc_generation||!at.revision||!at.nearby||
    at.now>UINT64_MAX-120000||owner.empty()||owner.size()>19||!equipment::metadata::utf8(owner))return false;
 inventory::validate_character(owned);active={at,at.now+120000,inventory::encode(owned),std::u16string(owner)};return true;
}
struct Proposal;
class Plan {
 unsigned item_;std::uint64_t revision_;Packet snapshot_;std::u16string owner_;
 Plan(unsigned item,std::uint64_t revision,Packet snapshot,std::u16string owner):item_(item),revision_(revision),snapshot_(std::move(snapshot)),owner_(std::move(owner)){}
 friend std::optional<Plan> authorize(Binding&,unsigned,const inventory::Profile&,const Observation&);
 friend std::optional<Proposal> propose(const Plan&,const inventory::Profile&,std::uint64_t);
};
inline std::optional<Plan> authorize(Binding& active,unsigned item,const inventory::Profile& current,const Observation& now){
 auto opening=std::move(active);active={};const auto& at=opening.at;
 if(!at.actor||at.actor!=now.actor||at.map!=now.map||at.world!=now.world||at.npc_generation!=now.npc_generation||
    at.revision!=now.revision||!now.nearby||now.now<at.now||now.now>=opening.deadline||opening.snapshot!=inventory::encode(current)||current.gold<fee)return {};
 const auto found=current.items.find(item);if(found==current.items.end()||!eligible(current,found->second))return {};
 return Plan{item,now.revision,std::move(opening.snapshot),std::move(opening.owner)};
}
struct Proposal {inventory::Profile profile;inventory::Batch packets;};
inline std::optional<Proposal> propose(const Plan& plan,const inventory::Profile& current,std::uint64_t revision){
 if(revision!=plan.revision_||inventory::encode(current)!=plan.snapshot_||current.gold<fee)return {};
 const auto found=current.items.find(plan.item_);if(found==current.items.end()||!eligible(current,found->second))return {};
 Proposal result{current,{customization_2022(plan.item_,plan.owner_)}};
 result.profile.items.at(plan.item_).customized=plan.owner_;result.profile.gold-=fee;inventory::validate_character(result.profile);return result;
}
template<class Save>std::optional<inventory::Batch> commit(const Plan& plan,inventory::identity::Session& identities,
 inventory::Profile& character,inventory::Profile& bank,std::uint64_t& revision,Save&& save){
 if(!revision||revision>=INT64_MAX)return {};auto result=propose(plan,character,revision);if(!result)return {};
 auto prepared=identities.prepare(result->profile,bank);if(!identities.preserves_bank(prepared))throw std::runtime_error("customization changed bank");
 static_assert(std::is_nothrow_move_assignable_v<inventory::Profile>);static_assert(std::is_nothrow_move_constructible_v<inventory::Batch>);
 auto packets=std::move(result->packets);const auto saved=save(prepared.durable.character,revision);if(!saved)return {};
 if(!identities.commit(std::move(prepared)))throw std::runtime_error("obsolete committed customization projection");
 character=std::move(prepared.wire.character);bank=std::move(prepared.wire.bank);revision=*saved;return std::move(packets);
}
}
