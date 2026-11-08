#pragma once
#include "gw_attack_admission.h"
#include "gw_dagger_dual_group_detached.h"
#include "gw_contact_cooldown_amendment.h"
namespace fw::gw::combat::weapon {
// World-owned retained physical second attack. Policy: first quoted contact
// duration is reused as inter-contact duration (500ms*IAS policy, rounding
// inherited from reviewed_attack_duration). No claim of exact retail ticks.
class DaggerSuccessfulSecondDraft {
 friend class fw::gw::world::Instance;
 const Scope source_,target_;const AttackPermit attempt_;
 DaggerDualGroupDraft group_;const std::uint64_t due_;
 const std::shared_ptr<skills::effects::ContactCooldownContinuation> continuation_;
 std::atomic_bool closed_{false};
 DaggerSuccessfulSecondDraft(Scope source,Scope target,const AttackPermit& attempt,
   const DaggerDualGroupDraft& group,std::uint64_t due,const std::shared_ptr<skills::effects::ContactCooldownContinuation>& continuation):
   source_(source),target_(target),attempt_(attempt),group_(group),due_(due),continuation_(continuation){}
public:
 DaggerSuccessfulSecondDraft(const DaggerSuccessfulSecondDraft&)=delete;
 std::uint64_t due()const{return due_;}
 Scope source()const{return source_;}Scope target()const{return target_;}
 skills::Slot slot()const{return continuation_->slot();}
 std::uint64_t skill_use()const{return continuation_->skill_use();}
 void cancel(){closed_.store(true);}bool open()const{return !closed_.load();}
};
}
