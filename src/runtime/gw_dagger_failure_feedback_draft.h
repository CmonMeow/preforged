#pragma once
#include "gw_attack_admission.h"
#include <optional>
#include <vector>
namespace fw::gw::combat::weapon {
// Original2022 opcode159/kind38: component7 on target; payload source,reason2.
// Resource0x1DB is proved. English localization/assets/rendering remain unknown.
inline std::optional<std::vector<std::uint8_t>> dagger_failure_wire_draft(Scope source,Scope target){
 if(!source.instance||source.instance!=target.instance||!source.generation||!target.generation||
    !source.actor||source.actor>65535||!target.actor||target.actor>65535||source.actor==target.actor)return {};
 std::vector<std::uint8_t> bytes{159,0};
 for(const std::uint32_t value:{38u,target.actor,source.actor,2u})
  for(unsigned shift=0;shift<32;shift+=8)bytes.push_back(static_cast<std::uint8_t>(value>>shift));
 return bytes;
}
//976 reuses the captured Fox/Spider family schema by approved STAGED inference.
// There is no direct fixed-checkpoint976 prerequisite-failure receipt or proved
// English localization. Actual World failure ownership remains mandatory.
inline bool dagger_single_failure_feedback_draft(unsigned skill){
 const auto role=dagger_role(skill);return role==780||role==778||role==976;
}
// Only a World-observed late prerequisite failure after successful claim may
// create this capability. No Energy/recharge/event authority is transferred.
// Single-phase Fox/Spider/Repeating; duals retain their separate phase owner.
class DaggerFailureFeedbackDraft {
 friend class fw::gw::world::Instance;
 const Scope source_,target_;const unsigned skill_,copy_;
 const std::weak_ptr<AttackAdmission> attempt_;std::atomic_bool consumed_{false};
 DaggerFailureFeedbackDraft(Scope source,Scope target,unsigned skill,unsigned copy,const AttackPermit& permit):
  source_(source),target_(target),skill_(skill),copy_(copy),attempt_(permit){}
public:
 DaggerFailureFeedbackDraft(const DaggerFailureFeedbackDraft&)=delete;
 std::optional<std::vector<std::uint8_t>> consume(Scope source,Scope target,unsigned skill,unsigned copy,const AttackPermit& permit){
  // Compare exact retained/current lifetimes and attempt identity before consume;
  // a foreign callback must not consume the legitimate pending cast's receipt.
  if(!dagger_single_failure_feedback_draft(skill_)||source!=source_||target!=target_||skill!=skill_||copy!=copy_||
     !permit||permit->open()||attempt_.lock()!=permit)return {};
  auto wire=dagger_failure_wire_draft(source,target);if(!wire||consumed_.exchange(true))return {};
  return wire;
 }
};
}
