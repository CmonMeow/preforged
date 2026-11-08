public:
    // A present result is a successful spell; its value reports a struck foe
    // for the intrinsic recharge factor. Empty space still grants the buff.
    std::optional<bool> flame_djinn_haste(skills::effects::Scope owner,
            const skills::effects::FlameDjinnHastePlan& plan,unsigned level,std::uint64_t now,
            const skills::effects::EnchantmentDuration* adjusted=nullptr,std::optional<std::uint64_t> completion_event={}){
        const auto canonical=skills::effects::flame_djinn_haste(plan.rank);
        const auto duration=canonical?(adjusted?adjusted->for_enchantment(1381,plan.duration,owner,now):std::optional(plan.duration)):std::nullopt;
        if(!canonical||*canonical!=plan||!duration||!level||level>255||now>UINT64_MAX-*duration||(completion_event&&!*completion_event))return {};
        std::lock_guard lock(mutex_);const auto found=players_.find(owner.actor);
        if(found==players_.end()||found->second.effect_scope!=owner)return {};
        auto& caster=found->second;
        if(!caster.resources||!caster.effect_registry||!caster.health_regeneration||!caster.team_token||
           now<caster.health_updated||now<caster.energy_updated||!advance_resource_event_locked(caster,now)||
           caster.precise_health<=0||caster.knocked_down_until>now||caster.next_health_effect>UINT32_MAX)return {};
        const auto origin=healing_position_locked(caster,now);if(!origin)return {};
        auto effect=static_cast<unsigned>(caster.next_health_effect);
        skills::effects::Entry entry{effect,owner.actor,1381,plan.rank,skills::effects::Category::enchantment,now,now+*duration};
        auto registry=*caster.effect_registry;auto timers=*caster.health_regeneration;
        auto change=registry.apply(owner,entry,skills::effects::Replace::same_skill);if(!change.accepted)return {};
        for(const auto& old:change.removed)timers.remove(owner,old.instance);
        if(!timers.apply(owner,{effect,owner.actor,1381,0,now,*entry.expires}))return {};
        const combat::Damage damage{combat::Kind::fire,double(plan.damage),level,1381};
        std::vector<skills::effects::Scope> player_targets,npc_targets;
        const auto player_hit=[&](Player& p){
            auto actor=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,p.reviewed_lightning_armor,p.reviewed_fire_armor,p.reviewed_cold_armor},now);
            return combat::apply(actor,damage,true);
        };
        for(auto&[id,p]:players_){
            if(!p.resources||!p.team_token||p.team_token==caster.team_token||p.surface.plane!=caster.surface.plane)continue;
            const auto point=healing_position_locked(p,now);if(!point)return {};
            if(std::hypot(point->x-origin->x,point->z-origin->z)>plan.radius)continue;
            if(!advance_health_locked(p,now))return {};if(p.precise_health<=0)continue;
            const auto hit=player_hit(p);
            if(hit.reason!=combat::Reason::accepted||!prepare_player_reactive_seed_locked(p,hit,now))return {};
            player_targets.push_back(p.effect_scope);
        }
        for(auto&[id,live]:npc_combat_){
            const auto body=npcs_.find(id);const auto life=live.lifetime();
            if(body==npcs_.end()||body->second.plane!=caster.surface.plane||!live.hostile_to_players(life))continue;
            refresh_npc_movement_locked(id,now);
            if(std::hypot(body->second.x-origin->x,body->second.y-origin->z)>plan.radius)continue;
            if(now<live.health_clock(life).value_or(now))return {};
            advance_npc_conditions_locked(id,live,now);const auto* actor=live.actor(life);
            if(!actor||!actor->health||actor->health->current<=0||!actor->damageable)continue;
            auto proposed=live;if(!preflight_npc_reversal_or_damage_locked(proposed,damage,now))return {};
            npc_targets.push_back(live.hex_scope());
        }
        if(completion_event&&!complete_ether_renewal_locked(owner,1381,*completion_event,now,true))return {};
        // Settle movement/end callbacks before rebuilding the final owned
        // application. Admission may have changed unrelated caster effects.
        advance_movement_effects_locked(caster,now);
        if(caster.precise_health>0){
            if(caster.next_health_effect>UINT32_MAX)return {};
            effect=static_cast<unsigned>(caster.next_health_effect);entry.instance=effect;
            registry=*caster.effect_registry;timers=*caster.health_regeneration;
            change=registry.apply(owner,entry,skills::effects::Replace::same_skill);if(!change.accepted)return {};
            for(const auto& old:change.removed)timers.remove(owner,old.instance);
            if(!timers.apply(owner,{effect,owner.actor,1381,0,now,*entry.expires}))return {};
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;std::set<const Outbox*> seen;
        for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto&[box,queue]:recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        bool struck=false;
        if(caster.precise_health>0){
            // Untargeted enchantments work within Well of the Profane. The
            // existing well boundary still owns stripping on actual entry.
            *caster.effect_registry=std::move(registry);*caster.health_regeneration=std::move(timers);++caster.next_health_effect;
            for(const auto& old:change.removed){caster.health_effects.erase(old.instance);caster.movement_effects.remove(old.instance,now);
                for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_remove(owner.actor,old.instance)});}
            caster.health_effects.emplace(effect,entry);caster.registered_movement=true;caster.movement_effects.apply(entry);
            advance_movement_effects_locked(caster,now);
            for(auto&[id,peer]:players_)peer.outbox->enqueue({health_effect_add(entry,owner.actor,now)});
            commit_applications_locked(owner,now);publish_status_locked(caster,now);
            for(const auto target:player_targets){
                auto& p=players_.at(target.actor);if(p.effect_scope!=target||p.precise_health<=0)continue;
                const auto hit=player_hit(p);
                if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(p,hit,owner.actor,now))
                    throw std::logic_error("preflighted Flame Djinn player damage failed");
                struck=true;
                if(p.passive_recovery)p.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            }
            for(const auto target:npc_targets){
                auto& live=npc_combat_.at(target.actor);const auto life=live.lifetime();
                if(live.hex_scope()!=target||!live.actor(life)->health||live.actor(life)->health->current<=0)continue;
                if(!commit_npc_reversal_or_damage_locked(caster,npcs_.at(target.actor),live,damage,now))
                    throw std::logic_error("preflighted Flame Djinn NPC damage failed");
                struck=true;
            }
        }
        captures.clear();
        const auto body=combat::detail::integer(21,owner.actor,1638);
        for(auto&[box,queue]:recipients){std::vector<Packet> batch{combat::detail::integer(58,owner.actor,0),body};
            batch.insert(batch.end(),queue.packets.begin(),queue.packets.end());box->enqueue(std::move(batch));}
        dispatch_fevered_boundary_locked(now);return struck;
    }
