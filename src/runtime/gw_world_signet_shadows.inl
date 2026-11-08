// Included within Instance. Signet damage: no spell-use/Backfire/Soul Barbs trigger.
public:
    bool signet_shadows(skills::effects::Scope scope,skills::effects::Scope target,
            const skills::effects::SignetShadowsPlan& plan,unsigned level,std::uint64_t now,
            std::vector<Packet>* visuals,std::function<void()>* publication=nullptr){
        if(visuals)visuals->clear();if(publication)*publication={};
        const auto canonical=skills::effects::signet_shadows(plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255||(publication&&!visuals))return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto a=players_.find(scope.actor);
        if(a==players_.end()||a->second.effect_scope!=scope||!a->second.resources||
           !resurrection_clock_valid(a->second,now)||!advance_health_locked(a->second,now)||
           a->second.precise_health<=0||a->second.knocked_down_until>now)return false;
        auto& source=a->second;const auto origin=healing_position_locked(source,now);if(!origin)return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        const auto begin_publication=[&]{
            if(!publication)return;
            std::set<const Outbox*> seen;
            for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto&[box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        };
        const auto finish_publication=[&]{
            captures.clear();
            if(publication&&!queues.empty())defer_or_publish_completion_locked(publication,[queues=std::move(queues)]()mutable{
                for(auto&[box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();
            });
        };
        const auto publish_impact=[&]{
            const auto body=skills::effects::signet_shadows_impact_2022(scope.actor,target.actor);
            if(visuals)visuals->push_back(body);else for(auto&[id,peer]:players_)peer.outbox->enqueue({body});
        };
        const auto record_source=[&]{if(source.passive_recovery)source.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);};
        if(const auto found=players_.find(target.actor);found!=players_.end()){
            auto& victim=found->second;
            if(victim.effect_scope!=target||!victim.resources||!victim.effect_registry||!victim.effect_registry->matches(target)||
               !source.team_token||!victim.team_token||source.team_token==victim.team_token||
               source.surface.plane!=victim.surface.plane||!resurrection_clock_valid(victim,now)||
               !advance_health_locked(victim,now)||victim.precise_health<=0)return false;
            const auto point=healing_position_locked(victim,now);
            if(!point||std::hypot(point->x-origin->x,point->z-origin->z)>plan.range)return false;
            // Nonprojectile signet: terrain walkability is not a hit gate.
            // Preserve authoritative range/plane checks, but do not require a
            // traversable straight path across disconnected ground cells.
            // Despite its historical name this helper only prepares general
            // direct-damage modifiers; it does not run spell-only reactions.
            auto actor=incoming_spell_actor_locked(victim,{victim.id,target.generation,
                combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true},now);
            const auto result=skills::effects::signet_shadows_target(plan,actor,victim.conditions,level,true,now);
            if(!result||!preflight_player_reversal_locked(victim,result->hit,now))return false;
            const auto seed=prepare_player_reactive_seed_locked(victim,result->hit,now);
            if(!seed)return false;
            begin_publication();record_source();publish_impact();
            if(!commit_player_reactive_hit_locked(victim,result->hit,scope.actor,now,false,&*seed))
                throw std::logic_error("preflighted Signet of Shadows player damage failed");
            if(victim.passive_recovery)victim.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            finish_publication();return true;
        }
        const auto npc=npcs_.find(target.actor);const auto found=npc_combat_.find(target.actor);
        if(npc==npcs_.end()||found==npc_combat_.end()||found->second.hex_scope()!=target||
           !found->second.hostile_to_players(found->second.lifetime())||npc->second.plane!=source.surface.plane||
           found->second.health_clock(found->second.lifetime()).value_or(0)>now)return false;
        auto& live=found->second;const auto life=live.lifetime();
        refresh_npc_movement_locked(target.actor,now);advance_npc_conditions_locked(target.actor,live,now);
        const auto* actor=live.actor(life);
        if(!actor||!actor->damageable||!actor->health||actor->health->current<=0||
           std::hypot(npc->second.x-origin->x,npc->second.y-origin->z)>plan.range)return false;
        const auto conditions=live.condition_snapshot(life);if(!conditions)return false;
        const auto prepared=skills::effects::signet_shadows_target(plan,*actor,*conditions,level,true,now);if(!prepared)return false;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(plan.damage+(prepared->blinded?plan.blind_bonus:0)),level,876};
        // LiveActor applies its own authoritative protection state; preview
        // that same path before any output or committed damage.
        auto preview=live;if(!preflight_npc_reversal_or_damage_locked(preview,damage,now))return false;
        begin_publication();record_source();publish_impact();
        if(!commit_npc_reversal_or_damage_locked(source,npc->second,live,damage,now))
            throw std::logic_error("preflighted Signet of Shadows damage failed");
        finish_publication();return true;
    }
