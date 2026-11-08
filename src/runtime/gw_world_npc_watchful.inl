// Included inside Instance. Explicit reviewed self-casts only; no fabricated
// NPC loadout, rank, profession bonus or autonomous decision policy.
private:
    struct NpcWatchfulCast {combat::npcs::control::Token token;skills::effects::WatchfulHealingPlan plan;};
    std::map<unsigned,NpcWatchfulCast> npc_watchful_casts_;
    std::set<unsigned> npc_watchful_in_progress_;
    std::optional<std::uint64_t> next_npc_watchful_deadline_locked()const{
        std::optional<std::uint64_t> result;
        for(const auto& [id,cast]:npc_watchful_casts_)if(!npc_watchful_in_progress_.contains(id))
            if(!result||cast.token.completes<*result)result=cast.token.completes;
        return result;
    }
    bool dispatch_npc_watchful_locked(std::uint64_t now){
        std::vector<combat::npcs::control::Token> due;
        for(const auto& [id,cast]:npc_watchful_casts_)if(!npc_watchful_in_progress_.contains(id)&&cast.token.completes<=now)due.push_back(cast.token);
        for(const auto& token:due){
            complete_npc_watchful_locked(token,token.completes);
            const auto pending=npc_watchful_casts_.find(token.actor.actor);
            if(pending!=npc_watchful_casts_.end()&&pending->second.token==token)return false;
        }
        return true;
    }
public:
    std::optional<combat::npcs::control::Token> begin_npc_watchful(
            skills::effects::Scope source,const skills::effects::WatchfulHealingPlan& plan,std::uint64_t now){
        const auto canonical=skills::effects::watchful_healing(plan.rank);
        if(!canonical||*canonical!=plan||now>UINT64_MAX-plan.activation-plan.duration-plan.recharge)return {};
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto body=npcs_.find(source.actor);auto live=npc_combat_.find(source.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source||next_npc_hex_==UINT32_MAX)return {};
        const auto life=live->second.lifetime();
        if(const auto old=npc_watchful_casts_.find(source.actor);old!=npc_watchful_casts_.end()){
            if(live->second.owns_spell_action(life,old->second.token))return {};
            npc_watchful_casts_.erase(old);
        }
        if(!settle_mind_wrack_caster_locked(source,now))return {};
        advance_npc_conditions_locked(source.actor,live->second,now);refresh_npc_movement_locked(source.actor,now);
        live=npc_combat_.find(source.actor);if(live==npc_combat_.end()||!live->second.matches(life))return {};
        const combat::ai::SpellTarget self{source,0,true,false,true,true};
        const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at){return skill_disabled_slot_locked(source,slot,at);};
        const auto choice=live->second.choose_spell(life,self,now,available);
        if(!choice||choice->skill!=1392)return {};
        const auto before=live->second.energy(life);const auto units=live->second.precise_energy_units(life);
        const auto token=live->second.begin_spell(life,*choice,self,now,available);if(!token)return {};
        npc_watchful_casts_.insert_or_assign(source.actor,NpcWatchfulCast{*token,plan});
        if(before)observe_npc_mind_wrack_energy_locked(source,{before->current,before->maximum,0,units?unsigned(*units%300000):0},now);
        live=npc_combat_.find(source.actor);const auto current_body=npcs_.find(source.actor);
        if(live==npc_combat_.end()||current_body==npcs_.end()||!live->second.matches(life)||
           !live->second.actor(life)->health||live->second.actor(life)->health->current<=0){
            npc_watchful_casts_.erase(source.actor);return token;}
        if(quest_npc_motion_.erase(source.actor))for(auto& [id,peer]:players_)if(peer.npc_publication_ready)
            peer.outbox->enqueue({position_packet(source.actor,current_body->second.x,current_body->second.y,current_body->second.plane),stop_packet(source.actor)});
        npc_flare_event_locked(60,source.actor,1392,source.actor);return token;
    }
    bool complete_npc_watchful(const combat::npcs::control::Token& token,std::uint64_t now){
        std::lock_guard lock(mutex_);if(now<token.completes)return false;
        return complete_npc_watchful_locked(token,token.completes);
    }
private:
    bool complete_npc_watchful_locked(const combat::npcs::control::Token& token,std::uint64_t now){
        const auto pending=npc_watchful_casts_.find(token.actor.actor);
        if(pending==npc_watchful_casts_.end()||pending->second.token!=token||now<token.completes)return false;
        if(!npc_watchful_in_progress_.insert(token.actor.actor).second)return false;
        struct Reset {std::set<unsigned>& actors;unsigned actor;~Reset(){actors.erase(actor);}} reset{npc_watchful_in_progress_,token.actor.actor};
        auto live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=token.actor){npc_watchful_casts_.erase(pending);return false;}
        const auto life=live->second.lifetime();const auto plan=pending->second.plan;
        if(!live->second.owns_spell_action(life,token)||live->second.control_epoch(life)!=token.epoch){npc_watchful_casts_.erase(pending);return false;}
        if(now>UINT64_MAX-plan.duration||next_npc_hex_==UINT32_MAX)return false;
        advance_npc_conditions_locked(token.actor.actor,live->second,now);
        live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||live->second.control_epoch(life)!=token.epoch||
           !live->second.actor(life)->health||live->second.actor(life)->health->current<=0){
            npc_watchful_casts_.erase(token.actor.actor);return false;}
        auto preview=live->second;
        if(!preview.complete_action(life,token,now)||!preview.apply_watchful_healing(life,plan,token.actor.actor,next_npc_hex_,now))return false;
        const auto aura=prepare_npc_aura_completion_locked(token,now);
        const auto terminal=complete_npc_spell_terminal_locked(token,now);
        if(terminal==NpcSpellTerminal::unavailable)return false;
        npc_watchful_casts_.erase(token.actor.actor);npc_flare_event_locked(58,token.actor.actor);
        if(terminal==NpcSpellTerminal::reaction_stopped)return true;
        live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||!live->second.matches(life))return true;
        const auto old=live->second.owned_effect_snapshot(life);
        if(!old||!live->second.apply_watchful_healing(life,plan,token.actor.actor,next_npc_hex_,now))return true;
        const auto effect=next_npc_hex_++;const auto entry=live->second.hex_effect(life,effect);
        std::vector<Packet> packets{*skills::particles::targeted_body_2022(token.actor.actor,token.actor.actor,1392)};
        for(const auto& previous:old->active(token.actor,now))if(previous.skill==1392){
            packets.push_back(health_effect_remove(token.actor.actor,previous.instance));watchful_credit_.erase({token.actor.actor,previous.instance});}
        watchful_credit_.insert_or_assign({token.actor.actor,effect},WatchfulCredit{token.actor,token.actor,now});
        packets.push_back(health_effect_add(*entry,token.actor.actor,now));
        packets.push_back(emotes::effects_2022(token.actor.actor,live->second.actor(life)->effects));
        for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue(packets);
        commit_applications_locked(token.actor,now);finish_npc_aura_completion_locked(aura,now);
        if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC Watchful Succor completion failed");return true;
    }
public:
