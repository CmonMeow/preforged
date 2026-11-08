// Included inside Instance. Typed immediate NPC effects share player damage authority.
private:
    void publish_npc_immediate_caster_body_followon_locked(skills::effects::Scope source,unsigned skill){
        const auto* row=skills::find_skill(skill);
        if(row&&row->caster_body_resource<2077){Packet packet;u16(packet,158);u32(packet,21);u32(packet,source.actor);u32(packet,row->caster_body_resource);
            for(auto& [id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({packet});}
    }
    combat::ai::activated::Completion complete_npc_immediate_damage_locked(
            const combat::npcs::control::Token& token,skills::effects::Scope target,
            const combat::ai::activated::Reviewed& reviewed,std::uint64_t now){
        using Result=combat::ai::activated::Completion;
        const auto source=token.actor;
        auto live=npc_combat_.find(source.actor);auto placed=npcs_.find(source.actor);
        if(!reviewed.valid()||!token.skill||*token.skill!=reviewed.slot||now<token.completes||
           live==npc_combat_.end()||placed==npcs_.end()||live->second.hex_scope()!=source||
           !live->second.owns_activated_skill_action(live->second.lifetime(),token))return Result::stopped;
        const auto life=live->second.lifetime();
        const auto stop=[&](){auto n=npc_combat_.find(source.actor);
            if(n!=npc_combat_.end()&&n->second.matches(life)&&n->second.owns_activated_skill_action(life,token))
                return n->second.fail_activated_skill_target_action(life,token,now)?Result::stopped:Result::deferred;
            return Result::stopped;};
        auto victim=players_.find(target.actor);
        if(victim==players_.end()||victim->second.effect_scope!=target)return stop();
        auto& p=victim->second;
        if(std::max({p.health_updated,p.energy_updated,p.last_move,p.steering?p.steering_tick:0})>now||
           now<live->second.health_clock(life).value_or(now)||now<live->second.energy_clock(life).value_or(now))return Result::deferred;
        advance_npc_conditions_locked(source.actor,live->second,now);
        live=npc_combat_.find(source.actor);placed=npcs_.find(source.actor);
        if(live==npc_combat_.end()||placed==npcs_.end()||!live->second.matches(life)||
           !live->second.owns_activated_skill_action(life,token))return Result::stopped;
        if(!p.resources||!advance_health_locked(p,now))return Result::deferred;
        live=npc_combat_.find(source.actor);placed=npcs_.find(source.actor);
        if(live==npc_combat_.end()||placed==npcs_.end()||!live->second.matches(life)||
           !live->second.owns_activated_skill_action(life,token)||live->second.control_epoch(life)!=token.epoch)return Result::stopped;
        const auto body=live->second.actor(life);
        if(!body||!body->health||body->health->current<=0||!live->second.hostile_to_players(life)||
           !p.npc_publication_ready||p.precise_health<=0||p.surface.plane!=placed->second.plane)return stop();
        const auto position=healing_position_locked(p,now);if(!position)return Result::deferred;
        const auto snapshot=ordinary_npc_snapshot_locked(p,placed->second,live->second,now);
        if(snapshot.distance>reviewed.effect_range()||!snapshot.line_of_sight)return stop();
        if(reviewed.empathy_effect)return complete_npc_empathy_followon_locked(token,target,*reviewed.empathy_effect,now);
        const unsigned level=placed->second.definition.level;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(reviewed.effect.damage),level,reviewed.slot.skill};
        const auto actor_at=[&](Player& current,combat::Health health){return incoming_spell_actor_locked(current,
            {current.id,current.effect_scope.generation,health,{},current.effects,true},now);};
        if(!admit_player_damage_sequence_locked(p,{damage},now,actor_at))return Result::deferred;
        live=npc_combat_.find(source.actor);placed=npcs_.find(source.actor);
        if(live==npc_combat_.end()||placed==npcs_.end()||!live->second.matches(life)||
           !live->second.owns_activated_skill_action(life,token)||live->second.control_epoch(life)!=token.epoch)return Result::stopped;
        const auto admitted_source=live->second.actor(life);
        if(!admitted_source||!admitted_source->health||admitted_source->health->current<=0)return stop();
        // Spells receive spell reactions; Vile Touch is a non-spell skill and
        // must never gain Ether Renewal/Aura/Succor or trigger spell-only rules.
        const bool spell=*combat::ai::activated::kind(reviewed.slot.skill)==combat::ai::activated::Kind::spell;
        const auto aura=spell?prepare_npc_aura_completion_locked(token,now):std::optional<NpcAuraCompletion>{};
        if(spell){const auto terminal=complete_npc_spell_terminal_locked(token,now);
            if(terminal==NpcSpellTerminal::unavailable)return Result::deferred;
            if(terminal==NpcSpellTerminal::reaction_stopped)return Result::completed;
        }else{
            live=npc_combat_.find(source.actor);
            if(live==npc_combat_.end()||!live->second.matches(life)||!live->second.complete_action(life,token,now))return Result::stopped;
        }
        live=npc_combat_.find(source.actor);victim=players_.find(target.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||victim==players_.end()||victim->second.effect_scope!=target)return Result::completed;
        const auto alive=live->second.actor(life);if(!alive||!alive->health||alive->health->current<=0)return Result::completed;
        auto& current=victim->second;if(!current.resources||current.precise_health<=0)return Result::completed;
        auto recipient=actor_at(current,{current.precise_health,current.resources->max_health});
        const auto hit=combat::apply(recipient,damage,true);
        publish_npc_immediate_caster_body_followon_locked(source,reviewed.slot.skill);
        publish_target_body_locked(target.actor,reviewed.slot.skill);
        if(!commit_player_reactive_hit_locked(current,hit,source.actor,now,false,nullptr,true,false,nullptr,source))
            throw std::logic_error("admitted NPC immediate damage changed");
        if(spell){if(current.passive_recovery)current.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            finish_npc_aura_completion_locked(aura,now);
            if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC immediate Succor completion failed");}
        completed_skill_action_locked(source,reviewed.slot.skill,now);
        return Result::completed;
    }
