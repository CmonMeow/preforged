// Included inside Instance; all operations run under its existing mutex.
    bool reserve_shield_bash_contact_locked(Player& defender,skills::effects::Scope source,skills::Slot slot,
            std::uint64_t event,std::uint64_t now,bool melee,bool unblockable,const skills::effects::AttributedBlock& block,bool commit=true){
        if(!slot.skill)return true;
        const auto protection=skills::effects::shield_bash_protection(*defender.effect_registry,defender.effect_scope,now,effective_shield_locked(defender),slot.skill);
        if(!protection)return false;if(!protection->instance)return true;
        const bool attributed=block.outcome==AttackOutcome::block&&block.source&&block.source->skill==363&&block.source->instance==protection->instance;
        const bool bypass=melee&&unblockable&&block.outcome==AttackOutcome::hit;
        if(!attributed&&!bypass)return true;
        if((commit&&!event)||slot.copy>65535||shield_bash_contacts_.contains(event)||shield_bash_contacts_.size()>=4096)return false;
        std::optional<skills::effects::ContactCooldownQuote> continuation_quote;
        if(melee){
            const auto attacker=players_.find(source.actor);const auto pending=pending_diversion_uses_.find(source);
            if(attacker==players_.end()||attacker->second.effect_scope!=source||!attacker->second.resources||
               attacker->second.precise_health<=0||now<attacker->second.health_updated||attacker->second.control_epoch==UINT64_MAX||
               !attacker->second.cooldown_authority)return false;
            const bool pending_ready=pending!=pending_diversion_uses_.end()&&pending->second.prepared.use.slot==slot&&
                pending->second.prepared.at==now&&pending->second.control_epoch==attacker->second.control_epoch&&!pending->second.shield_bash_extra;
            if(dagger_dual_second_draft_locked(source,defender.effect_scope,slot.skill,now)){
                if(!commit||!dagger_dual_world_draft_->group.owns_event(event)||!dagger_dual_world_draft_->continuation||
                   dagger_dual_world_draft_->continuation->slot()!=slot||!dagger_dual_world_draft_->bash_quote)return false;
                continuation_quote=dagger_dual_world_draft_->bash_quote;
            }else if(pending_ready){auto clock=*attacker->second.cooldown_authority;if(!clock.order(source,now))return false;}
            else return false;
        }
        if(commit)shield_bash_contacts_.emplace(event,ShieldBashContactReservation{defender.effect_scope,slot,protection->instance,bypass,source,continuation_quote});return true;
    }
    void finish_shield_bash_contact_locked(const combat::weapon::ResolvedAttack& terminal){
        const auto queued=shield_bash_contacts_.find(terminal.event);if(queued==shield_bash_contacts_.end())return;
        const auto reserved=queued->second;shield_bash_contacts_.erase(queued);
        const auto defender=players_.find(reserved.target.actor);
        if(defender==players_.end()||defender->second.effect_scope!=terminal.target||terminal.target!=reserved.target||terminal.attacker!=reserved.source||
           !defender->second.effect_registry||defender->second.precise_health<=0)return;
        auto& target=defender->second;
        const skills::effects::ShieldBashContact contact{terminal,reserved.slot,reserved.bypass?0u:reserved.effect,reserved.bypass};
        const auto proposal=skills::effects::prepare_shield_bash_contact(*target.effect_registry,target.effect_scope,effective_shield_locked(target),contact);
        if(!proposal)return;
        if(proposal->knockdown){
            const auto attacker=players_.find(terminal.attacker.actor);const auto pending=pending_diversion_uses_.find(terminal.attacker);
            if(reserved.continuation_quote&&attacker!=players_.end()&&attacker->second.effect_scope==terminal.attacker&&attacker->second.precise_health>0){
                if(!dagger_dual_second_draft_locked(terminal.attacker,terminal.target,reserved.slot.skill,terminal.at)||
                   !dagger_dual_world_draft_->group.owns_event(terminal.event)||!dagger_dual_world_draft_->physical_release[1])
                    throw std::logic_error("Bash amendment lacks canonical actual second release");
                auto& source=attacker->second;
                if(source.knocked_down_until<=terminal.at&&!knockdown_player_locked(source,2000,terminal.at))
                    throw std::logic_error("preflighted continuation Bash knockdown failed");
                const auto amendment=commit_contact_amendment_locked(*reserved.continuation_quote,terminal.event,terminal.at);
                if(!amendment)throw std::logic_error("preflighted continuation Bash amendment failed");
                dagger_dual_world_draft_->amendment=*amendment;
            }else if(attacker!=players_.end()&&attacker->second.effect_scope==terminal.attacker&&attacker->second.precise_health>0&&
               pending!=pending_diversion_uses_.end()&&pending->second.prepared.use.slot==reserved.slot&&
               pending->second.prepared.at==terminal.at&&pending->second.control_epoch==attacker->second.control_epoch){
                auto& source=attacker->second;
                if(source.knocked_down_until<=terminal.at&&!knockdown_player_locked(source,2000,terminal.at))throw std::logic_error("preflighted Shield Bash knockdown failed");
                // This exact attack has already reached a terminal contact. Its
                // own retaliation cannot retroactively make it an unused skill.
                pending->second.control_epoch=source.control_epoch;pending->second.shield_bash_extra=15000;
            }
            // A prior recipient's retaliation can kill the source. The trusted
            // terminal block still spends this defender's Bash; a dead source
            // receives no new control and follows the shared death/use cleanup.
        }
        *target.effect_registry=proposal->registry;
        for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(target.id,reserved.effect)});
        publish_status_locked(target,terminal.at);
    }
