// Included inside World. Successful terminal events only; starts, failures and
// interruption callbacks must never reach this entrypoint. The sequence belongs
// to the exact recipient lifetime, and must increase across all its completions.
private:
    bool succor_spell_completed_locked(skills::effects::Scope target,unsigned skill,std::uint64_t sequence,std::uint64_t now){
        const auto* row=skills::find_skill(skill);
        if(!sequence||!row||!skills::effects::succor_scope(target))return false;
        auto recipient=players_.find(target.actor);
        // Settlement may end upkeep, kill either actor, or erase ownership.
        // Never retain a map iterator/reference across any such authority call.
        if(recipient!=players_.end()){
            if(recipient->second.effect_scope!=target||!advance_resource_event_locked(recipient->second,now))return false;
        }else{
            const auto npc=npc_combat_.find(target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
            const auto clock=npc->second.health_clock(npc->second.lifetime());
            const auto energy_clock=npc->second.energy_clock(npc->second.lifetime());
            if((clock&&now<*clock)||(energy_clock&&now<*energy_clock))return false;
            advance_npc_conditions_locked(target.actor,npc->second,now);
        }
        // Copies survive caster settlement and owned-end/death callbacks.
        const auto registry=[&]()->std::optional<skills::effects::Registry>{
            if(const auto player=players_.find(target.actor);player!=players_.end()){
                if(player->second.effect_scope!=target||!player->second.effect_registry)return {};
                return *player->second.effect_registry;
            }
            const auto npc=npc_combat_.find(target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return {};
            return npc->second.owned_effect_snapshot(npc->second.lifetime());
        };
        std::vector<std::pair<skills::effects::Scope,unsigned>> keys;
        for(const auto& [key,lease]:succor_leases_)if(lease.target==target)keys.push_back(key);
        for(const auto& key:keys){
            auto lease_it=succor_leases_.find(key);
            auto consumer=succor_consumers_.find(key);
            if(lease_it==succor_leases_.end()||consumer==succor_consumers_.end())continue;
            const auto lease=lease_it->second;
            if(sequence<=consumer->second.sequence())continue;
            auto effects=registry();if(!effects)continue;
            auto caster=players_.find(lease.caster.actor);
            if(caster==players_.end()||caster->second.effect_scope!=lease.caster)continue;
            if(!skills::effects::succor_contribution(lease,lease.caster,target,*effects,now))continue;
            if(skills::effects::is_spell(*row)&&!preflight_debilitating_locked(caster->second,1,now))return false;
            // Preflight settlement can remove this lease, replace its effect,
            // or cause death cleanup. Re-resolve every identity before commit.
            lease_it=succor_leases_.find(key);consumer=succor_consumers_.find(key);
            effects=registry();caster=players_.find(lease.caster.actor);
            if(lease_it==succor_leases_.end()||lease_it->second!=lease||consumer==succor_consumers_.end()||
               !effects||
               caster==players_.end()||caster->second.effect_scope!=lease.caster||caster->second.precise_health<=0||
               !skills::effects::succor_contribution(lease,lease.caster,target,*effects,now))continue;
            auto proposed=consumer->second;
            const auto loss=proposed.completed(lease.caster,target,*effects,skill,sequence,now,
                [](skills::effects::Scope,unsigned){return true;});
            if(!loss)return false;
            // Publish deduplication before reactions: exact-zero Mind Wrack or
            // upkeep cleanup may erase this consumer during the Energy commit.
            consumer->second=std::move(proposed);
            if(*loss)commit_debilitating_locked(caster->second,*loss,now);
        }
        return true;
    }
    // Trusted NPC producers call only after the live action and its spell
    // effect/release succeed. Token sequence is shared across that NPC lifetime;
    // preview copies, failed targets and interrupted/cancelled actions exclude it.
    bool succor_npc_spell_completed_locked(const combat::npcs::control::Token& token,std::uint64_t now){
        if(token.kind!=combat::npcs::control::Action::spell||!token.skill||
           !token.sequence||now<token.completes)return false;
        const auto* row=skills::find_skill(token.skill->skill);
        const auto npc=npc_combat_.find(token.actor.actor);
        if(!row||!skills::effects::is_spell(*row)||npc==npc_combat_.end()||npc->second.hex_scope()!=token.actor)return false;
        return succor_spell_completed_locked(token.actor,token.skill->skill,token.sequence,now);
    }
public:
    bool succor_spell_completed(skills::effects::Scope target,unsigned skill,std::uint64_t sequence,std::uint64_t now){
        std::lock_guard lock(mutex_);
        return succor_spell_completed_locked(target,skill,sequence,now);
    }
