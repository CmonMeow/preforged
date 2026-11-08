// Shared physical dual group with an opt-in retained paid second phase.
// Every dual enters with the existing paying cast permit and retained result.
private:
    struct DaggerDualWorldDraft {
        skills::effects::Scope source,target;unsigned skill=0;
        std::uint64_t at=0;
        combat::weapon::DaggerDualGroupDraft group;
        std::array<bool,2> physical_release{};
        std::shared_ptr<skills::effects::ContactCooldownContinuation> continuation;
        std::optional<skills::effects::ContactCooldownQuote> bash_quote;
        std::optional<skills::effects::CooldownAmendment> amendment;
        bool retry_before_release=false;
    };
    std::unique_ptr<DaggerDualWorldDraft> dagger_dual_world_draft_;
    std::uint64_t next_dagger_dual_group_draft_=1;
    std::map<skills::effects::Scope,std::weak_ptr<combat::weapon::DaggerSuccessfulSecondDraft>> dagger_successful_seconds_draft_;
    static bool dagger_dual_skill_draft(unsigned skill){return combat::weapon::dagger_dual_role(skill);}
    bool dagger_dual_allowed_draft_locked(skills::effects::Scope source,skills::effects::Scope target,
            unsigned skill,std::uint64_t at)const{
        return !dagger_dual_skill_draft(skill)||(dagger_dual_world_draft_&&
            dagger_dual_world_draft_->group.matches(source,target,skill,at));
    }
    bool dagger_dual_second_draft_locked(skills::effects::Scope source,skills::effects::Scope target,
            unsigned skill,std::uint64_t at)const{
        return dagger_dual_skill_draft(skill)&&dagger_dual_world_draft_&&
            dagger_dual_world_draft_->group.matches(source,target,skill,at)&&dagger_dual_world_draft_->group.ordinal()==1;
    }
    std::optional<bool> claim_dagger_dual_release_draft_locked(skills::effects::Scope source,
            skills::effects::Scope target,unsigned skill,std::uint64_t event,std::uint64_t at){
        if(!dagger_dual_skill_draft(skill))return false;
        if(!dagger_dual_world_draft_)return {};
        return dagger_dual_world_draft_->group.release(source,target,skill,event,at);
    }
    void finish_dagger_dual_event_draft_locked(const combat::weapon::ResolvedAttack& outcome){
        if(!dagger_dual_world_draft_||!dagger_dual_world_draft_->group.owns_event(outcome.event))return;
        if(!dagger_dual_world_draft_->group.finish(outcome))throw std::logic_error("dual group lost actual strike terminal");
    }
    // Called only after the canonical release observer consumes its event.
    // Reservation/cancelled terminals alone do not prove an actual attack made.
    void mark_dagger_physical_release_draft_locked(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t event,std::uint64_t at){
        if(dagger_dual_world_draft_&&dagger_dual_world_draft_->group.owns_event(event)&&
           dagger_dual_world_draft_->group.matches(source,target,dagger_dual_world_draft_->skill,at)){
            if(dagger_dual_world_draft_->continuation&&!bind_contact_event_locked(dagger_dual_world_draft_->continuation,dagger_dual_world_draft_->group.ordinal(),event))
                throw std::logic_error("owned dual continuation lost actual release binding");
            dagger_dual_world_draft_->physical_release[dagger_dual_world_draft_->group.ordinal()]=true;
        }
    }
    // Preflight potential Bash before release/Empathy/Vigor. No RNG is drawn
    // here; only the later actual attributed block/bypass spends the quote.
    bool preflight_dagger_second_bash_draft_locked(Player& defender,skills::effects::Scope source,
            const combat::weapon::AttackPlan& plan,std::uint64_t event,std::uint64_t now){
        if(!dagger_dual_second_draft_locked(source,defender.effect_scope,plan.skill,now))return true;
        if(!defender.effect_registry||!dagger_dual_world_draft_->continuation)return false;
        const auto protection=skills::effects::shield_bash_protection(*defender.effect_registry,defender.effect_scope,now,effective_shield_locked(defender),plan.skill);
        if(!protection)return false;if(!protection->instance)return true;
        const auto attacker=players_.find(source.actor);
        if(attacker==players_.end()||attacker->second.effect_scope!=source||attacker->second.control_epoch>=UINT64_MAX-1)return false;
        if(!dagger_dual_world_draft_->group.owns_event(event)||!attacker->second.cooldown_authority)return false;
        auto clock=*attacker->second.cooldown_authority;
        if(!clock.order(source,now)||!clock.order(source,now))return false; // permanent exhaustion, not retry
        const auto quote=preflight_contact_amendment_locked(dagger_dual_world_draft_->continuation,event,now);
        if(!quote){dagger_dual_world_draft_->retry_before_release=true;return false;}dagger_dual_world_draft_->bash_quote=*quote;return true;
    }
    bool dagger_dual_participants_draft_locked(skills::effects::Scope source,
            skills::effects::Scope target,const combat::weapon::Owned& weapon,std::uint64_t now){
        const auto actor=players_.find(source.actor);
        if(actor==players_.end()||actor->second.effect_scope!=source||!actor->second.resources||actor->second.precise_health<=0||
           actor->second.knocked_down_until>now||!actor->second.ordinary_weapon||
           !combat::weapon::same_weapon(*actor->second.ordinary_weapon,weapon)||weapon.item_type!=32||weapon.caster_weapon)return false;
        if(const auto player=players_.find(target.actor);player!=players_.end()){
            if(player->second.effect_scope!=target||player->second.precise_health<=0)return false;
            const auto state=ordinary_snapshot_locked(actor->second,player->second,now);
            return state.alive&&state.target_alive&&state.hostile&&state.line_of_sight&&state.distance<=128;
        }
        refresh_npc_movement_locked(target.actor,now);
        const auto npc=npc_combat_.find(target.actor);const auto placed=npcs_.find(target.actor);
        if(npc==npc_combat_.end()||placed==npcs_.end()||npc->second.hex_scope()!=target)return false;
        const auto* body=npc->second.actor(npc->second.lifetime());if(!body||!body->health||body->health->current<=0)return false;
        const auto state=ordinary_npc_snapshot_locked(actor->second,placed->second,npc->second,now);
        return state.alive&&state.target_alive&&state.hostile&&state.line_of_sight&&state.distance<=128;
    }
    // Enter only from the original public melee_attack owner under mutex_.
    // There is one logical attack call/cost/cooldown owner and at most two
    // ACTUAL synchronous strike invocations. Every strike independently runs
    // existing miss/block/critical/qualified damage/reactions/terminal delivery.
    // A lethal/retiring first strike never fabricates a second hit or receipt.
    bool dagger_dual_attack_draft_locked(skills::effects::Scope source,skills::effects::Scope target,
            const combat::weapon::Owned& weapon,const combat::weapon::AttackPlan& plan,std::uint64_t now,
            const combat::weapon::AttackPermit& paid_attempt,
            const std::shared_ptr<skills::effects::ContactCooldownContinuation>& continuation,
            std::shared_ptr<combat::weapon::DaggerSuccessfulSecondDraft>* deferred){
        if(!paid_attempt||!deferred||!continuation||continuation->source()!=source||continuation->slot()!=skills::Slot{plan.skill,plan.copy}||!dagger_dual_skill_draft(plan.skill)||!combat::weapon::valid(plan,true)||dagger_dual_world_draft_||
           next_dagger_dual_group_draft_==UINT64_MAX||next_attack_outcome_>UINT64_MAX-2||
           !dagger_dual_participants_draft_locked(source,target,weapon,now))return false;
        const auto owner=players_.find(source.actor);if(owner==players_.end()||owner->second.effect_scope!=source)return false;
        const auto ledger=attack_outcomes_.find(source);
        if(ledger!=attack_outcomes_.end()&&(ledger->second.clock()>now||ledger->second.size()>4094))return false;
        const auto group=combat::weapon::DaggerDualGroupDraft::begin(source,target,owner->second.dagger_chain,
            plan.skill,next_dagger_dual_group_draft_,now);
        if(!group)return false;
        ++next_dagger_dual_group_draft_;
        dagger_dual_world_draft_=std::make_unique<DaggerDualWorldDraft>(DaggerDualWorldDraft{source,target,plan.skill,now,*group,{},continuation,{},{}});
        struct Release {std::unique_ptr<DaggerDualWorldDraft>& state;~Release(){state.reset();}} release{dagger_dual_world_draft_};
        const bool first=melee_attack_locked(source,target,weapon,plan,now,{},true);
        const bool first_terminal=dagger_dual_world_draft_->group.outcomes()[0].has_value();
        const bool first_physical=dagger_dual_world_draft_->physical_release[0];
        // A reservation cancelled before the canonical observer is not an attack.
        if(!first_physical){dagger_dual_world_draft_->group.stop();return false;}
        if(!first||!first_terminal||!dagger_dual_participants_draft_locked(source,target,weapon,now)){
            dagger_dual_world_draft_->group.stop();return first_terminal;
        }
        if(deferred&&paid_attempt){
            if(!dagger_dual_world_draft_->physical_release[0]){dagger_dual_world_draft_->group.stop();return false;}
            const auto current=players_.find(source.actor);
            if(current==players_.end()||current->second.effect_scope!=source||
               current->second.control_epoch!=paid_attempt->control_||
               current->second.attack_interrupt_epoch!=paid_attempt->interrupt_){dagger_dual_world_draft_->group.stop();return true;}
            const auto delta=paid_attempt->release_-paid_attempt->began_;
            if(!delta||now>UINT64_MAX-delta){dagger_dual_world_draft_->group.stop();return true;}
            auto phase=std::shared_ptr<combat::weapon::DaggerSuccessfulSecondDraft>(
                new combat::weapon::DaggerSuccessfulSecondDraft(source,target,paid_attempt,dagger_dual_world_draft_->group,now+delta,continuation));
            dagger_successful_seconds_draft_.insert_or_assign(source,phase);*deferred=std::move(phase);return true;
        }
        return false; // No synchronous second-attack compatibility route.

    }
// Required exact caller hooks for this candidate driver:
// * public bool melee_attack rejects duals; actual paying Runtime completion
//     supplies its exact World attempt and receives the retained second cap.
// * paid AttackResolution constructors pass plan.skill to begin_attack_outcome;
//   all other callers retain default0. This prevents a nested unrelated contact
//   from claiming a dual event merely because scopes happen to match.
// * begin_attack_outcome, after real ledger.release and event assignment:
//   claim_dagger_dual_release; on second skip ONLY logical-release effects
//   (weapon-spell ending,Dodge reservation,pet release); both events remain real.
// * record/finish owner calls finish_dagger_dual_event after its real terminal
//   consumers; BOTH physical attacks trigger Vigorous; second skips only
//   the existing logical Spiteful release policy.
// * family effect helper uses the group's retained admitted stage2 snapshot
//   for each strike and preserves per-strike condition/damage ownership.
// Explicit current multi-contact policy: Dodge bonus is reserved to the first
// real event only, matching current next-contact convention. Retail dual
// combined-source evidence and animation cardinality still gate normal binding.
