// Discardable shared damage admission. Seed recipient facts are prepared on real
// World authority before copying. Never run World handlers on a copied Player.
private:
    struct DamageAdmissionBatch {
        std::map<skills::effects::Scope,skills::effects::PlayerDamageAdmission> players;
        std::map<skills::effects::Scope,combat::npcs::LiveActor> npcs;
        const std::map<skills::effects::Scope,skills::effects::HealingSeedReaction>* seed_facts=nullptr;
        std::optional<skills::effects::Scope> missing_seed;
    };
    skills::effects::PlayerDamageAdmission* admission_player_locked(
            DamageAdmissionBatch& batch,const Player& player,std::uint64_t now)const{
        using namespace skills::effects;
        if(!player.resources)return nullptr;
        if(const auto existing=batch.players.find(player.effect_scope);existing!=batch.players.end())return &existing->second;
        const auto [it,inserted]=batch.players.try_emplace(player.effect_scope,PlayerDamageAdmission{
            player.effect_registry?*player.effect_registry:Registry(player.effect_scope),
            temporary_health_snapshot_locked(player,now),
            player.shield_absorption_hits.value_or(ShieldAbsorption(player.effect_scope))});
        return &it->second;
    }
    bool project_batch_seed_locked(DamageAdmissionBatch& batch,
            skills::effects::PlayerDamageAdmission& damaged,const std::optional<combat::Hit>& actual,
            std::uint64_t now)const{
        using namespace skills::effects;
        if(!actual||actual->applied<=0)return true;
        const auto target=damaged.health.owner;
        const auto effect=damaged.registry.newest_skill(target,274,now);if(!effect)return true;
        const auto plan=healing_seed(effect->rank);if(!plan)return false;
        if(!batch.seed_facts)return false;
        const auto admitted=batch.seed_facts->find(target);
        if(admitted==batch.seed_facts->end()||admitted->second.effect!=effect->instance||
           admitted->second.cause!=effect->cause||admitted->second.healing!=plan->healing){
            batch.missing_seed=target;return false;
        }
        const auto& seed=admitted->second;
        for(const auto recipient:seed.recipients){
            if(const auto player=players_.find(recipient.actor);player!=players_.end()){
                if(player->second.effect_scope!=recipient)return false;
                auto* state=admission_player_locked(batch,player->second,now);if(!state)return false;
                if(state->health.current<=0)continue;
                // Admission must project the same unmodified Health gain as
                // common publication, including real pending wound debt.
                state->health.capacity.event(state->health.current,seed.healing);
            }else{
                const auto live=npc_combat_.find(recipient.actor);
                if(live==npc_combat_.end()||live->second.hex_scope()!=recipient)return false;
                const auto [it,inserted]=batch.npcs.try_emplace(recipient,live->second);
                auto& proposed=it->second;const auto life=proposed.lifetime();const auto actor=proposed.actor(life);
                if(!actor||!actor->health||actor->health->current<=0)continue;
                if(!proposed.gain_ally_health(life,seed.healing,now))return false;
            }
        }
        return true;
    }
    std::optional<combat::DamageStep> project_batch_player_damage_locked(
            DamageAdmissionBatch& batch,const Player& player,combat::Actor& actor,
            const combat::Damage& damage,std::uint64_t now)const{
        using namespace skills::effects;
        auto* projected=admission_player_locked(batch,player,now);if(!projected)return {};
        auto& state=*projected;auto effective=actor;
        if(effective.cumulative_reduction){
            effective.incoming_reduction-=effective.cumulative_reduction->reduction;
            effective.cumulative_reduction.reset();
        }
        if(state.registry.newest_skill(player.effect_scope,1399,now)){
            const auto quote=state.absorption.prepare(state.registry,player.effect_scope,state.absorption.last_event()+1,now);
            if(!quote)return {};
            effective.incoming_reduction+=quote->reduction;
            effective.cumulative_reduction=ShieldAbsorption::receipt(*quote);
        }
        const auto incoming=combat::apply(effective,damage,true);
        const auto step=project_player_protected_hit(state,incoming,now);if(!step)return {};
        state=step->state;
        if(!project_batch_seed_locked(batch,state,step->actual,now))return {};
        actor.health=combat::Health{state.health.current,state.health.capacity.maximum()};
        actor.effects=state.registry.presentation(player.effect_scope,actor.effects&~0x10u,now)|
            (state.health.current<=0?0x10u:0u);
        return combat::DamageStep{incoming,step->actual};
    }
    bool project_batch_condition_reactions_locked(DamageAdmissionBatch& batch,const Player& player,
            const combat::conditions::State& before,const combat::conditions::State& after,std::uint64_t now)const{
        using namespace skills::effects;
        auto* projected=admission_player_locked(batch,player,now);if(!projected)return false;
        auto& state=*projected;
        for(const auto& event:immediate_condition_transitions(before,after,now)){
            if(state.health.current<=0)break;
            const auto trigger=fragility_reaction(state.registry,player.effect_scope,event);if(!trigger)continue;
            const auto credit=fragility_credit_.find(player.id);
            if(credit==fragility_credit_.end()||credit->second.target!=player.effect_scope||
               credit->second.effect!=trigger->cause.instance)continue;
            auto actor=incoming_spell_actor_locked(player,{player.id,player.effect_scope.generation,
                combat::Health{state.health.current,state.health.capacity.maximum()},{},player.effects,true},now);
            if(!project_batch_player_damage_locked(batch,player,actor,{combat::Kind::armor_ignoring,
                    double(trigger->plan.damage),credit->second.level,trigger->plan.skill},now))return false;
        }
        return true;
    }

    // Admit an ordered direct-damage sequence against copied protection/Health,
    // including Seed feedback between hits. Preparing real recipient timelines
    // can change authority, so discard the batch and restart after each new fact.
    template<class ActorFactory>
    bool admit_player_damage_sequence_locked(Player& player,const std::vector<combat::Damage>& damages,
            std::uint64_t now,ActorFactory actor_at,double* projected_health=nullptr){
        std::map<skills::effects::Scope,skills::effects::HealingSeedReaction> facts;
        const auto scope=player.effect_scope;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            const auto found=players_.find(scope.actor);
            if(found==players_.end()||found->second.effect_scope!=scope||!found->second.resources||found->second.precise_health<=0)return false;
            auto& current=found->second;
            if(damages.empty()){if(projected_health)*projected_health=current.precise_health;return true;}
            const auto kind=damages.front().kind;
            if(std::any_of(damages.begin(),damages.end(),[&](const auto& d){return d.kind!=kind;}))return false;
            auto armor=watch_yourself_armor_sequence_locked(current,kind,now);
            std::optional<combat::Hit> previous;
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            for(const auto& damage:damages){
                auto* state=admission_player_locked(batch,current,now);if(!state)return false;
                if(state->health.current<=0)break;
                auto actor=actor_at(current,combat::Health{state->health.current,state->health.capacity.maximum()});
                if(armor){const auto rating=armor(previous?&*previous:nullptr);if(!rating)return false;
                    const auto effective=current.conditions.armor(*rating,now);
                    if(kind==combat::Kind::cold)actor.cold_armor=effective;
                    else if(kind==combat::Kind::fire)actor.fire_armor=effective;
                    else if(kind==combat::Kind::earth)actor.earth_armor=effective;
                    else if(kind==combat::Kind::lightning)actor.lightning_armor=effective;
                }
                const auto step=project_batch_player_damage_locked(batch,current,actor,damage,now);
                if(!step){valid=false;break;}previous=step->projected;
            }
            if(valid){if(projected_health)*projected_health=batch.players.at(scope).health.current;return true;}
            if(!batch.missing_seed)return false;
            const auto target=players_.find(batch.missing_seed->actor);
            if(target==players_.end()||target->second.effect_scope!=*batch.missing_seed)return false;
            const auto fact=prepare_healing_seed_direct_locked(target->second,1.,now);if(!fact)return false;
            facts[*batch.missing_seed]=*fact;
        }
        return false;
    }

    struct DamagePublication {
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
    };
    void begin_damage_publication_locked(DamagePublication& pending){
        ensure_completion_recipients_locked();
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(!peer.outbox->captured_on_this_thread()&&seen.insert(peer.outbox.get()).second)
            pending.queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        for(auto& [box,queue]:pending.queues)pending.captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
    }
    std::function<void()> finish_damage_publication_locked(DamagePublication& pending){
        pending.captures.clear();if(pending.queues.empty())return {};
        auto queues=std::make_shared<decltype(pending.queues)>(std::move(pending.queues));
        auto sent=std::make_shared<std::atomic_bool>(false);
        return [queues,sent]{if(sent->exchange(true))return;for(auto& [box,queue]:*queues)box->enqueue(std::move(queue.packets));queues->clear();};
    }
    bool commit_player_direct_skill_locked(skills::effects::Scope source,skills::effects::Scope target,
            const combat::Damage& damage,std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto found=players_.find(target.actor);
        if(found==players_.end()||found->second.effect_scope!=target||!found->second.resources)return false;
        if(!admit_player_damage_sequence_locked(found->second,{damage},now,[&](const Player& live,combat::Health health){
            return incoming_spell_actor_locked(live,{live.id,target.generation,health,{},live.effects,true},now,true);
        }))return false;
        const auto current=players_.find(target.actor);
        if(current==players_.end()||current->second.effect_scope!=target||!current->second.resources||current->second.precise_health<=0)return false;
        auto& player=current->second;
        auto actor=incoming_spell_actor_locked(player,{player.id,target.generation,combat::Health{player.precise_health,player.resources->max_health},{},player.effects,true},now,true);
        const auto hit=combat::apply(actor,damage,true);
        if(hit.reason!=combat::Reason::accepted)return false;
        DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        const bool accepted=commit_player_reactive_hit_locked(player,hit,source.actor,now,false,nullptr,false,true,nullptr,source);
        auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();
        return accepted;
    }
