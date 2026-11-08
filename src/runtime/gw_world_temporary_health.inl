private:
    skills::effects::TemporaryHealthSnapshot temporary_health_snapshot_locked(const Player& player,std::uint64_t now)const{
        return {player.effect_scope,now,player.precise_health,player.deep_wound};
    }
    void commit_temporary_health_locked(Player& player,const skills::effects::TemporaryHealthTransition& next){
        const auto previous_maximum=player.resources->max_health;
        player.temporary_health=next.ledger;player.precise_health=next.health.current;player.deep_wound=next.health.capacity;
        player.resources->max_health=player.deep_wound.maximum();player.resources->health=static_cast<unsigned>(std::ceil(player.precise_health));if(previous_maximum!=player.resources->max_health)player.displayed_health_pips.reset();
    }
    bool resize_permanent_health_locked(Player& player,unsigned capacity,std::uint64_t now){
        if(player.temporary_health){const auto next=skills::effects::prepare_permanent_health_resize(*player.temporary_health,temporary_health_snapshot_locked(player,now),capacity,player.conditions.has(combat::conditions::Kind::deep_wound,now));
            if(!next)return false;commit_temporary_health_locked(player,*next);
        }else player.deep_wound.capacity(player.precise_health,capacity,player.conditions.has(combat::conditions::Kind::deep_wound,now));
        return true;
    }
public:
    // Complete both effects from authoritative World actors. Runtime owns
    // instant-shout admission/payment/recharge; this is not a command bypass.
    bool complete_to_limit(skills::effects::Scope source,const skills::effects::ToLimitPlan& plan,std::uint64_t now){
        const auto canonical=skills::effects::to_limit(plan.rank);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source)return false;
        auto& player=found->second;
        // Like other shouts, this is allowed during actions and knockdown.
        if(!player.resources||!advance_resource_event_locked(player,now)||player.precise_health<=0||
           !player.effect_registry||!player.health_regeneration||player.next_health_effect>UINT32_MAX)return false;
        const auto origin=healing_position_locked(player,now);if(!origin)return false;
        unsigned foes=0;
        for(auto& [id,candidate]:players_){
            if(id==source.actor||!player.team_token||!candidate.team_token||candidate.team_token==player.team_token||
               candidate.surface.plane!=player.surface.plane||!candidate.resources)continue;
            if(!advance_resource_event_locked(candidate,now))return false;
            if(candidate.precise_health<=0)continue;
            const auto point=healing_position_locked(candidate,now);if(!point)return false;
            if(std::hypot(point->x-origin->x,point->z-origin->z)<=plan.radius)++foes;
        }
        for(auto& [id,live]:npc_combat_){
            const auto placement=npcs_.find(id);const auto life=live.lifetime();
            if(placement==npcs_.end()||!live.hostile_to_players(life)||placement->second.plane!=player.surface.plane)continue;
            if(now<live.health_clock(life).value_or(now))return false;
            advance_npc_conditions_locked(id,live,now);refresh_npc_movement_locked(id,now);
            const auto actor=live.actor(life);
            if(actor&&actor->health&&actor->health->current>0&&
               std::hypot(placement->second.x-origin->x,placement->second.y-origin->z)<=plan.radius)++foes;
        }
        // Settling another actor may trigger shared reactions on this caster.
        if(player.precise_health<=0)return false;
        const auto before=player.temporary_health.value_or(skills::effects::TemporaryHealth(source,player.deep_wound.base,now));
        const auto effect=static_cast<unsigned>(player.next_health_effect);
        const auto next=skills::effects::prepare_to_limit(before,temporary_health_snapshot_locked(player,now),
            *player.effect_registry,player.adrenaline,plan,effect,foes,player.conditions.has(combat::conditions::Kind::deep_wound,now));
        if(!next)return false;
        auto timers=*player.health_regeneration;
        if(!timers.apply(source,{effect,source.actor,316,0,now,next->applied.expires}))return false;
        commit_temporary_health_locked(player,next->health);*player.effect_registry=next->registry;
        player.adrenaline=next->adrenaline;*player.health_regeneration=std::move(timers);++player.next_health_effect;
        std::vector<Packet> packets{combat::detail::integer(21,source.actor,562)};
        if(next->health.removed){player.health_effects.erase(*next->health.removed);packets.push_back(health_effect_remove(source.actor,*next->health.removed));}
        player.health_effects.emplace(effect,next->applied);packets.push_back(health_effect_add(next->applied,source.actor,now));
        for(auto& [id,peer]:players_){auto batch=packets;
            if(peer.effect_scope==source)for(const auto& pool:player.adrenaline.pools())batch.push_back(combat::adrenaline::snapshot_2022(source.actor,pool));
            const auto vitals=id==source.actor?owner_vitals(player):public_health(player);batch.insert(batch.end(),vitals.begin(),vitals.end());peer.outbox->enqueue(std::move(batch));}
        commit_applications_locked(source,now);settle_conditional_stance_locked(player,now);return true;
    }
    // Trusted effect-only stage for the Health accounting tests. Normal316
    // must atomically include its foe-count/Adrenaline outcome and paid shout.
    bool stage_to_limit_health(skills::effects::Scope source,const skills::effects::ToLimitPlan& plan,std::uint64_t now){
        const auto canonical=skills::effects::to_limit(plan.rank);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source||!found->second.resources||now<found->second.health_updated||!advance_health_locked(found->second,now)||found->second.precise_health<=0)return false;
        auto& player=found->second;if(!player.effect_registry||!player.health_regeneration||player.next_health_effect>UINT32_MAX)return false;
        const auto before=player.temporary_health.value_or(skills::effects::TemporaryHealth(source,player.deep_wound.base,now));
        const auto effect=static_cast<unsigned>(player.next_health_effect);
        const auto next=skills::effects::prepare_to_limit_health(before,temporary_health_snapshot_locked(player,now),plan,effect,player.conditions.has(combat::conditions::Kind::deep_wound,now));if(!next)return false;
        auto registry=*player.effect_registry;auto timers=*player.health_regeneration;
        const skills::effects::Entry entry{effect,source.actor,316,plan.rank,skills::effects::Category::other,now,now+plan.duration};
        if(!registry.apply(source,entry,skills::effects::Replace::same_skill).accepted||!timers.apply(source,{effect,source.actor,316,0,now,now+plan.duration}))return false;
        commit_temporary_health_locked(player,*next);*player.effect_registry=std::move(registry);*player.health_regeneration=std::move(timers);++player.next_health_effect;
        std::vector<Packet> packets;if(next->removed){player.health_effects.erase(*next->removed);packets.push_back(health_effect_remove(source.actor,*next->removed));}
        player.health_effects.emplace(effect,entry);packets.push_back(health_effect_add(entry,source.actor,now));
        for(auto& [id,peer]:players_){auto batch=packets;const auto vitals=id==source.actor?owner_vitals(player):public_health(player);batch.insert(batch.end(),vitals.begin(),vitals.end());peer.outbox->enqueue(std::move(batch));}
        settle_conditional_stance_locked(player,now);return true;
    }

    // Trusted347effect stage;Runtime payment/instant publication is separate.
    bool stage_endure_pain_health(skills::effects::Scope source,const skills::effects::EndurePainPlan& plan,std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto canonical=skills::effects::endure_pain(plan.rank);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source||!found->second.resources||now<found->second.health_updated||!advance_health_locked(found->second,now)||found->second.precise_health<=0)return false;
        auto& player=found->second;if(!player.effect_registry||!player.health_regeneration||player.next_health_effect>UINT32_MAX)return false;
        const auto before=player.temporary_health.value_or(skills::effects::TemporaryHealth(source,player.deep_wound.base,now));
        const auto existing=player.effect_registry->newest_skill(source,347,now);
        if(before.endure_pain_source()){
            const auto& owned=*before.endure_pain_source();const auto entry=player.effect_registry->lookup(source,owned.effect);
            if(!existing||existing->instance!=owned.effect||!entry||entry->cause!=source.actor||entry->rank!=owned.rank||entry->applied!=owned.begins||entry->expires!=owned.ends)return false;
        }else if(existing)return false;
        const auto effect=static_cast<unsigned>(player.next_health_effect);
        const auto next=skills::effects::prepare_endure_pain_health(before,temporary_health_snapshot_locked(player,now),plan,effect,player.conditions.has(combat::conditions::Kind::deep_wound,now));if(!next)return false;
        auto registry=*player.effect_registry;auto timers=*player.health_regeneration;
        const skills::effects::Entry entry{effect,source.actor,347,plan.rank,skills::effects::Category::other,now,now+plan.duration};
        if(!registry.apply(source,entry,skills::effects::Replace::same_skill).accepted||!timers.apply(source,{effect,source.actor,347,0,now,now+plan.duration}))return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        commit_temporary_health_locked(player,*next);*player.effect_registry=std::move(registry);*player.health_regeneration=std::move(timers);++player.next_health_effect;
        std::vector<Packet> packets;if(next->removed){player.health_effects.erase(*next->removed);packets.push_back(health_effect_remove(source.actor,*next->removed));}
        player.health_effects.emplace(effect,entry);packets.push_back(health_effect_add(entry,source.actor,now));
        for(auto& [id,peer]:players_){auto batch=packets;const auto vitals=id==source.actor?owner_vitals(player):public_health(player);batch.insert(batch.end(),vitals.begin(),vitals.end());peer.outbox->enqueue(std::move(batch));}
        settle_conditional_stance_locked(player,now);captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();};
        defer_or_publish_completion_locked(publication,std::move(publish));return true;
    }

    // Trusted effect stage. The Runtime callback supplies paid cast admission;
    // the captured indefinite status is published after cast completion.
    bool stage_stamina_signet_health(skills::effects::Scope source,const skills::effects::StaminaSignetPlan& plan,
                                     std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto canonical=skills::effects::stamina_signet(plan.rank);
        if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source||!found->second.resources||
           now<found->second.health_updated||!advance_resource_event_locked(found->second,now)||
           found->second.precise_health<=0)return false;
        auto& player=found->second;
        if(!player.effect_registry||!player.health_regeneration||player.next_health_effect>UINT32_MAX)return false;
        const auto before=player.temporary_health.value_or(skills::effects::TemporaryHealth(source,player.deep_wound.base,now));
        const auto existing=player.effect_registry->newest_skill(source,1411,now);
        if(before.stamina_signet_source()){
            const auto& owned=*before.stamina_signet_source();const auto entry=player.effect_registry->lookup(source,owned.effect);
            if(!existing||existing->instance!=owned.effect||!entry||entry->cause!=source.actor||
               entry->rank!=owned.rank||entry->applied!=owned.begins||entry->expires)return false;
        }else if(existing)return false;
        const auto effect=static_cast<unsigned>(player.next_health_effect);
        const auto next=skills::effects::prepare_stamina_signet_health(before,temporary_health_snapshot_locked(player,now),plan,effect,
                    player.conditions.has(combat::conditions::Kind::deep_wound,now));
        if(!next)return false;
        auto registry=*player.effect_registry;
        const skills::effects::Entry entry{effect,source.actor,1411,plan.rank,skills::effects::Category::other,now,{}};
        const auto changed=registry.apply(source,entry,skills::effects::Replace::same_skill);
        if(!changed.accepted)return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        commit_temporary_health_locked(player,*next);*player.effect_registry=std::move(registry);++player.next_health_effect;
        std::vector<Packet> packets;
        for(const auto& old:changed.removed){player.health_effects.erase(old.instance);packets.push_back(health_effect_remove(source.actor,old.instance));}
        player.health_effects.emplace(effect,entry);packets.push_back(stamina_signet_status(entry,source.actor));
        for(auto& [id,peer]:players_){auto batch=packets;const auto vitals=id==source.actor?owner_vitals(player):public_health(player);
            batch.insert(batch.end(),vitals.begin(),vitals.end());peer.outbox->enqueue(std::move(batch));}
        settle_conditional_stance_locked(player,now);captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();};
        defer_or_publish_completion_locked(publication,std::move(publish));return true;
    }

    // Trusted family effect transaction. Runtime owns Adrenaline/instant use.
    bool stage_defy_pain_health(skills::effects::Scope source,const skills::effects::DefyPainPlan& plan,std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto canonical=skills::effects::defy_pain(plan.rank,plan.skill==3204);if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);const auto found=players_.find(source.actor);
        if(found==players_.end()||found->second.effect_scope!=source||!found->second.resources||now<found->second.health_updated||!advance_resource_event_locked(found->second,now)||found->second.precise_health<=0)return false;
        auto& player=found->second;if(!player.effect_registry||!player.health_regeneration||player.next_health_effect>UINT32_MAX)return false;
        const auto before=player.temporary_health.value_or(skills::effects::TemporaryHealth(source,player.deep_wound.base,now));
        if(before.defy_pain_source()){
            if(!skills::effects::active_defy_pain(*player.effect_registry,before,source,now))return false;
        }else if(player.effect_registry->newest_skill(source,318,now)||player.effect_registry->newest_skill(source,3204,now))return false;
        if(plan.stance&&!settle_defensive_bar_locked(player,now))return false;
        const auto effect=static_cast<unsigned>(player.next_health_effect);
        const auto next=skills::effects::prepare_defy_pain_health(before,temporary_health_snapshot_locked(player,now),plan,effect,player.conditions.has(combat::conditions::Kind::deep_wound,now));if(!next)return false;
        auto registry=*player.effect_registry;auto timers=*player.health_regeneration;std::vector<skills::effects::Entry> removed;
        if(next->removed){const auto old=registry.lookup(source,*next->removed);if(!old)return false;removed.push_back(*old);registry.remove(source,*next->removed);timers.remove(source,*next->removed);}
        std::optional<skills::effects::DefensiveRewardProposal> defensive;
        if(plan.stance)if(const auto old=registry.newest(source,skills::effects::Category::stance,now);old&&old->skill==345){
            defensive=player.cooldown_authority?skills::effects::prepare_defensive_reward(registry,player.adrenaline,*player.cooldown_authority,source,old->instance,now,skills::effects::DefensiveEnd::replacement,true):std::nullopt;
            if(!defensive)return false;registry=defensive->registry;removed.push_back(defensive->ended);timers.remove(source,old->instance);
        }
        const skills::effects::Entry entry{effect,source.actor,plan.skill,plan.rank,plan.stance?skills::effects::Category::stance:skills::effects::Category::other,now,now+plan.duration};
        const auto change=registry.apply(source,entry,plan.stance?skills::effects::Replace::category:skills::effects::Replace::same_skill);if(!change.accepted)return false;
        for(const auto& old:change.removed){removed.push_back(old);timers.remove(source,old.instance);}
        if(!timers.apply(source,{effect,source.actor,plan.skill,0,now,now+plan.duration}))return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        advance_movement_effects_locked(player,now);commit_temporary_health_locked(player,*next);
        *player.effect_registry=std::move(registry);*player.health_regeneration=std::move(timers);++player.next_health_effect;
        if(defensive){player.adrenaline=defensive->adrenaline;player.cooldown_authority=defensive->authority;publish_adrenaline_locked(player);}
        std::vector<Packet> packets;
        for(const auto& old:removed){player.health_effects.erase(old.instance);player.movement_effects.remove(old.instance,now);packets.push_back(health_effect_remove(player.id,old.instance));
            const auto credit=hex_breaker_credit_.find(player.id);if(credit!=hex_breaker_credit_.end()&&credit->second.effect==old.instance)hex_breaker_credit_.erase(credit);}
        player.health_effects.emplace(effect,entry);packets.push_back(health_effect_add(entry,player.id,now));advance_movement_effects_locked(player,now);
        for(auto& [id,peer]:players_){auto batch=packets;const auto vitals=id==player.id?owner_vitals(player):public_health(player);batch.insert(batch.end(),vitals.begin(),vitals.end());peer.outbox->enqueue(std::move(batch));}
        settle_conditional_stance_locked(player,now);captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();};
        defer_or_publish_completion_locked(publication,std::move(publish));return true;
    }
