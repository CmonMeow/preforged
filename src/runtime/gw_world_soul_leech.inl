// Included inside Instance. Trusted application; normal casting remains gated.
public:
    bool stage_soul_leech(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::SoulLeechPlan& plan,std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();const auto canonical=skills::effects::soul_leech(plan.rank);
        if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);if(!settle_timed_effects_locked(now))return false;
        auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.resources||!caster->second.effect_registry||
           !advance_resource_event_locked(caster->second,now)||caster->second.precise_health<=0||caster->second.knocked_down_until>now)return false;
        const auto duration=skills::effects::hex_duration(128,plan.duration,source,caster->second.effect_registry->active(source,now),now);
        const auto origin=healing_position_locked(caster->second,now);if(!duration||!origin)return false;
        auto player=players_.find(target.actor);auto npc=npc_combat_.find(target.actor);
        unsigned effect=0;
        if(player!=players_.end()){
            auto& p=player->second;
            if(p.effect_scope!=target||!p.resources||!p.effect_registry||!p.team_token||!caster->second.team_token||
               p.team_token==caster->second.team_token||p.surface.plane!=caster->second.surface.plane||
               p.next_health_effect>UINT32_MAX||!advance_resource_event_locked(p,now)||p.precise_health<=0)return false;
            const auto point=healing_position_locked(p,now);if(!point||std::hypot(point->x-origin->x,point->z-origin->z)>plan.range)return false;
            if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,*point);if(!line||line->plane!=p.surface.plane)return false;}
            const auto blocked=prevent_selected_hex_locked(caster->second,p,128,now,false);if(!blocked)return false;
            if(*blocked){const auto done=prevent_selected_hex_locked(caster->second,p,128,now);return done&&*done;}
            effect=static_cast<unsigned>(p.next_health_effect);
            const auto application=skills::effects::prepare_soul_leech_application(*p.effect_registry,source,target,effect,plan,now,duration);
            if(!application)return false;
            *p.effect_registry=application->registry;++p.next_health_effect;
            std::vector<Packet> packets;
            for(const auto& old:application->change.removed){p.health_effects.erase(old.instance);packets.push_back(health_effect_remove(p.id,old.instance));}
            p.health_effects.emplace(effect,application->entry);packets.push_back(health_effect_add(application->entry,p.id,now));
            soul_leech_credit_[target.actor]={source,target,effect,now};
            for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
            commit_applications_locked(target,now);publish_status_locked(p,now);
        }else{
            const auto body=npcs_.find(target.actor);
            if(npc==npc_combat_.end()||body==npcs_.end()||npc->second.hex_scope()!=target)return false;
            advance_npc_conditions_locked(target.actor,npc->second,now);refresh_npc_movement_locked(target.actor,now);
            npc=npc_combat_.find(target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target||body->second.plane!=caster->second.surface.plane||
               std::hypot(body->second.x-origin->x,body->second.y-origin->z)>plan.range||next_npc_hex_==UINT32_MAX)return false;
            if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,{body->second.x,body->second.y});if(!line||line->plane!=body->second.plane)return false;}
            effect=next_npc_hex_;
            if(!npc->second.apply_soul_leech(npc->second.lifetime(),source,plan,effect,now,duration))return false;
            ++next_npc_hex_;soul_leech_credit_[target.actor]={source,target,effect,now};
            for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,npc->second.actor(npc->second.lifetime())->effects)});
            commit_applications_locked(target,now);
        }
        publish_target_body_locked(target.actor,128,source.actor,visuals);
        return true;
    }
private:
    // Called only after the shared spell activation cursor has reserved event.
    // Current private ordering is Soul Leech before Backfire; retail combined
    // punishment ordering and original-caster death policy remain unverified.
    bool soul_leech_spell_locked(skills::effects::Scope target,unsigned spell,std::uint64_t event,std::uint64_t now){
        const auto credit=soul_leech_credit_.find(target.actor);
        if(credit==soul_leech_credit_.end()||credit->second.target!=target)return true;
        const auto owner=credit->second;
        auto caster=players_.find(owner.caster.actor);auto player=players_.find(target.actor);auto npc=npc_combat_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=owner.caster||!caster->second.resources||
           now<caster->second.health_updated||!advance_health_locked(caster->second,now)||caster->second.precise_health<=0)return true;
        const auto converted=[&](unsigned amount)->bool{
            if(!commit_reversal_life_steal_transfer_locked(owner.caster,target,amount,now))return false;
            const auto current_caster=players_.find(owner.caster.actor);
            if(current_caster!=players_.end()&&current_caster->second.effect_scope==owner.caster&&current_caster->second.precise_health<=0)
                kill_locked(current_caster->second,now);
            if(const auto recipient=players_.find(target.actor);recipient!=players_.end())
                return recipient->second.effect_scope==target&&recipient->second.precise_health>0;
            const auto recipient=npc_combat_.find(target.actor);
            if(recipient==npc_combat_.end()||recipient->second.hex_scope()!=target)return false;
            const auto* actor=recipient->second.actor(recipient->second.lifetime());
            return actor&&actor->health&&actor->health->current>0;
        };
        std::optional<skills::effects::SoulLeechTransfer> transfer;
        if(player!=players_.end()){
            auto& p=player->second;if(p.effect_scope!=target||!p.effect_registry||!p.resources)return false;
            if(!skills::effects::soul_leech_reaction(*p.effect_registry,target,spell,now))return true;
            if(!advance_health_locked(p,now)||p.precise_health<=0)return false;
            if(!skills::effects::soul_leech_reaction(*p.effect_registry,target,spell,now))return true;
            transfer=skills::effects::prepare_soul_leech_transfer(*p.effect_registry,owner,caster->second.effect_scope,target,spell,event,now,
                {caster->second.precise_health,caster->second.resources->max_health},{p.precise_health,p.resources->max_health},shielding_reduction_locked(p,now));
            if(!transfer)return false;
            if(p.effect_registry->newest_skill(target,307,now)){
                const auto reaction=skills::effects::soul_leech_reaction(*p.effect_registry,target,spell,now);
                if(!reaction)return false;
                // A zero reduced input does not consume Reversal or create a
                // Health event. Use the raw plan amount, not an overkill-capped
                // transfer, when quoting a positive conversion.
                if(double(reaction->plan.amount)<=shielding_reduction_locked(p,now))return true;
                return converted(reaction->plan.amount);
            }
            health_event_locked(p,transfer->health.target.current-p.precise_health,now);
            if(p.passive_recovery&&transfer->health.stolen)p.passive_recovery->record(recovery::Event::health_loss,now);
        }else{
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
            if(!npc->second.soul_leech_reaction(npc->second.lifetime(),spell,now))return true;
            advance_npc_conditions_locked(target.actor,npc->second,now);
            npc=npc_combat_.find(target.actor);
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
            const auto* actor=npc->second.actor(npc->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)return false;
            if(!npc->second.soul_leech_reaction(npc->second.lifetime(),spell,now))return true;
            const auto registry=npc->second.owned_effect_snapshot(npc->second.lifetime());
            if(registry&&registry->newest_skill(target,307,now)){
                const auto reaction=skills::effects::soul_leech_reaction(*registry,target,spell,now);
                const auto reduction=npc->second.life_steal_reduction(npc->second.lifetime(),now);
                if(!reaction||!reduction)return false;
                // Keep ownership/event qualification before mutating either
                // actor; the ordinary NPC transfer is not conversion-aware.
                const auto qualified=skills::effects::prepare_soul_leech_transfer(*registry,owner,caster->second.effect_scope,target,spell,event,now,
                    {caster->second.precise_health,caster->second.resources->max_health},*actor->health,*reduction);
                if(!qualified)return false;
                if(double(reaction->plan.amount)<=*reduction)return true;
                return converted(reaction->plan.amount);
            }
            transfer=npc->second.apply_soul_leech_transfer(npc->second.lifetime(),owner,caster->second.effect_scope,spell,event,now,
                {caster->second.precise_health,caster->second.resources->max_health});
            if(!transfer)return false;
        }
        health_event_locked(caster->second,transfer->health.source.current-caster->second.precise_health,now);
        transfer->health.source.current=caster->second.precise_health;
        if(player!=players_.end()){transfer->health.target.current=player->second.precise_health;transfer->health.killed=player->second.precise_health<=0;}
        auto packets=skills::effects::life_steal_packets(owner.caster.actor,target.actor,transfer->health);
        if(transfer->health.killed&&npc!=npc_combat_.end())packets.push_back(combat::detail::effects(target.actor,npc->second.actor(npc->second.lifetime())->effects));
        for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        if(transfer->health.killed){
            if(player!=players_.end())kill_locked(player->second,now);
            else{const auto body=npcs_.find(target.actor);if(body!=npcs_.end())record_npc_death_locked(caster->second,body->second,npc->second,now);}
        }
        if(caster->second.precise_health<=0)kill_locked(caster->second,now);
        return !transfer->health.killed;
    }
public:
