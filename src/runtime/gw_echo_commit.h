#pragma once
#include "gw_world_simulation.h"
#include "gw_temporary_skill_session.h"
#include "gw_temporary_skill_ids.h"
namespace fw::gw::skills::effects {
struct EchoCommit {
 std::optional<TemporarySkillSlotChange> changed;
 std::vector<skills::Packet> packets;
};
// Owning connection thread only. World never invokes session callbacks, and
// the caller must not edit the session between prepare/ready/commit.
inline std::optional<EchoCommit> commit_echo_copy(world::Instance& world,
 TemporarySkillSession& session,TemporarySkillIds& ids,Scope owner,skills::Slot source,skills::Slot used,Time now,std::uint64_t completion_event=0){
 if(session.owner()!=owner)return {};
 const auto before=session.bar();const auto found=std::find(before.begin(),before.end(),source);
 if(found==before.end())return {};
 const auto offer=world.prepare_echo_use(owner,source,used,now,completion_event);if(!offer)return {};
 auto next_ids=ids;auto replacement=source;
 if(offer->copied_skill){
  const auto allocated=next_ids.allocate_echo(owner,*offer,before);if(!allocated)return {};
  replacement=*allocated;
 }
 const auto index=unsigned(found-before.begin());
 auto prepared=session.prepare_echo(*offer,index,replacement);
 if(!prepared||!session.ready(*prepared))return {};
 EchoCommit output;
 const SkillbarTransition transition{before,prepared->bar()};
 if(offer->copied_skill){
  output.changed=TemporarySkillSlotChange{index,source,replacement};
  output.packets.push_back(skills::slot_packet_2022(owner.actor,index,replacement));
 }
 const auto receipt=world.commit_echo_use(owner,prepared->proposal(),transition);if(!receipt)return {};
 if(!session.commit_echo(std::move(*prepared),*receipt))throw std::logic_error("serialized Echo session publication rejected");
 static_assert(std::is_nothrow_move_assignable_v<TemporarySkillIds>);
 ids=std::move(next_ids);return output;
}
struct EchoBatchCommit {std::vector<TemporarySkillSlotChange> changes;std::vector<skills::Packet> packets;};
inline std::optional<EchoBatchCommit> commit_echo_copies(world::Instance& world,
 TemporarySkillSession& session,TemporarySkillIds& ids,Scope owner,const std::vector<skills::Slot>& sources,
 skills::Slot used,Time now,std::uint64_t completion_event){
 if(session.owner()!=owner||sources.empty()||sources.size()>2)return {};
 auto next_ids=ids;auto allocation_bar=session.bar();
 std::vector<EchoUseProposal> offers;std::vector<skills::Slot> replacements;EchoBatchCommit output;
 for(const auto source:sources){
  const auto found=std::find(session.bar().begin(),session.bar().end(),source);if(found==session.bar().end())return {};
  const auto offer=world.prepare_echo_use(owner,source,used,now,completion_event);if(!offer)return {};
  auto replacement=source;const auto index=unsigned(found-session.bar().begin());
  if(offer->copied_skill){
   const auto allocated=next_ids.allocate_echo(owner,*offer,allocation_bar);if(!allocated)return {};
   replacement=*allocated;allocation_bar[index]=replacement;
   output.changes.push_back({index,source,replacement});output.packets.push_back(skills::slot_packet_2022(owner.actor,index,replacement));
  }
  offers.push_back(*offer);replacements.push_back(replacement);
 }
 auto prepared=session.prepare_echo_batch(offers,replacements);if(!prepared||!session.ready(*prepared))return {};
 const auto receipts=world.commit_echo_uses(owner,prepared->proposals(),SkillbarTransition{session.bar(),prepared->bar()});if(!receipts)return {};
 if(!session.commit_echo_batch(std::move(*prepared),*receipts))throw std::logic_error("serialized Echo batch publication rejected");
 ids=std::move(next_ids);return output;
}

// Trusted Runtime completion observer. Discover only currently armed slots;
// callers never supply the source list from a client request.
inline std::optional<EchoBatchCommit> observe_echo_completion(world::Instance& world,
 TemporarySkillSession& session,TemporarySkillIds& ids,Scope owner,const CooldownCompletion& completion){
 if(session.owner()!=owner||completion.stamp.owner!=owner||!completion.skill_use)return {};
 const auto recorded=world.cooldown_completion_stamp(owner,completion.skill_use);
 const auto bar=world.cooldown_bar_snapshot(owner);
 // A completed copy skill may already have replaced its own used slot.
 // Its retained finalized receipt, rather than current membership, proves use.
 if(!recorded||*recorded!=completion||!bar||bar->bar!=session.bar()||bar->published.at>completion.stamp.at)return {};
 std::vector<skills::Slot> sources;
 for(const auto source:session.bar())if(echo(source.skill)&&
    world.prepare_echo_use(owner,source,completion.slot,completion.stamp.at,completion.skill_use))sources.push_back(source);
 if(sources.empty())return EchoBatchCommit{};
 return commit_echo_copies(world,session,ids,owner,sources,completion.slot,completion.stamp.at,completion.skill_use);
}

}
