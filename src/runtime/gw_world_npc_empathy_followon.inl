// Detached NPC caster -> player selected-hex application. No normal auth binding.
private:
    bool apply_npc_empathy_player_followon_locked(skills::effects::Scope caster,skills::effects::Scope target,
            const skills::effects::EmpathyPlan& plan,std::uint64_t now,bool commit,std::optional<unsigned> level_hint={}){
        const auto canonical=skills::effects::empathy(plan.skill,plan.rank);
        if(!canonical||canonical->damage!=plan.damage||canonical->reduction!=plan.reduction||canonical->duration!=plan.duration||now>UINT64_MAX-plan.duration)return false;
        auto source=npc_combat_.find(caster.actor);auto body=npcs_.find(caster.actor);const auto found=players_.find(target.actor);
        if(source==npc_combat_.end()||body==npcs_.end()||source->second.hex_scope()!=caster||found==players_.end()||found->second.effect_scope!=target||caster.instance!=target.instance)return false;
        const auto life=source->second.lifetime();
        if(now<source->second.health_clock(life).value_or(now)||now<source->second.energy_clock(life).value_or(now))return false;
        advance_npc_conditions_locked(caster.actor,source->second,now);refresh_npc_movement_locked(caster.actor,now);
        source=npc_combat_.find(caster.actor);body=npcs_.find(caster.actor);
        if(source==npc_combat_.end()||body==npcs_.end()||!source->second.matches(life)||!source->second.hostile_to_players(life)||source->second.knocked_down(life,now))return false;
        auto* caster_actor=source->second.actor(life);unsigned level=body->second.definition.level;
        if(!caster_actor||!caster_actor->health||caster_actor->health->current<=0||!level||level>255||(level_hint&&*level_hint!=level))return false;
        auto& victim=found->second;
        if(!victim.resources||!victim.effect_registry||!victim.health_regeneration||!victim.team_token||victim.next_health_effect>=UINT32_MAX||!advance_health_locked(victim,now)||victim.precise_health<=0)return false;
        // Target settlement may retire a linked caster; retain only Scope and
        // reacquire the actual NPC body/control/resources before dereferencing.
        source=npc_combat_.find(caster.actor);body=npcs_.find(caster.actor);
        if(source==npc_combat_.end()||body==npcs_.end()||!source->second.matches(life)||!source->second.hostile_to_players(life)||source->second.knocked_down(life,now))return false;
        caster_actor=source->second.actor(life);level=body->second.definition.level;
        if(!caster_actor||!caster_actor->health||caster_actor->health->current<=0||!level||level>255||(level_hint&&*level_hint!=level)||victim.surface.plane!=body->second.plane)return false;
        const auto location=healing_position_locked(victim,now);if(!location)return false;
        const auto facts=ordinary_npc_snapshot_locked(victim,body->second,source->second,now);
        if(!facts.line_of_sight||facts.distance>1248)return false;
        auto registry=*victim.effect_registry;std::vector<skills::effects::Entry> removed;
        const auto stance=registry.newest_skill(target,10,now);std::optional<unsigned> owner_level;
        std::optional<BackfireCredit> prevention_credit;
        if(stance){const auto credit=hex_breaker_credit_.find(victim.id);
            if(credit==hex_breaker_credit_.end()||credit->second.target!=target||credit->second.effect!=stance->instance)return false;
            prevention_credit=credit->second;owner_level=credit->second.level;
        }else for(const auto& old:registry.active(target,now))if(old.skill==26||old.skill==3151){if(!registry.remove(target,old.instance))return false;removed.push_back(old);}
        const skills::effects::Entry entry{static_cast<unsigned>(victim.next_health_effect),caster.actor,plan.skill,plan.rank,skills::effects::Category::hex,now,now+plan.duration};
        const auto selected=skills::effects::selected_hex_application(registry,target,caster,entry,skills::effects::Replace::same_skill,*caster_actor,owner_level,now);if(!selected)return false;
        if(selected->kind==skills::effects::HexApplicationKind::prevented){
            if(!stance||!prevention_credit||!selected->retaliation)return false;
            const combat::Damage retaliation{combat::Kind::armor_ignoring,double(selected->retaliation->reaction.damage),prevention_credit->level,10};
            if(selected->retaliation->hit.reason==combat::Reason::accepted){auto preview=source->second;if(!preflight_npc_reversal_or_damage_locked(preview,retaliation,now))return false;}
            if(!commit)return true;
            *victim.effect_registry=selected->registry;victim.health_regeneration->remove(target,stance->instance);victim.health_effects.erase(stance->instance);
            victim.movement_effects.remove(stance->instance,now);hex_breaker_credit_.erase(victim.id);
            for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(victim.id,stance->instance)});publish_status_locked(victim,now);
            if(victim.passive_recovery)victim.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            if(selected->retaliation->hit.reason==combat::Reason::accepted&&
               !commit_npc_reversal_or_damage_locked(&victim,body->second,source->second,retaliation,now))throw std::logic_error("preflighted NPC Hex Breaker retaliation changed");
            return true;
        }
        if(!commit)return true;
        commit_empathy_player_registration_followon_locked(victim,selected->registry,entry,removed,caster,level,now,true);return true;
    }
    combat::ai::activated::Completion complete_npc_empathy_followon_locked(const combat::npcs::control::Token& token,
            skills::effects::Scope target,const skills::effects::EmpathyPlan& plan,std::uint64_t now){
        using Result=combat::ai::activated::Completion;const auto source=token.actor;
        auto live=npc_combat_.find(source.actor);
        if(!token.skill||token.skill->skill!=plan.skill||now<token.completes||live==npc_combat_.end()||live->second.hex_scope()!=source||!live->second.owns_activated_skill_action(live->second.lifetime(),token))return Result::stopped;
        // Application readiness settles real source/target causal frontiers.
        // A control interval may begin and expire during that settlement: being
        // standing now does not restore the old paid activation identity.
        const auto still_owned=[&](){const auto current=npc_combat_.find(source.actor);
            return current!=npc_combat_.end()&&current->second.hex_scope()==source&&
                current->second.control_epoch(current->second.lifetime())==token.epoch&&
                current->second.owns_activated_skill_action(current->second.lifetime(),token);};
        if(!apply_npc_empathy_player_followon_locked(source,target,plan,now,false))return still_owned()?Result::deferred:Result::stopped;
        if(!still_owned())return Result::stopped;
        const auto aura=prepare_npc_aura_completion_locked(token,now);
        const auto terminal=complete_npc_spell_terminal_locked(token,now);
        if(terminal==NpcSpellTerminal::unavailable)return still_owned()?Result::deferred:Result::stopped;
        if(terminal==NpcSpellTerminal::reaction_stopped)return Result::completed;
        live=npc_combat_.find(source.actor);const auto victim=players_.find(target.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source||victim==players_.end()||victim->second.effect_scope!=target||victim->second.precise_health<=0)return Result::completed;
        const auto actor=live->second.actor(live->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)return Result::completed;
        publish_npc_immediate_caster_body_followon_locked(source,plan.skill);
        if(!apply_npc_empathy_player_followon_locked(source,target,plan,now,true))throw std::logic_error("admitted NPC Empathy application changed");
        live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source)return Result::completed;
        finish_npc_aura_completion_locked(aura,now);
        if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC Empathy Succor completion failed");
        completed_skill_action_locked(source,plan.skill,now);return Result::completed;
    }
private:
