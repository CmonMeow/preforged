// NEW candidate inside Instance. Failed prerequisites share actual attack release
// ownership with existing SkillHit::skill_failed, then finish cancelled/unqualified.
private:
    std::optional<combat::weapon::ResolvedAttack> release_dagger_failed_attempt_draft_locked(
            skills::effects::Scope source,skills::effects::Scope target,unsigned skill,std::uint64_t now,bool second){
        if(!player_empathy_reaction_ready_locked(source,now))return {};
        const auto event=begin_attack_outcome_locked(source,target,now,combat::weapon::AttackDelivery::melee,0,!second);
        if(!event)return {};
        if(!player_empathy_release_locked(source,target,event,now)){
            const auto actor=players_.find(source.actor);
            if(actor!=players_.end()&&actor->second.effect_scope==source&&actor->second.precise_health>0){
                finish_attack_outcome_locked(source,target,event,AttackOutcome::cancelled,now);return {};
            }
            // Lethal reaction follows an actual failed attack; preserve its
            // typed terminal and paid acceptance instead of rolling it back.
        }
        // Empathy oldwiki explicitly says Vigorous does NOT trigger on skill failure.
        finish_attack_outcome_locked(source,target,event,AttackOutcome::cancelled,now,true);
        std::optional<combat::weapon::ResolvedAttack> outcome;
        for(auto it=resolved_attacks_.rbegin();it!=resolved_attacks_.rend();++it)
            if(it->attacker==source&&it->event==event){outcome=*it;break;}
        // Cancellation here is a typed prerequisite failure, not miss/block.
        // Never qualify, contact damage, grant chain, IW damage, or critical Energy.
        if(!second)ordinary_spiteful_release_locked(source,now,skill);
        return outcome;
    }
public:
    combat::weapon::DaggerPaidCompletionDraft complete_dagger_failed_second_draft(
            const std::shared_ptr<combat::weapon::DaggerDualFailureFeedbackDraft>& phase,std::uint64_t now){
        if(!phase||now<phase->second_at_||now-phase->second_at_>90000)return {};
        std::lock_guard lock(mutex_);DamagePublication pending;begin_damage_publication_locked(pending);
        combat::weapon::DaggerPaidCompletionDraft result;
        const auto perform=[&]{
            const auto actor=players_.find(phase->source_.actor);
            if(actor==players_.end()||actor->second.effect_scope!=phase->source_||
               actor->second.control_epoch!=phase->attempt_->control_||actor->second.attack_interrupt_epoch!=phase->attempt_->interrupt_||
               !settle_timed_effects_locked(now)||!dagger_dual_participants_draft_locked(phase->source_,phase->target_,phase->attempt_->weapon_,now))return;
            // Reacquire after settlement: a queued control can end before now
            // while still interrupting this retained physical attempt.
            const auto current=players_.find(phase->source_.actor);
            if(current==players_.end()||current->second.effect_scope!=phase->source_||
               current->second.control_epoch!=phase->attempt_->control_||
               current->second.attack_interrupt_epoch!=phase->attempt_->interrupt_)return;
            unsigned expected=1;if(!phase->releases_.compare_exchange_strong(expected,3))return;
            if(const auto event=release_dagger_failed_attempt_draft_locked(phase->source_,phase->target_,phase->skill_,now,true)){
                result.accepted=true;result.prerequisite_failed=true;result.outcomes.push_back(*event);
            }
        };perform();result.publication=finish_damage_publication_locked(pending);return result;
    }
