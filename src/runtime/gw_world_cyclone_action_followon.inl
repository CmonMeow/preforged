// Detached staged owned Cyclone330 source action. No normal Runtime hook.
private:
    std::map<skills::effects::Scope,std::weak_ptr<combat::weapon::AttackAdmission>> cyclone_attempts_followon_;
    bool player_empathy_action_release_locked(const combat::weapon::AttackMade& made,bool react=true){
        const auto ledger=attack_outcomes_.find(made.owner());if(ledger==attack_outcomes_.end())return false;
        const auto seen=ledger->second.action_observed(made.owner(),made,1);if(!seen)return false;if(*seen)return true;
        const auto source=players_.find(made.owner().actor);
        if(source==players_.end()||source->second.effect_scope!=made.owner()||(react&&!player_empathy_reaction_ready_locked(made.owner(),made.at())))return false;
        if(!ledger->second.observe_action_once(made.owner(),made,1))return false;
        return !react||commit_player_empathy_reaction_locked(made.owner(),made.at());
    }
public:
    struct CycloneCompletionFollowon{bool accepted=false;std::vector<combat::weapon::ResolvedAttack> contacts;std::function<void()> publication;};
    combat::weapon::AttackPermit prepare_cyclone_cast_followon(skills::effects::Scope source,
            const combat::weapon::Owned& weapon,const combat::weapon::AttackPlan& plan,std::uint64_t now,std::uint64_t release){
        if(plan.skill!=330||!combat::weapon::valid(plan)||weapon.item_type!=2||weapon.caster_weapon||release<=now||release-now>90000)return {};
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source||holding_bundle_locked(source)||!settle_timed_effects_locked(now))return {};
        auto& actor=found->second;
        if(!actor.ordinary_weapon||!combat::weapon::same_weapon(*actor.ordinary_weapon,weapon)||!actor.effect_registry||!advance_resource_event_locked(actor,now)||actor.precise_health<=0||actor.knocked_down_until>now)return {};
        const auto effects=actor.effect_registry->active(source,now);const auto dwarven=dwarven_bonuses_locked(actor,now);if(!dwarven)return {};
        const auto duration=skills::effects::reviewed_attack_duration(plan.windup_ms,dwarven->haste||skills::effects::tiger_stance_active(effects,now)||skills::effects::hekets_rampage_active(effects,now)||skills::effects::frenzy_active(effects,now)||skills::effects::flurry_active(effects,now)||skills::effects::lightning_reflexes_active(effects,now)||skills::effects::flail_active(effects,now),skills::effects::faintheartedness_active(effects,now),skills::effects::bestial_fury_active(effects,now));
        if(!duration||!*duration||release-now!=*duration)return {};
        if(const auto prior=cyclone_attempts_followon_.find(source);prior!=cyclone_attempts_followon_.end())if(const auto permit=prior->second.lock();permit&&permit->open())return {};
        auto permit=combat::weapon::AttackPermit(new combat::weapon::AttackAdmission(source,source,weapon,plan,now,release,actor.control_epoch,actor.attack_interrupt_epoch,false));
        cyclone_attempts_followon_.insert_or_assign(source,permit);return permit;
    }
    CycloneCompletionFollowon complete_cyclone_cast_followon(skills::effects::Scope source,
            const combat::weapon::Owned& weapon,const combat::weapon::AttackPlan& plan,const combat::weapon::AttackPermit& permit,std::uint64_t now){
        if(!permit||plan.skill!=330||!combat::weapon::valid(plan)||weapon.item_type!=2||weapon.caster_weapon)return {};
        std::lock_guard lock(mutex_);
        // Settlement is causal output even on refusal. Capture it before any
        // side-effecting validation, matching the paid dagger completion owner.
        DamagePublication pending;begin_damage_publication_locked(pending);
        const auto perform=[&]()->CycloneCompletionFollowon{
            if(!settle_timed_effects_locked(now))return {};
            const auto found=players_.find(source.actor);const auto owned=cyclone_attempts_followon_.find(source);
            if(found==players_.end()||found->second.effect_scope!=source||holding_bundle_locked(source)||!found->second.ordinary_weapon||!combat::weapon::same_weapon(*found->second.ordinary_weapon,weapon)||owned==cyclone_attempts_followon_.end()||owned->second.lock()!=permit||!permit->claim(source,source,weapon,plan,now,found->second.control_epoch,found->second.attack_interrupt_epoch))return {};
            CycloneCompletionFollowon result;const auto first=next_attack_outcome_;
            result.accepted=cyclone_axe_locked(found->second,source,weapon,plan,now);
            for(const auto& contact:resolved_attacks_)if(contact.attacker==source&&contact.event>=first)result.contacts.push_back(contact);
            return result;
        };
        auto result=perform();auto publication=finish_damage_publication_locked(pending);
        if(result.accepted)result.publication=std::move(publication);
        else if(publication)publication(); // refusal still publishes authoritative settlement
        return result;
    }
private:
