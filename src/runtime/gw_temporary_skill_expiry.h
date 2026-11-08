#pragma once
#include "gw_world_simulation.h"
#include "gw_temporary_skill_session.h"
namespace fw::gw::skills::effects {
struct TemporaryCopyExpiry {
 std::vector<TemporarySkillRestoration> changes;
 CopyRestorationReceipt receipt;
};
// The owner thread publishes World membership/cooldown authority first, then
// installs the already-prepared session state with a no-throw move.
inline std::optional<TemporaryCopyExpiry> expire_temporary_copy(world::Instance& world,
 TemporarySkillSession& session,Scope owner,Time now){
 static_assert(std::is_nothrow_move_assignable_v<TemporarySkillSession>);
 static_assert(std::is_nothrow_move_constructible_v<CopyRestorationReceipt>);
 auto next=session;auto changes=next.expire(owner,now);if(!changes)return {};
 auto receipt=world.restore_temporary_skill_slots(owner,{session.bar(),next.bar()},*changes,now);if(!receipt)return {};
 session=std::move(next);return TemporaryCopyExpiry{std::move(*changes),std::move(*receipt)};
}
}
