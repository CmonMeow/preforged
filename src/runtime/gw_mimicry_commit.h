#pragma once
#include "gw_world_simulation.h"
#include "gw_temporary_skill_session.h"
#include "gw_temporary_skill_ids.h"
namespace fw::gw::skills::effects {
struct MimicryCopyCommit {TemporarySkillSlotChange change;std::vector<skills::Packet> packets;};
// Owning session thread only. World never invokes a session callback; prepared
// allocation is discarded if either participant's bar/lifetime became stale.
inline std::optional<MimicryCopyCommit> commit_mimicry_copy(world::Instance& world,TemporarySkillSession& session,
        TemporarySkillIds& ids,const MimicryDonorLease& lease){
    if(session.owner()!=lease.owner()||session.bar()!=lease.owner_bar().bar)return {};
    const auto found=std::find(session.bar().begin(),session.bar().end(),lease.source());if(found==session.bar().end())return {};
    auto proposed_ids=ids;const auto replacement=proposed_ids.allocate_mimicry(lease.owner(),lease,session.bar());
    if(!replacement)return {};
    const auto index=unsigned(found-session.bar().begin());auto prepared=session.prepare_mimicry(lease,index,*replacement);
    if(!prepared||!session.ready(*prepared))return {};
    MimicryCopyCommit result{{index,lease.source(),*replacement},
        {skills::slot_packet_2022(lease.owner().actor,index,*replacement)}};
    const auto receipt=world.commit_mimicry_copy(lease,{session.bar(),prepared->bar()},*replacement);if(!receipt)return {};
    if(!session.commit_mimicry(std::move(*prepared),*receipt))throw std::logic_error("serialized Mimicry session publication rejected");
    static_assert(std::is_nothrow_move_assignable_v<TemporarySkillIds>);
    ids=std::move(proposed_ids);return result;
}
}
