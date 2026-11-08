#pragma once
#include "gw_dagger_dual_failure_feedback_draft.h"
#include "gw_attack_outcomes.h"
#include "gw_dagger_successful_second_draft.h"
#include <functional>
namespace fw::gw::combat::weapon {
inline bool dagger_paid_id_draft(unsigned id){return dagger_role(id)!=0;}
struct DaggerPaidCompletionDraft {
    std::shared_ptr<DaggerSuccessfulSecondDraft> successful_second;
    bool retry=false;
    std::optional<skills::effects::CooldownAmendment> amendment;
    bool accepted=false,prerequisite_failed=false,reset_recharge=false;
    std::vector<ResolvedAttack> outcomes;
    std::function<void()> publication;
    std::shared_ptr<DaggerDualFailureFeedbackDraft> dual_failure_feedback;
    std::shared_ptr<DaggerFailureFeedbackDraft> failure_feedback; // narrow World-issued Fox/Spider only
};
}
