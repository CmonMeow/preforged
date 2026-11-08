private:
    void retire_bonds_caster_locked(skills::effects::Scope caster,std::uint64_t now){
        if(const auto pending=npc_bonds_casts_.find(caster.actor);
           pending!=npc_bonds_casts_.end()&&pending->second.token.actor==caster)
            npc_bonds_casts_.erase(pending);
        for(const auto& link:bonds_of_torment_.pending()){
            if(link.caster!=caster)continue;
            const auto target=players_.find(link.target.actor);
            if(target!=players_.end()&&target->second.effect_scope==link.target&&
               target->second.health_effects.contains(link.effect))
                remove_health_effect_locked(target->second,link.effect,now);
        }
        bonds_of_torment_.discard(caster);
    }
    void bonds_npc_caster_damage_locked(skills::effects::Scope caster,double applied,
            std::uint64_t now,std::vector<Packet>* deferred){
        if(!bonds_of_torment_.size()||next_bonds_damage_event_==UINT64_MAX)return;
        const auto intents=bonds_of_torment_.propagate(caster,now,next_bonds_damage_event_++,applied);
        if(!intents)return;
        for(const auto& intent:*intents){
            const auto target=players_.find(intent.target.actor);
            const auto source=npcs_.find(intent.caster.actor);
            if(target==players_.end()||source==npcs_.end()||
               target->second.effect_scope!=intent.target||!target->second.resources||
               !target->second.effect_registry||target->second.precise_health<=0||
               !source->second.definition.level)continue;
            auto& p=target->second;
            if(!advance_health_locked(p,now)||p.effect_scope!=intent.target||
               p.precise_health<=0||!p.effect_registry)continue;
            const auto entry=p.effect_registry->lookup(intent.target,intent.effect);
            if(!entry||entry->skill!=1881||entry->kind!=skills::effects::Category::hex||
               entry->cause!=intent.caster.actor||entry->applied>now||
               !entry->expires||now>=*entry->expires)continue;
            auto actor=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
                combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,
                {},{},p.reviewed_cold_armor},now);
            const auto hit=combat::apply(actor,{combat::Kind::cold,
                intent.cold_damage,source->second.definition.level,1881},true);
            if(hit.reason!=combat::Reason::accepted)continue;
            // Resolve the NPC's actual post-armor/protection hit through the
            // same one-use conversion as direct damage. Finalize death below
            // after publishing the final Health, including healing-induced death.
            // Quote Seed from the actual post-protection/conversion loss.
            // Reflected cold damage is direct damage, not a Health transfer.
            const auto seed=prepare_player_reactive_seed_locked(p,hit,now);
            if(!seed||p.effect_scope!=intent.target||p.precise_health<=0)continue;
            std::optional<combat::Hit> actual;
            if(!commit_player_resolved_hit_locked(p,hit,intent.caster.actor,now,&actual,deferred,true))continue;
            if(actual&&actual->applied>0&&seed->effect&&p.effect_registry){
                const auto lease=p.effect_registry->newest_skill(intent.target,274,now);
                if(lease&&lease->instance==seed->effect&&lease->cause==seed->cause)
                    publish_healing_seed_direct_locked(*seed,now);
            }
            if(deferred){
                const auto health=public_health(p);
                deferred->insert(deferred->end(),health.begin(),health.end());
            }else publish_health_locked(p);
            if(p.precise_health<=0)kill_locked(p,now);
        }
    }
    // Caller owns mutex_ and has settled the event frontier. Hex Breaker
    // needs its NPC-source retaliation path and is rejected for this slice.
    bool stage_npc_bonds_of_torment_locked(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::BondsOfTormentPlan& plan,
            std::uint64_t now){
        if(skills::effects::bonds_of_torment_npc()!=plan||source==target)return false;
        const auto placed=npcs_.find(source.actor);
        auto live=npc_combat_.find(source.actor);
        auto selected=players_.find(target.actor);
        if(placed==npcs_.end()||live==npc_combat_.end()||
           selected==players_.end()||live->second.hex_scope()!=source||
           !live->second.hostile_to_players(live->second.lifetime())||
           !placed->second.definition.level||selected->second.effect_scope!=target||
           !selected->second.effect_registry||!selected->second.resources||
           selected->second.next_health_effect>UINT32_MAX)return false;
        advance_npc_conditions_locked(source.actor,live->second,now);
        const auto* caster=live->second.actor(live->second.lifetime());
        if(!caster||!caster->health||caster->health->current<=0)return false;
        auto& p=selected->second;
        if(!advance_health_locked(p,now)||p.precise_health<=0||
           p.effect_registry->newest_skill(target,10,now))return false;
        const auto position=healing_position_locked(p,now);
        if(!position||p.surface.plane!=placed->second.plane||
           std::hypot(position->x-placed->second.x,position->z-placed->second.y)>1248.f)return false;
        if(navigation_){
            const auto line=layered_->trace(p.surface,
                {placed->second.x,placed->second.y},*position);
            if(!line||line->plane!=p.surface.plane)return false;
        }
        const auto effect=static_cast<unsigned>(p.next_health_effect);
        const auto proposed=skills::effects::prepare_bonds_of_torment_application(
            *p.effect_registry,bonds_of_torment_,source,target,effect,plan,now);
        if(!proposed)return false;
        *p.effect_registry=proposed->registry;
        bonds_of_torment_=proposed->ledger;
        ++p.next_health_effect;
        std::vector<Packet> packets;
        for(const auto& old:proposed->change.removed){
            p.health_effects.erase(old.instance);
            packets.push_back(health_effect_remove(p.id,old.instance));
        }
        p.health_effects.emplace(effect,proposed->entry);
        packets.push_back(health_effect_add(proposed->entry,p.id,now));
        for(auto& [id,peer]:players_)peer.outbox->enqueue(packets);
        commit_applications_locked(target,now);
        publish_status_locked(p,now);
        return true;
    }
public:
    bool stage_npc_bonds_of_torment(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::BondsOfTormentPlan& plan,
            std::uint64_t now){
        std::lock_guard lock(mutex_);
        return settle_timed_effects_locked(now)&&
            stage_npc_bonds_of_torment_locked(source,target,plan,now);
    }
private:
    struct NpcBondsCast {
        combat::npcs::control::Token token;
        skills::effects::Scope target;
    };
    std::map<unsigned,NpcBondsCast> npc_bonds_casts_;
    std::set<unsigned> npc_bonds_in_progress_;
    std::map<unsigned,skills::effects::Scope> npc_bonds_ai_;
    std::optional<std::uint64_t> next_npc_bonds_deadline_locked()const{
        std::optional<std::uint64_t> next;
        for(const auto& [id,cast]:npc_bonds_casts_)
            if(!npc_bonds_in_progress_.contains(id)&&
               (!next||cast.token.completes<*next))next=cast.token.completes;
        return next;
    }
    bool dispatch_npc_bonds_locked(std::uint64_t now){
        std::vector<combat::npcs::control::Token> due;
        for(const auto& [id,cast]:npc_bonds_casts_)
            if(!npc_bonds_in_progress_.contains(id)&&cast.token.completes<=now)
                due.push_back(cast.token);
        for(const auto& token:due){
            complete_npc_bonds_of_torment_locked(token,token.completes);
            const auto pending=npc_bonds_casts_.find(token.actor.actor);
            if(pending!=npc_bonds_casts_.end()&&pending->second.token==token)return false;
        }
        return true;
    }
    void drive_npc_bonds_ai_locked(std::uint64_t now){
        // Opt-in encounter policy, not an inferred ArenaNet targeting rule.
        // Prefer the nearest visible, unhexed player; the paid cast path
        // rechecks every admission fact before spending Energy.
        std::erase_if(npc_bonds_ai_,[&](const auto& binding){
            const auto live=npc_combat_.find(binding.first);
            return live==npc_combat_.end()||live->second.hex_scope()!=binding.second;
        });
        for(const auto& [id,source]:npc_bonds_ai_){
            auto live=npc_combat_.find(id);
            const auto placed=npcs_.find(id);
            if(live==npc_combat_.end()||placed==npcs_.end())continue;
            const auto life=live->second.lifetime();
            if(now<live->second.health_clock(life).value_or(now)||
               now<live->second.energy_clock(life).value_or(now))continue;
            advance_npc_conditions_locked(id,live->second,now);
            live=npc_combat_.find(id);
            if(live==npc_combat_.end()||!live->second.matches(life)||
               live->second.active_action(life,now)||!live->second.advance_energy(life,now))continue;
            std::optional<skills::effects::Scope> choice;
            double nearest=1248;
            for(auto& [target_id,p]:players_){
                if(!p.npc_publication_ready||!p.resources||!p.effect_registry||
                   p.precise_health<=0||p.effect_registry->newest_skill(p.effect_scope,1881,now))continue;
                const auto facts=ordinary_npc_snapshot_locked(p,placed->second,live->second,now);
                if(!facts.target_alive||!facts.alive||!facts.line_of_sight||
                   p.surface.plane!=placed->second.plane||facts.distance>nearest)continue;
                if(choice&&facts.distance==nearest&&target_id>=choice->actor)continue;
                choice=p.effect_scope;nearest=facts.distance;
            }
            if(choice)begin_npc_bonds_of_torment_locked(source,*choice,now);
        }
    }
public:
    bool configure_npc_bonds_ai(skills::effects::Scope source){
        std::lock_guard lock(mutex_);
        const auto live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source||
           npc_bonds_ai_.contains(source.actor))return false;
        const auto life=live->second.lifetime();
        if(!live->second.hostile_to_players(life)||
           !live->second.spell_recharge_until(life,{1881,0},0))return false;
        npc_bonds_ai_.emplace(source.actor,source);
        return true;
    }
    // Explicit reviewed loadout only. The spellbook owns paid Energy,
    // activation, recharge and interruption.
    std::optional<combat::npcs::control::Token> begin_npc_bonds_of_torment(
            skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        if(!skills::effects::bonds_of_torment_npc())return {};
        std::lock_guard lock(mutex_);
        return begin_npc_bonds_of_torment_locked(source,target,now);
    }
private:
    std::optional<combat::npcs::control::Token> begin_npc_bonds_of_torment_locked(
            skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        if(!settle_timed_effects_locked(now)||!settle_mind_wrack_caster_locked(source,now))return {};
        auto live=npc_combat_.find(source.actor);
        const auto placed=npcs_.find(source.actor);
        const auto selected=players_.find(target.actor);
        if(live==npc_combat_.end()||placed==npcs_.end()||selected==players_.end()||
           live->second.hex_scope()!=source||selected->second.effect_scope!=target||
           !selected->second.npc_publication_ready||
           !live->second.hostile_to_players(live->second.lifetime()))return {};
        const auto life=live->second.lifetime();
        if(const auto old=npc_bonds_casts_.find(source.actor);old!=npc_bonds_casts_.end()){
            if(live->second.owns_spell_action(life,old->second.token))return {};
            npc_bonds_casts_.erase(old);
        }
        if(!advance_health_locked(selected->second,now))return {};
        advance_npc_conditions_locked(source.actor,live->second,now);
        refresh_npc_movement_locked(source.actor,now);
        const auto snapshot=ordinary_npc_snapshot_locked(selected->second,placed->second,live->second,now);
        const combat::ai::SpellTarget facts{target,float(snapshot.distance),
            selected->second.precise_health>0,true,
            selected->second.surface.plane==placed->second.plane,snapshot.line_of_sight};
        const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at){
            return skill_disabled_slot_locked(source,slot,at);
        };
        const auto choice=live->second.choose_spell(life,facts,now,available);
        if(!choice||choice->skill!=1881)return {};
        const auto before=live->second.energy(life);
        const auto units=live->second.precise_energy_units(life);
        const auto token=live->second.begin_spell(life,*choice,facts,now,available);
        if(!token)return {};
        npc_bonds_casts_[source.actor]={*token,target};
        publish_npc_enchantment_ends_locked(live->second);
        if(before)observe_npc_mind_wrack_energy_locked(source,
            {before->current,before->maximum,0,units?unsigned(*units%300000):0},now);
        live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||
           live->second.actor(life)->health->current<=0){
            npc_bonds_casts_.erase(source.actor);return token;
        }
        if(quest_npc_motion_.erase(source.actor)){
            const auto& body=placed->second;
            for(auto& [id,peer]:players_)if(peer.npc_publication_ready)
                peer.outbox->enqueue({position_packet(source.actor,body.x,body.y,body.plane),
                                      stop_packet(source.actor)});
        }
        npc_flare_event_locked(60,source.actor,1881,target.actor);
        return token;
    }
public:
    bool complete_npc_bonds_of_torment(const combat::npcs::control::Token& token,
            std::uint64_t now){
        if(now<token.completes||!token.skill||token.skill->skill!=1881)return false;
        std::lock_guard lock(mutex_);
        return complete_npc_bonds_of_torment_locked(token,token.completes);
    }
private:
    bool complete_npc_bonds_of_torment_locked(const combat::npcs::control::Token& token,
            std::uint64_t now){
        const auto pending=npc_bonds_casts_.find(token.actor.actor);
        if(pending==npc_bonds_casts_.end()||pending->second.token!=token||
           now<token.completes||npc_bonds_in_progress_.contains(token.actor.actor))return false;
        npc_bonds_in_progress_.insert(token.actor.actor);
        struct Reset{std::set<unsigned>& active;unsigned actor;
            ~Reset(){active.erase(actor);}} reset{npc_bonds_in_progress_,token.actor.actor};
        const auto selected=pending->second.target;
        auto live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=token.actor){
            npc_bonds_casts_.erase(pending);return false;
        }
        const auto life=live->second.lifetime();
        if(!live->second.owns_spell_action(life,token)||
           live->second.control_epoch(life)!=token.epoch){
            npc_bonds_casts_.erase(pending);return false;
        }
        advance_npc_conditions_locked(token.actor.actor,live->second,token.completes);
        live=npc_combat_.find(token.actor.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||
           !live->second.actor(life)||!live->second.actor(life)->health||
           live->second.actor(life)->health->current<=0||
           live->second.control_epoch(life)!=token.epoch){
            npc_bonds_casts_.erase(token.actor.actor);return false;
        }
        const auto player=players_.find(selected.actor);
        const auto body=npcs_.find(token.actor.actor);
        if(player==players_.end()||body==npcs_.end()||
           player->second.effect_scope!=selected||
           !advance_health_locked(player->second,token.completes))return false;
        const auto state=ordinary_npc_snapshot_locked(player->second,body->second,
            live->second,token.completes);
        if(!state.target_alive||!state.alive||!state.line_of_sight||
           player->second.surface.plane!=body->second.plane||state.distance>1248){
            if(!live->second.fail_target_action(life,token,token.completes))return false;
            npc_bonds_casts_.erase(token.actor.actor);
            npc_flare_event_locked(59,token.actor.actor);return true;
        }
        const auto aura=prepare_npc_aura_completion_locked(token,token.completes);
        const auto terminal=complete_npc_spell_terminal_locked(token,token.completes);
        if(terminal==NpcSpellTerminal::unavailable)return false;
        npc_bonds_casts_.erase(token.actor.actor);
        npc_flare_event_locked(58,token.actor.actor);
        if(terminal==NpcSpellTerminal::reaction_stopped)return true;
        const auto plan=*skills::effects::bonds_of_torment_npc();
        const bool applied=stage_npc_bonds_of_torment_locked(token.actor,selected,
            plan,token.completes);
        finish_npc_aura_completion_locked(aura,token.completes);
        if(applied&&!succor_npc_spell_completed_locked(token,token.completes))throw std::logic_error("NPC Bonds Succor completion failed");
        return applied;
    }
