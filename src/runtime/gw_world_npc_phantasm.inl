private:
    struct NpcPhantasmCast {combat::npcs::control::Token token;skills::effects::Scope target;unsigned rank=0;};
    std::map<unsigned,NpcPhantasmCast> npc_phantasm_casts_;
    std::set<unsigned> npc_phantasm_in_progress_;
    std::optional<std::uint64_t> next_npc_phantasm_deadline_locked()const{
        std::optional<std::uint64_t> next;
        for(const auto& [id,cast]:npc_phantasm_casts_)if(!npc_phantasm_in_progress_.contains(id))
            if(!next||cast.token.completes<*next)next=cast.token.completes;
        return next;
    }
    bool dispatch_npc_phantasm_locked(std::uint64_t now){
        std::vector<combat::npcs::control::Token> due;
        for(const auto& [id,cast]:npc_phantasm_casts_)if(!npc_phantasm_in_progress_.contains(id)&&cast.token.completes<=now)due.push_back(cast.token);
        for(const auto& token:due){complete_npc_phantasm_locked(token,token.completes);
            const auto pending=npc_phantasm_casts_.find(token.actor.actor);if(pending!=npc_phantasm_casts_.end()&&pending->second.token==token)return false;}
        return true;
    }
    std::optional<combat::ai::SpellTarget> npc_phantasm_target_locked(skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        const auto caster=npc_combat_.find(source.actor),victim=npc_combat_.find(target.actor);
        const auto a=npcs_.find(source.actor),b=npcs_.find(target.actor);
        if(caster==npc_combat_.end()||victim==npc_combat_.end()||a==npcs_.end()||b==npcs_.end()||
           caster->second.hex_scope()!=source||victim->second.hex_scope()!=target||source.actor==target.actor||
           !caster->second.actor(caster->second.lifetime())->health||caster->second.actor(caster->second.lifetime())->health->current<=0||
           !caster->second.hostile_to_players(caster->second.lifetime())||!victim->second.healable_ally(victim->second.lifetime())||
           !victim->second.actor(victim->second.lifetime())->damageable)return {};
        bool line=a->second.plane==b->second.plane;
        if(line&&navigation_){const auto cell=navigation_->locate_cell(a->second.plane,{a->second.x,a->second.y});
            if(!cell)line=false;else{const auto trace=layered_->trace({a->second.plane,*cell},{a->second.x,a->second.y},{b->second.x,b->second.y});line=trace&&trace->plane==b->second.plane;}}
        return combat::ai::SpellTarget{target,float(std::hypot(a->second.x-b->second.x,a->second.y-b->second.y)),true,true,a->second.plane==b->second.plane,line};
    }
public:
    // Explicit reviewed loadout/rank;no autonomous skill choice is invented.
    std::optional<combat::npcs::control::Token> begin_npc_phantasm(skills::effects::Scope source,
            skills::effects::Scope target,unsigned rank,std::uint64_t now){
        const auto plan=skills::effects::degeneration_hex(31,rank);if(!plan||now>UINT64_MAX-plan->duration-66000)return {};
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        auto caster=npc_combat_.find(source.actor),victim=npc_combat_.find(target.actor);
        if(caster==npc_combat_.end()||victim==npc_combat_.end()||caster->second.hex_scope()!=source||victim->second.hex_scope()!=target)return {};
        const auto life=caster->second.lifetime();
        if(const auto old=npc_phantasm_casts_.find(source.actor);old!=npc_phantasm_casts_.end()){
            if(caster->second.owns_spell_action(life,old->second.token))return {};npc_phantasm_casts_.erase(old);}
        if(!settle_mind_wrack_caster_locked(source,now))return {};
        advance_npc_conditions_locked(source.actor,caster->second,now);advance_npc_conditions_locked(target.actor,victim->second,now);
        refresh_npc_movement_locked(source.actor,now);refresh_npc_movement_locked(target.actor,now);
        const auto facts=npc_phantasm_target_locked(source,target,now);if(!facts)return {};
        caster=npc_combat_.find(source.actor);if(caster==npc_combat_.end()||!caster->second.matches(life))return {};
        const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at){return skill_disabled_slot_locked(source,slot,at);};
        const auto choice=caster->second.choose_spell(life,*facts,now,available);if(!choice||choice->skill!=31)return {};
        const auto before=caster->second.energy(life);const auto units=caster->second.precise_energy_units(life);
        const auto token=caster->second.begin_spell(life,*choice,*facts,now,available);if(!token)return {};
        npc_phantasm_casts_[source.actor]={*token,target,rank};
        if(before)observe_npc_mind_wrack_energy_locked(source,{before->current,before->maximum,0,units?unsigned(*units%300000):0},now);
        caster=npc_combat_.find(source.actor);
        if(caster==npc_combat_.end()||!caster->second.matches(life)||caster->second.actor(life)->health->current<=0){npc_phantasm_casts_.erase(source.actor);return token;}
        // A non-instant spell stops the source's existing path just as other
        // reviewed NPC casts do. Do not let quest movement continue through it.
        if(quest_npc_motion_.erase(source.actor)){
            const auto& body=npcs_.at(source.actor);
            for(auto& [id,peer]:players_)if(peer.npc_publication_ready)
                peer.outbox->enqueue({position_packet(source.actor,body.x,body.y,body.plane),stop_packet(source.actor)});
        }
        npc_flare_event_locked(60,source.actor,31,target.actor);return token;
    }
    bool complete_npc_phantasm(const combat::npcs::control::Token& token,std::uint64_t now){
        std::lock_guard lock(mutex_);if(now<token.completes)return false;return complete_npc_phantasm_locked(token,token.completes);
    }
private:
    bool complete_npc_phantasm_locked(const combat::npcs::control::Token& token,std::uint64_t now){
        const auto pending=npc_phantasm_casts_.find(token.actor.actor);
        if(pending==npc_phantasm_casts_.end()||pending->second.token!=token||now<token.completes||npc_phantasm_in_progress_.contains(token.actor.actor))return false;
        npc_phantasm_in_progress_.insert(token.actor.actor);
        struct Reset{std::set<unsigned>& active;unsigned actor;~Reset(){active.erase(actor);}} reset{npc_phantasm_in_progress_,token.actor.actor};
        const auto cast=pending->second;auto caster=npc_combat_.find(token.actor.actor);
        if(caster==npc_combat_.end()||caster->second.hex_scope()!=token.actor){npc_phantasm_casts_.erase(pending);return false;}
        const auto life=caster->second.lifetime();
        if(!caster->second.owns_spell_action(life,token)||caster->second.control_epoch(life)!=token.epoch){npc_phantasm_casts_.erase(pending);return false;}
        advance_npc_conditions_locked(token.actor.actor,caster->second,now);
        auto target=npc_combat_.find(cast.target.actor);
        if(target!=npc_combat_.end()&&target->second.hex_scope()==cast.target)advance_npc_conditions_locked(cast.target.actor,target->second,now);
        refresh_npc_movement_locked(token.actor.actor,now);refresh_npc_movement_locked(cast.target.actor,now);
        const auto facts=npc_phantasm_target_locked(token.actor,cast.target,now);
        caster=npc_combat_.find(token.actor.actor);
        if(caster==npc_combat_.end()||!caster->second.matches(life)||caster->second.actor(life)->health->current<=0||caster->second.control_epoch(life)!=token.epoch){npc_phantasm_casts_.erase(token.actor.actor);return false;}
        if(!facts||!facts->same_plane||!facts->line_of_sight){
            if(!caster->second.fail_target_action(life,token,now))return false;
            npc_phantasm_casts_.erase(token.actor.actor);npc_flare_event_locked(59,token.actor.actor);return true;
        }
        const auto plan=skills::effects::degeneration_hex(31,cast.rank);
        if(!plan||now>UINT64_MAX-plan->duration||next_npc_hex_==UINT32_MAX)return false;
        target=npc_combat_.find(cast.target.actor);auto preview=target->second;
        if(!preview.apply_conjure_phantasm_from_npc(preview.lifetime(),token.actor,cast.rank,next_npc_hex_,now))return false;
        const auto aura=prepare_npc_aura_completion_locked(token,now);
        const auto terminal=complete_npc_spell_terminal_locked(token,now);
        if(terminal==NpcSpellTerminal::unavailable)return false;
        npc_phantasm_casts_.erase(token.actor.actor);npc_flare_event_locked(58,token.actor.actor);
        if(terminal==NpcSpellTerminal::reaction_stopped)return true;
        if(!npc_phantasm_target_locked(token.actor,cast.target,now))return true;
        target=npc_combat_.find(cast.target.actor);preview=target->second;
        const auto applied=preview.apply_conjure_phantasm_from_npc(preview.lifetime(),token.actor,cast.rank,next_npc_hex_,now);if(!applied)return true;
        const auto effect=next_npc_hex_++;target->second=std::move(preview);
        for(const auto& old:applied->removed){npc_simple_enchantment_credit_.erase({cast.target.actor,old.instance});for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(cast.target.actor,old.instance)});}
        npc_simple_enchantment_credit_[{cast.target.actor,effect}]=cast.target;
        const auto entry=target->second.hex_effect(target->second.lifetime(),effect);if(!entry)throw std::logic_error("NPC Conjure Phantasm effect absent");
        const auto body=skills::particles::targeted_body_2022(cast.target.actor,token.actor.actor,31);if(!body)throw std::logic_error("NPC Conjure Phantasm body absent");
        for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({*body,health_effect_add(*entry,cast.target.actor,now),emotes::effects_2022(cast.target.actor,target->second.actor(target->second.lifetime())->effects)});
        commit_applications_locked(cast.target,now);finish_npc_aura_completion_locked(aura,now);
        if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC Phantasm Succor completion failed");return true;
    }
public:
