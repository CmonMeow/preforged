#pragma once
#include "gw_diversion_authority.h"
namespace fw::gw::skills::effects {
// Trusted pre-effect snapshot, not a token accepted from client input. World
// must own the stored snapshot and classify the actual skill-use event.
struct PreparedDiversionUse {DiversionUse use;Time at=0;std::optional<Entry> hex;};
inline std::optional<PreparedDiversionUse> capture_diversion_use(const Registry& registry,
 const DiversionUse& use,Time now){
 if(!registry.matches(use.owner)||!use.event||!use.skill_used||!use.slot.skill||!find_skill(use.slot.skill))return {};
 const auto hex=registry.newest_skill(use.owner,30,now);
 if(hex&&!diversion_use(registry,use,now))return {};
 return PreparedDiversionUse{use,now,hex};
}
struct FinalizedDiversionUse {
 Registry registry;DiversionAuthority authority;
 std::optional<unsigned> removed_instance;
 std::optional<Time> recharge_until;
};
inline std::optional<FinalizedDiversionUse> finalize_diversion_use(const Registry& current,
 const DiversionAuthority& original,const PreparedDiversionUse& prepared,Time now,Time base,Time existing){
 if(now!=prepared.at||!current.matches(prepared.use.owner))return {};
 Time additional=0;
 if(prepared.hex){
  const auto& h=*prepared.hex;const auto plan=diversion(h.rank);
  if(!plan||h.skill!=30||h.kind!=Category::hex||!h.instance||!h.cause||h.cause==prepared.use.owner.actor||
     h.applied>now||!h.expires||now>=*h.expires)return {};
  if(!prepared.use.instant_recharge&&prepared.use.slot.skill!=22&&prepared.use.slot.skill!=1049)additional=plan->additional_recharge;
 }
 const auto deadline=diversion_recharge_deadline(now,base,additional,existing);if(!deadline)return {};
 auto authority=original;const auto extension=additional?deadline:std::nullopt;
 if(!authority.record(prepared.use,now,extension))return {};
 auto registry=current;std::optional<unsigned> removed;
 if(prepared.hex){const auto& h=*prepared.hex;
  if(const auto present=current.lookup(prepared.use.owner,h.instance)){
   // An effect instance reused for a different record cannot be consumed.
   if(present->cause!=h.cause||present->skill!=h.skill||present->rank!=h.rank||present->kind!=h.kind||
      present->applied!=h.applied||present->expires!=h.expires)return {};
   if(!registry.remove(prepared.use.owner,h.instance))return {};removed=h.instance;
  }
 }
 return FinalizedDiversionUse{std::move(registry),std::move(authority),removed,extension};
}
}
