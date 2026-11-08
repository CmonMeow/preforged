#pragma once
#include "gw_dagger_failure_feedback_draft.h"
namespace fw::gw::combat::weapon {
// Private policy500ms: captured transport gaps469/500/516/563ms support order
// and separated phases, not an exact retail simulation interval or IAS rule.
// This class owns failure feedback phases, NEVER AttackOutcomes or skill costs.
class DaggerDualFailureFeedbackDraft {
 friend class fw::gw::world::Instance;
 const Scope source_,target_;const unsigned skill_,copy_;
 const AttackPermit attempt_;const std::uint64_t first_at_,second_at_;
 std::atomic<unsigned> phases_{0};std::atomic<unsigned> releases_{0};
 DaggerDualFailureFeedbackDraft(Scope source,Scope target,unsigned skill,unsigned copy,
        const AttackPermit& attempt,std::uint64_t now):source_(source),target_(target),
        skill_(skill),copy_(copy),attempt_(attempt),first_at_(now),second_at_(now+500){}
public:
 DaggerDualFailureFeedbackDraft(const DaggerDualFailureFeedbackDraft&)=delete;
 Scope source()const{return source_;}
 std::uint64_t second_due()const{return second_at_;}
 bool physical_open()const{return releases_.load()!=3;}
 std::optional<std::vector<std::uint8_t>> consume(unsigned phase,Scope source,Scope target,unsigned skill,unsigned copy,
        const AttackPermit& attempt,std::uint64_t now){
  if(phase>1||!dagger_dual_role(skill_)||source!=source_||target!=target_||skill!=skill_||copy!=copy_||
     !attempt||attempt!=attempt_||attempt->open()||now<(phase?second_at_:first_at_))return {};
  if(releases_.load()<(phase?3u:1u))return {};
  auto wire=dagger_failure_wire_draft(source,target);if(!wire)return {};
  unsigned expected=phase?1u:0u;
  if(!phases_.compare_exchange_strong(expected,phase?3u:1u))return {};
  return wire;
 }
 void cancel(){phases_.store(3);releases_.store(3);}
};
}
