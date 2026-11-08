#pragma once
#include "gw_quest_charr_gate.h"
#include "gw_collector_exchange.h"
#include "gw_inventory_identity.h"
#include "gw_quest_wire.h"
#include <type_traits>
namespace fw::gw::quests::charr_gate {
// Original decoded354 packet552527, same310 placed552528, quest46 removed
//552529, XP200 at552534, new owned inventory23704. Source packet SHA256
//e210b492c044e475735ac55dca85944598fff445e120e657674df9d7fc4df55c.
inline items::Definition long_sword(unsigned identity){
 items::Definition d;d.id=identity;d.file=2147493102u;d.type=27;d.interaction=571473920;
 d.price=10;d.model=2978;d.quantity=1;d.encoded_name={9997,60200,53520,30681};
 d.modifiers={616038912,578302479,604504273,623903139,559415301,2760378630};return d;
}
struct Reward {Profile quests;inventory::Profile inventory;inventory::Batch packets;unsigned item=0,xp=experience;};
inline std::optional<Reward> reward(const Profile& current,const inventory::Profile& owned,unsigned cycle,const Eligibility& e,const Interaction& npc){
 validate(current);inventory::validate_character(owned);
 const auto found=current.records.find(id);
 if(!eligible(e)||!matches(npc,146)||found==current.records.end()||found->second.cycle!=cycle||
    found->second.stage!=Stage::completed||!valid_progress(found->second))return {};
 const auto quests=advance(current,id,cycle,Stage::completed,Stage::rewarded);if(!quests)return {};
 collectors::Offer offer;offer.reward.definition=long_sword(1);
 // This older capture establishes no equipment metadata. Preserve unknown,
 // rather than silently fabricating customization/profession to allow equip.
 const auto issued=collectors::issue_reward(owned,offer,{},collectors::Proposal{owned,{},0});if(!issued)return {};
 if(issued->reward>inventory::identity::owned_last)return {};
 auto packets=issued->packets;packets.push_back(remove_2022(id));
 return Reward{*quests,issued->profile,std::move(packets),issued->reward,experience};
}
struct Revisions {std::uint64_t quests=0,inventory=0;bool operator==(const Revisions&)const=default;};
struct Publication {inventory::Batch packets;unsigned xp=experience;};
// Save MUST persist proposed quests, durable inventory and XP200 together under
// both expected revisions. Nothing publishes or mutates session state on CAS
// failure. Caller uses existing progression publication after durable success;
// no guessed completion packet or XP total is generated here.
template<class Save>
std::optional<Publication> commit(inventory::identity::Session& identities,Profile& quests,inventory::Profile& character,
 inventory::Profile& bank,Revisions& revisions,unsigned cycle,const Eligibility& e,const Interaction& npc,Save&& save){
 if(!revisions.quests||!revisions.inventory||revisions.quests>=INT64_MAX||revisions.inventory>=INT64_MAX)return {};
 auto proposed=reward(quests,character,cycle,e,npc);if(!proposed)return {};
 auto prepared=identities.prepare(proposed->inventory,bank);
 if(!identities.preserves_bank(prepared))throw std::runtime_error("quest46 reward changed durable bank");
 // Finish all allocation before durable success. Default-allocator profile
 // moves and packet moves below are statically required to be nonthrowing.
 static_assert(std::is_nothrow_move_assignable_v<Profile>);
 static_assert(std::is_nothrow_move_assignable_v<inventory::Profile>);
 static_assert(std::is_nothrow_move_constructible_v<Publication>);
 Publication publication{std::move(proposed->packets),experience};
 const auto saved=save(proposed->quests,prepared.durable.character,experience,revisions);if(!saved)return {};
 if(!identities.commit(std::move(prepared)))throw std::runtime_error("obsolete committed quest46 identity projection");
 quests=std::move(proposed->quests);character=std::move(prepared.wire.character);bank=std::move(prepared.wire.bank);revisions=*saved;
 return std::move(publication);
}
}
