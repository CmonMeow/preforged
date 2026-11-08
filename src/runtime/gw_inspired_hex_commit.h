#pragma once
#include "gw_world_simulation.h"
#include "gw_hex_copy_session.h"
#include "gw_hex_copy_ids.h"
#include "gw_temporary_skill_expiry.h"
namespace fw::gw::skills::effects {
struct InspiredHexCommit {std::optional<HexCopySlotChange> changed;};
// Called only by the owning connection thread. Preflight all temporary state
// before World removal; do not interleave session edits between these steps.
inline std::optional<InspiredHexCommit> commit_inspired_hex_copy(world::Instance& world,
 HexCopySession& session,HexCopyIds& ids,Scope caster,Scope ally,const InspiredHexPlan& plan,skills::Slot source,Time now){
 if(session.owner()!=caster||source.skill!=plan.skill)return {};
 const auto& bar=session.bar();const auto found=std::find(bar.begin(),bar.end(),source);
 if(found==bar.end())return {};
 const unsigned index=unsigned(found-bar.begin());
 const auto offer=world.prepare_inspired_hex(caster,ally,plan,now);if(!offer)return {};
 if(offer->copy.kind==HexCopyKind::player_pvp_alias)return {}; // Alias wire identity remains unverified.
 if(offer->copy.kind!=HexCopyKind::player){
  if(!world.commit_inspired_hex(*offer))return {};return InspiredHexCommit{};
 }
 auto next_ids=ids;
 const auto replacement=next_ids.allocate(caster,offer->copy.actual,bar,plan.removed_category);if(!replacement)return {};
 auto prepared=session.prepare(*offer,index,source,*replacement);
 if(!prepared||!session.ready(*prepared))return {};
 const auto result=world.commit_inspired_hex(prepared->offer(),SkillbarTransition{session.bar(),prepared->bar()});if(!result)return {};
 if(!session.commit(std::move(*prepared),*result))throw std::logic_error("serialized hex copy publication rejected");
 static_assert(std::is_nothrow_move_assignable_v<HexCopyIds>);
 ids=std::move(next_ids);
 return InspiredHexCommit{HexCopySlotChange{index,source,*replacement}};
}
// Prepare session state without mutation, publish the exact before/after pair
// under World authority, then perform the no-throw session move. The owning
// connection serializes all callers; World never calls into the session.
inline std::optional<std::vector<TemporaryHexRestoration>> expire_hex_copy(world::Instance& world,
 HexCopySession& session,Scope owner,Time now){
 auto result=expire_temporary_copy(world,session,owner,now);if(!result)return {};
 return std::move(result->changes);
}
inline std::optional<std::vector<HexCopySlotChange>> replace_hex_copy_base(world::Instance& world,
 HexCopySession& session,Scope owner,const skills::Slots& base,const skills::Policy& policy,Time now){
 static_assert(std::is_nothrow_move_assignable_v<HexCopySession>);
 auto next=session;const auto changes=next.replace_base(owner,base,policy,now);if(!changes)return {};
 const auto published=world.cooldown_bar_snapshot(owner);
 // Normal build updates may already have published the new base. Accept only
 // that exact bar or this session's previous bar, never an unrelated edit.
 if(!published||(published->bar!=session.bar()&&published->bar!=next.bar())||
    !world.publish_skillbar_transition(owner,{published->bar,next.bar()},now))return {};
 session=std::move(next);return changes;
}
}
