#include "gw_world_dagger_failed_attempt_draft.inl"
private:
    std::map<skills::effects::Scope,std::weak_ptr<combat::weapon::DaggerDualFailureFeedbackDraft>> dagger_paid_failed_second_draft_;
    std::map<skills::effects::Scope,std::weak_ptr<combat::weapon::AttackAdmission>> dagger_paid_attempts_draft_;
    void retire_dagger_paid_owner_draft_locked(skills::effects::Scope owner){
        if(const auto attempt=dagger_paid_attempts_draft_.find(owner);attempt!=dagger_paid_attempts_draft_.end()){
            if(const auto retained=attempt->second.lock())retained->cancel();dagger_paid_attempts_draft_.erase(attempt);
        }
        if(const auto phase=dagger_successful_seconds_draft_.find(owner);phase!=dagger_successful_seconds_draft_.end()){
            if(const auto retained=phase->second.lock())retained->cancel();dagger_successful_seconds_draft_.erase(phase);
        }
        if(const auto phase=dagger_paid_failed_second_draft_.find(owner);phase!=dagger_paid_failed_second_draft_.end()){
            if(const auto retained=phase->second.lock())retained->cancel();dagger_paid_failed_second_draft_.erase(phase);
        }
    }
    bool dagger_paid_predicate_draft_locked(skills::effects::Scope source,skills::effects::Scope target,
            const combat::weapon::Owned& weapon,const combat::weapon::AttackPlan& plan,std::uint64_t now){
        const auto actor=players_.find(source.actor);
        if(actor==players_.end()||actor->second.effect_scope!=source)return false;
        if(const auto policy=combat::weapon::dagger_attack_policy(plan.skill,plan.rank))
            return combat::weapon::dagger_attack_eligible(*policy,actor->second.dagger_chain,source,target,weapon.item_type,weapon.caster_weapon,now);
        if(dagger_dual_skill_draft(plan.skill)||plan.skill==976)return actor->second.dagger_chain.current(target,now)==2;
        if(plan.skill==778){
            if(const auto p=players_.find(target.actor);p!=players_.end())return p->second.effect_scope==target&&p->second.knocked_down_until>now;
            const auto n=npc_combat_.find(target.actor);return n!=npc_combat_.end()&&n->second.hex_scope()==target&&n->second.knocked_down(n->second.lifetime(),now);
        }
        return plan.skill==779||plan.skill==783;
    }
public:
    // Authenticated staged capability only. Prepare happens before Runtime
    // Energy payment. No chain/event/resource reservation or payment is faked.
    combat::weapon::AttackPermit prepare_dagger_cast_draft(skills::effects::Scope source,skills::effects::Scope target,
            const combat::weapon::Owned& weapon,const combat::weapon::AttackPlan& plan,
            std::uint64_t now,std::uint64_t release){
        if(!combat::weapon::dagger_paid_id_draft(plan.skill)||!combat::weapon::valid(plan,true)||
           release<=now||release-now>90000||weapon.item_type!=32||weapon.caster_weapon)return {};
        std::lock_guard lock(mutex_);
        if(holding_bundle_locked(source)||!settle_timed_effects_locked(now)||
           !dagger_dual_participants_draft_locked(source,target,weapon,now))return {};
        if(const auto old=dagger_successful_seconds_draft_.find(source);old!=dagger_successful_seconds_draft_.end())
            if(const auto phase=old->second.lock();phase&&phase->open())return {};
        if(const auto old=dagger_paid_failed_second_draft_.find(source);old!=dagger_paid_failed_second_draft_.end())
            if(const auto phase=old->second.lock();phase&&phase->physical_open())return {};
        const auto actor=players_.find(source.actor);if(actor==players_.end()||actor->second.effect_scope!=source)return {};
        if(!actor->second.effect_registry)return {};
        const auto active=actor->second.effect_registry->active(source,now);
        const auto dwarven=dwarven_bonuses_locked(actor->second,now);if(!dwarven)return {};
        const auto duration=skills::effects::reviewed_attack_duration(plan.windup_ms,
            dwarven->haste||skills::effects::tiger_stance_active(active,now)||skills::effects::hekets_rampage_active(active,now)||
            skills::effects::frenzy_active(active,now)||skills::effects::flurry_active(active,now)||
            skills::effects::lightning_reflexes_active(active,now)||skills::effects::flail_active(active,now),
            skills::effects::faintheartedness_active(active,now),skills::effects::bestial_fury_active(active,now));
        if(!duration||!*duration||release-now!=*duration)return {}; // independent World quote validation
        if(const auto prior=dagger_paid_attempts_draft_.find(source);prior!=dagger_paid_attempts_draft_.end())
            if(const auto pending=prior->second.lock();pending&&pending->open())return {};
        const bool illusionary=illusionary_attack_locked(actor->second,weapon,now).has_value();
        if(!illusionary&&plan.requires_critical_authority&&!critical_attack_ready_locked(source,target,weapon,now))return {};
        if(dagger_paid_predicate_draft_locked(source,target,weapon,plan,now)){
        if(const auto policy=combat::weapon::dagger_attack_policy(plan.skill,plan.rank)){
            if(!illusionary&&!dagger_secondary_ready_draft_locked(source,target,*policy,now))return {};
        }else if(const auto family=combat::weapon::detached_dagger_family_plan(plan.skill,plan.rank)){
            // Admission preview uses an explicit current stage snapshot; actual
            // dual events still require the real group owner at completion.
            const auto snapshot=actor->second.dagger_chain;
            if(!prepare_dagger_family_draft_locked(source,target,*family,now,illusionary,&snapshot))return {};
        }else return {};
        } // Missing prerequisite is a real paid failed attempt, per Dagger_attack revision2662681.
        const auto current=players_.find(source.actor);if(current==players_.end()||current->second.effect_scope!=source||
            current->second.precise_health<=0||!current->second.ordinary_weapon||!combat::weapon::same_weapon(*current->second.ordinary_weapon,weapon))return {};
        auto permit=combat::weapon::AttackPermit(new combat::weapon::AttackAdmission(source,target,weapon,plan,now,release,
            current->second.control_epoch,current->second.attack_interrupt_epoch,false));
        dagger_paid_attempts_draft_.insert_or_assign(source,permit);return permit;
    }
    combat::weapon::DaggerPaidCompletionDraft complete_dagger_cast_draft(skills::effects::Scope source,
            skills::effects::Scope target,const combat::weapon::Owned& weapon,const combat::weapon::AttackPlan& plan,
            const combat::weapon::AttackPermit& permit,std::uint64_t now,unsigned copy=0){
        using Result=combat::weapon::DaggerPaidCompletionDraft;
        if(copy!=plan.copy||now>UINT64_MAX-500||!permit||!combat::weapon::dagger_paid_id_draft(plan.skill)||!combat::weapon::valid(plan,true))return {};
        std::lock_guard lock(mutex_);DamagePublication pending;begin_damage_publication_locked(pending);
        const auto perform=[&]()->Result{
            const auto actor=players_.find(source.actor);const auto owned=dagger_paid_attempts_draft_.find(source);
            //976 must join existing owner-issued skill use before real release.
            if(plan.skill==976){const auto use=pending_diversion_uses_.find(source);
                if(use==pending_diversion_uses_.end()||use->second.prepared.at!=now||
                   use->second.prepared.use.slot!=skills::Slot{976,plan.copy}||use->second.repeating_strike_extra)return {};}

            if(actor==players_.end()||actor->second.effect_scope!=source||owned==dagger_paid_attempts_draft_.end()||owned->second.lock()!=permit||
               !permit->claim(source,target,weapon,plan,now,actor->second.control_epoch,actor->second.attack_interrupt_epoch))return {};
            if(!settle_timed_effects_locked(now)||!dagger_dual_participants_draft_locked(source,target,weapon,now))return {};
            // Timed control may have expired by this coarse callback. Exact
            // retained epochs still invalidate the original paid admission.
            const auto current=players_.find(source.actor);
            if(current==players_.end()||current->second.effect_scope!=source||
               current->second.control_epoch!=permit->control_||current->second.attack_interrupt_epoch!=permit->interrupt_)return {};
            if(!dagger_paid_predicate_draft_locked(source,target,weapon,plan,now)){
                Result failed;failed.accepted=true;failed.prerequisite_failed=true;
                failed.reset_recharge=true; // Wiki prerequisite failure; miss/block retain ordinary recharge
                if(combat::weapon::dagger_single_failure_feedback_draft(plan.skill))
                    failed.failure_feedback=std::shared_ptr<combat::weapon::DaggerFailureFeedbackDraft>(
                        new combat::weapon::DaggerFailureFeedbackDraft(source,target,plan.skill,copy,permit));
                if(dagger_dual_skill_draft(plan.skill))
                    failed.dual_failure_feedback=std::shared_ptr<combat::weapon::DaggerDualFailureFeedbackDraft>(
                        new combat::weapon::DaggerDualFailureFeedbackDraft(source,target,plan.skill,copy,permit,now));
                if(failed.dual_failure_feedback){
                    failed.dual_failure_feedback->releases_.store(1);
                    dagger_paid_failed_second_draft_.insert_or_assign(source,failed.dual_failure_feedback);
                }
                if(const auto event=release_dagger_failed_attempt_draft_locked(source,target,plan.skill,now,false))
                    failed.outcomes.push_back(*event);
                else{
                    if(failed.dual_failure_feedback)failed.dual_failure_feedback->cancel();
                    failed.dual_failure_feedback.reset();failed.failure_feedback.reset();failed.accepted=false;
                }
                return failed; // real cancelled/unqualified attack terminal, no fabricated hit
            }
            const auto first_event=next_attack_outcome_;
            std::shared_ptr<combat::weapon::DaggerSuccessfulSecondDraft> successful_second;
            bool dual_first_physical=false;
            if(dagger_dual_skill_draft(plan.skill)){
                const auto pending_use=pending_diversion_uses_.find(source);
                if(pending_use==pending_diversion_uses_.end())return {};
                const auto continuation=retain_contact_cooldown_locked(source,{plan.skill,plan.copy},pending_use->second.prepared.use.event,now);
                if(!continuation)return {};
                dual_first_physical=dagger_dual_attack_draft_locked(source,target,weapon,plan,now,permit,continuation,&successful_second);
            }
            else melee_attack_locked(source,target,weapon,plan,now,{},true);
            Result result;result.successful_second=std::move(successful_second);
            for(const auto& outcome:resolved_attacks_)
                if(outcome.attacker==source&&outcome.target==target&&outcome.event>=first_event&&outcome.event<next_attack_outcome_)
                    result.outcomes.push_back(outcome);
            if(plan.skill==976&&std::any_of(result.outcomes.begin(),result.outcomes.end(),[](const auto& outcome){return !outcome.skill_failed&&outcome.outcome==AttackOutcome::miss;})){
                const auto use=pending_diversion_uses_.find(source);
                if(use!=pending_diversion_uses_.end()&&use->second.prepared.at==now&&use->second.prepared.use.slot==skills::Slot{976,plan.copy})
                    use->second.repeating_strike_extra=15000;
            }
            // Do not report all-or-nothing refusal after a real first strike or
            // pre-damage cancellation already committed by the owned ledger.
            result.accepted=dagger_dual_skill_draft(plan.skill)?dual_first_physical&&!result.outcomes.empty():
                std::any_of(result.outcomes.begin(),result.outcomes.end(),[](const auto& outcome){
                return outcome.outcome!=AttackOutcome::cancelled&&outcome.outcome!=AttackOutcome::target_retired;
            });return result;
        };
        auto result=perform();auto publication=finish_damage_publication_locked(pending);
        if(result.accepted)result.publication=std::move(publication);
        else if(publication)publication(); // preserve authoritative refusal settlement
        return result;
    }

public:
    combat::weapon::DaggerPaidCompletionDraft complete_dagger_successful_second_draft(
            const std::shared_ptr<combat::weapon::DaggerSuccessfulSecondDraft>& phase,
            const skills::effects::LocalCooldownSnapshot& before,std::uint64_t now){
        using Result=combat::weapon::DaggerPaidCompletionDraft;
        if(!phase||!phase->open()||now<phase->due_||now-phase->due_>90000)return {};
        if(!publish_local_cooldowns(phase->source_,before)){Result retry;retry.retry=true;return retry;}
        std::lock_guard lock(mutex_);DamagePublication pending;begin_damage_publication_locked(pending);
        Result result;bool owned_phase=false;
        const auto perform=[&]{
            const auto tracked=dagger_successful_seconds_draft_.find(phase->source_);
            if(tracked==dagger_successful_seconds_draft_.end()||tracked->second.lock()!=phase)return;
            owned_phase=true;
            const auto actor=players_.find(phase->source_.actor);
            if(actor==players_.end()||actor->second.effect_scope!=phase->source_||
               actor->second.control_epoch!=phase->attempt_->control_||actor->second.attack_interrupt_epoch!=phase->attempt_->interrupt_||
               holding_bundle_locked(phase->source_)||!settle_timed_effects_locked(now)||
               !dagger_dual_participants_draft_locked(phase->source_,phase->target_,phase->attempt_->weapon_,now)||dagger_dual_world_draft_)return;
            // Timed settlement can deliver a control whose active KD has already
            // ended at this coarse update. Exact epochs, not current KD alone,
            // decide whether this retained physical release survived.
            const auto current=players_.find(phase->source_.actor);
            if(current==players_.end()||current->second.effect_scope!=phase->source_||
               current->second.control_epoch!=phase->attempt_->control_||
               current->second.attack_interrupt_epoch!=phase->attempt_->interrupt_)return;
            if(!current->second.local_cooldown_snapshot_ready||!current->second.local_cooldown_snapshot||
               current->second.local_cooldown_snapshot->snapshot!=before){result.retry=true;return;}
            if(phase->closed_.exchange(true))return;
            auto group=phase->group_;if(!group.select_second(now))return;
            dagger_dual_world_draft_=std::make_unique<DaggerDualWorldDraft>(DaggerDualWorldDraft{phase->source_,phase->target_,phase->attempt_->plan_.skill,now,std::move(group),{},phase->continuation_,{},{}});
            struct Reset{std::unique_ptr<DaggerDualWorldDraft>& state;~Reset(){state.reset();}} reset{dagger_dual_world_draft_};
            const auto first=next_attack_outcome_;
            melee_attack_locked(phase->source_,phase->target_,phase->attempt_->weapon_,phase->attempt_->plan_,now,{},true);
            for(const auto& outcome:resolved_attacks_)if(outcome.attacker==phase->source_&&outcome.target==phase->target_&&outcome.event>=first&&outcome.event<next_attack_outcome_)result.outcomes.push_back(outcome);
            result.accepted=dagger_dual_world_draft_->physical_release[1]&&!result.outcomes.empty();
            result.amendment=dagger_dual_world_draft_->amendment;
            if(!dagger_dual_world_draft_->physical_release[1]&&dagger_dual_world_draft_->retry_before_release){
                result.retry=true;phase->closed_.store(false); // no actual event bound; original first group remains retained
            }
        };perform();
        if(owned_phase&&!result.accepted&&!result.retry)phase->cancel();
        result.publication=finish_damage_publication_locked(pending);return result;
    }
