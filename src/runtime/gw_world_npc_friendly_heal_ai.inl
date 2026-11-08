// Explicitly enrolled autonomous hold-position healers; no content activation.
private:
    struct FriendlyHealAiBinding {skills::effects::Scope source;combat::ai::friendly::HealPolicy policy;};
    std::map<unsigned,FriendlyHealAiBinding> npc_friendly_heal_ai_;
public:
    bool configure_npc_friendly_heal_ai(skills::effects::Scope source,
            const combat::ai::friendly::HealPolicy& policy){
        // Following requires a separately reviewed route/motion profile. The
        // pure selector describes approach decisions but this stage holds still.
        if(!combat::ai::friendly::valid_policy(policy)||!policy.hold_position)return false;
        std::lock_guard lock(mutex_);const auto healer=npc_friendly_healers_.find(source.actor);
        if(healer==npc_friendly_healers_.end()||healer->second.source!=source||npc_friendly_heal_ai_.contains(source.actor))return false;
        npc_friendly_heal_ai_.emplace(source.actor,FriendlyHealAiBinding{source,policy});return true;
    }
    void tick_npc_friendly_heal_ai(std::uint64_t now){
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);drive_npc_friendly_heal_ai_locked(now);
    }
private:
    void drive_npc_friendly_heal_ai_locked(std::uint64_t now){
        std::vector<std::pair<skills::effects::Scope,skills::effects::Scope>> requests;
        {
            std::erase_if(npc_friendly_heal_ai_,[&](const auto& item){const auto live=npc_combat_.find(item.first);
                return live==npc_combat_.end()||live->second.hex_scope()!=item.second.source;});
            for(const auto&[id,binding]:npc_friendly_heal_ai_){
                auto live=npc_combat_.find(id);const auto profile=npc_friendly_healers_.find(id);
                if(live==npc_combat_.end()||profile==npc_friendly_healers_.end()||profile->second.source!=binding.source)continue;
                const auto life=live->second.lifetime();
                if(now<live->second.health_clock(life).value_or(now)||now<live->second.energy_clock(life).value_or(now))continue;
                advance_npc_conditions_locked(id,live->second,now);refresh_npc_movement_locked(id,now);
                live=npc_combat_.find(id);if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.advance_energy(life,now))continue;
                const auto healer=npc_friendly_healer_snapshot_locked(binding.source);if(!healer)continue;
                const bool active=live->second.active_action(life,now).has_value();
                std::vector<combat::ai::friendly::Recipient> candidates;
                for(const auto&[player_id,player]:players_)
                    if(const auto recipient=npc_friendly_recipient_locked(binding.source,player.effect_scope,now))candidates.push_back(*recipient);
                // Snapshot scopes first: condition settlement may retire actors.
                std::vector<skills::effects::Scope> scopes;
                for(const auto&[actor_id,actor]:npc_combat_)scopes.push_back(actor.hex_scope());
                for(const auto scope:scopes)if(const auto recipient=npc_friendly_recipient_locked(binding.source,scope,now))candidates.push_back(*recipient);
                const auto decision=combat::ai::friendly::select(binding.policy,profile->second.profile,*healer,candidates,now,active,true);
                if(decision&&decision->kind==combat::ai::friendly::HealDecisionKind::cast)requests.emplace_back(binding.source,decision->target);
            }
        }
        // Begin performs fresh lifetime/Health/Energy/slot/cooldown revalidation
        // under the same World mutex. No decision snapshot is committed directly.
        for(const auto&[source,target]:requests)begin_npc_friendly_heal_locked(source,target,now);
    }

public:
