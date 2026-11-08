// Included within Instance. Predicate facts are sampled after shared spell-use reactions.
public:
    bool discord(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::Discord& plan,unsigned level,
                 std::uint64_t now,std::vector<Packet>* visuals,std::function<void()>* publication=nullptr){
        if(visuals)visuals->clear();if(publication)*publication={};if(publication&&!visuals)return false;const auto damage=skills::effects::discord_damage(plan,level);if(!damage)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto a=players_.find(scope.actor);
        if(a==players_.end()||a->second.effect_scope!=scope||!a->second.resources||!resurrection_clock_valid(a->second,now)||
            !advance_health_locked(a->second,now)||a->second.precise_health<=0||a->second.knocked_down_until>now)return false;
        auto& source=a->second;const auto origin=healing_position_locked(source,now);if(!origin)return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        const auto begin_publication=[&]{
            if(!publication)return;
            std::set<const Outbox*> seen;for(auto&[id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto&[box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        };
        const auto finish_publication=[&]{
            captures.clear();if(publication&&!queues.empty())defer_or_publish_completion_locked(publication,[queues=std::move(queues)]()mutable{for(auto&[box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();});
        };
        const auto publish=[&]{if(const auto body=skills::particles::targeted_body_2022(target.actor,scope.actor,plan.skill)){
            if(visuals)visuals->push_back(*body);else for(auto&[id,peer]:players_)peer.outbox->enqueue({*body});}};
        if(const auto found=players_.find(target.actor);found!=players_.end()){
            auto& p=found->second;
            if(p.effect_scope!=target||!p.resources||!p.effect_registry||!p.effect_registry->matches(target)||!source.team_token||!p.team_token||
                source.team_token==p.team_token||source.surface.plane!=p.surface.plane||!resurrection_clock_valid(p,now)||!advance_health_locked(p,now)||p.precise_health<=0)return false;
            const auto point=healing_position_locked(p,now);if(!point||std::hypot(point->x-origin->x,point->z-origin->z)>1248)return false;
            if(navigation_){const auto line=layered_->trace(source.surface,*origin,*point);if(!line||line->plane!=p.surface.plane)return false;}
            const auto ready=skills::effects::discord_ready(p.conditions.count(now)>0,p.effect_registry->has(target,skills::effects::Category::hex,now),p.effect_registry->has(target,skills::effects::Category::enchantment,now));
            if(!ready)return false;
            if(*ready){auto actor=incoming_spell_actor_locked(p,{p.id,target.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},now);
                const auto hit=combat::apply(actor,*damage,true);if(hit.reason!=combat::Reason::accepted||!preflight_player_reversal_locked(p,hit,now))return false;
                const auto seed=prepare_player_reactive_seed_locked(p,hit,now);
                if(!seed)return false;
                begin_publication();
                if(!commit_player_reactive_hit_locked(p,hit,scope.actor,now,false,&*seed))throw std::logic_error("preflighted Discord player damage failed");
                if(p.passive_recovery)p.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            }
            publish();finish_publication();return true; // Valid no-effect casts still succeed and recharge.
        }
        const auto npc=npcs_.find(target.actor);const auto found=npc_combat_.find(target.actor);
        if(npc==npcs_.end()||found==npc_combat_.end()||found->second.hex_scope()!=target||!found->second.hostile_to_players(found->second.lifetime())||
            npc->second.plane!=source.surface.plane||found->second.health_clock(found->second.lifetime()).value_or(0)>now)return false;
        auto& live=found->second;const auto life=live.lifetime();refresh_npc_movement_locked(target.actor,now);advance_npc_conditions_locked(target.actor,live,now);
        const auto* actor=live.actor(life);if(!actor||!actor->damageable||!actor->health||actor->health->current<=0||std::hypot(npc->second.x-origin->x,npc->second.y-origin->z)>1248)return false;
        if(navigation_){const auto line=layered_->trace(source.surface,*origin,{npc->second.x,npc->second.y});if(!line||line->plane!=npc->second.plane)return false;}
        const auto conditions=live.condition_snapshot(life);const auto registry=live.hex_registry_snapshot(life,now);
        if(!conditions||!registry)return false;
        const auto enchantments=live.enchantment_count(life,now);
        const auto ready=skills::effects::discord_ready(conditions->count(now)>0,registry->has(target,skills::effects::Category::hex,now),enchantments?std::optional(*enchantments>0):std::nullopt);
        if(!ready)return false;
        if(*ready){auto preview=live;if(!preflight_npc_reversal_or_damage_locked(preview,*damage,now))return false;
            begin_publication();
            if(!commit_npc_reversal_or_damage_locked(source,npc->second,live,*damage,now))throw std::logic_error("preflighted Discord damage failed");
        }
        publish();finish_publication();return true;
    }
