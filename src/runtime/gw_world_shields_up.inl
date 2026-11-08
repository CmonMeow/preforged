// Shared timed party-defense shout transaction. Runtime owns payment and
// activation;recipient lifetime,range,replacement and publication are World-owned.
public:
    bool stage_shields_up_party(skills::effects::Scope source,const skills::effects::ShieldsUpPlan& plan,
            std::uint64_t now,std::function<void()>* publication=nullptr){
        return stage_party_defense_shout(source,plan,now,publication);
    }
    bool stage_dont_trip_party(skills::effects::Scope source,const skills::effects::DontTripPlan& plan,
            std::uint64_t now,std::function<void()>* publication=nullptr){
        return stage_party_defense_shout(source,plan,now,publication);
    }
private:
    template<class Plan> bool stage_party_defense_shout(skills::effects::Scope source,
            const Plan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto canonical=skills::effects::party_defense_canonical(plan);
        if(!canonical||*canonical!=plan)return false;
        const auto skill=skills::effects::party_defense_skill(plan);
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto owner=players_.find(source.actor);
        if(owner==players_.end()||owner->second.effect_scope!=source||
           !advance_resource_event_locked(owner->second,now)||owner->second.precise_health<=0)return false;
        const auto group=parties_.group(static_cast<std::uint16_t>(source.actor));
        const auto origin=healing_position_locked(owner->second,now);
        if(!group||!origin)return false;
        // Consume every linked resource/expiry event before retaining copied
        // registries. Later recipients must not roll back earlier reactions.
        for(const auto id:parties_.members(group)){
            const auto recipient=players_.find(id);if(recipient==players_.end())return false;
            if(!advance_resource_event_locked(recipient->second,now))return false;
        }
        std::vector<skills::effects::Scope> npc_scopes;
        for(const auto&[target,member]:npc_party_members_)npc_scopes.push_back(target);
        for(const auto target:npc_scopes){const auto live=npc_combat_.find(target.actor);
            if(live!=npc_combat_.end()&&live->second.hex_scope()==target)
                advance_npc_conditions_locked(target.actor,live->second,now);}
        if(owner->second.precise_health<=0)return false;
        struct Prepared {Player* player;skills::effects::Registry registry;
            skills::regeneration::State timer;skills::effects::Entry entry;
            std::vector<skills::effects::Entry> removed;};
        std::vector<Prepared> prepared;
        for(const auto id:parties_.members(group)){
            const auto found=players_.find(id);if(found==players_.end())return false;
            auto& recipient=found->second;
            const auto point=healing_position_locked(recipient,now);if(!point)return false;
            const float distance=std::hypot(point->x-origin->x,point->z-origin->z);
            if(distance>plan.range||recipient.surface.plane!=owner->second.surface.plane)continue;
            if(!advance_resource_event_locked(recipient,now))return false;
            if(recipient.precise_health<=0)continue;
            if(!recipient.effect_registry||!recipient.health_regeneration||
               recipient.next_health_effect>UINT32_MAX)return false;
            const auto target=recipient.effect_scope;
            const auto effect=static_cast<unsigned>(recipient.next_health_effect);
            const auto entry=skills::effects::prepare_party_defense_shout(plan,source,target,
                effect,now,true,true,distance);
            if(!entry)return false;
            auto registry=*recipient.effect_registry;
            auto timer=*recipient.health_regeneration;
            const auto changed=registry.apply(target,*entry,skills::effects::Replace::same_skill);
            if(!changed.accepted)return false;
            for(const auto& old:changed.removed)timer.remove(target,old.instance);
            if(!timer.apply(target,{effect,source.actor,skill,0,now,*entry->expires}))return false;
            prepared.push_back({&recipient,std::move(registry),std::move(timer),
                *entry,changed.removed});
        }
        struct NpcPrepared {std::uint32_t id;combat::npcs::LiveActor actor;
            skills::effects::Entry entry;std::vector<skills::effects::Entry> removed;};
        std::vector<NpcPrepared> npc_prepared;
        auto next_npc_effect=next_npc_hex_;
        for(const auto& [npc_scope,owner_scope]:npc_party_members_){
            const auto party_owner=players_.find(owner_scope.actor);
            const auto npc=npc_combat_.find(npc_scope.actor);
            const auto placement=npcs_.find(npc_scope.actor);
            if(party_owner==players_.end()||party_owner->second.effect_scope!=owner_scope||
               !party_owner->second.resources||
               parties_.group(static_cast<std::uint16_t>(owner_scope.actor))!=group||
               npc==npc_combat_.end()||npc->second.hex_scope()!=npc_scope||
               placement==npcs_.end()||
               placement->second.plane!=owner->second.surface.plane)continue;
            refresh_npc_movement_locked(npc_scope.actor,now);
            const float distance=std::hypot(placement->second.x-origin->x,
                placement->second.y-origin->z);
            if(distance>plan.range)continue;
            if(next_npc_effect>=UINT32_MAX)return false;
            auto proposed=npc->second;
            const auto changed=proposed.apply_party_defense_shout(proposed.lifetime(),source,
                plan,next_npc_effect,now,true,distance);
            if(!changed)return false;
            const auto entry=proposed.hex_effect(proposed.lifetime(),next_npc_effect);
            if(!entry)return false;
            npc_prepared.push_back({npc_scope.actor,std::move(proposed),*entry,
                changed->removed});
            ++next_npc_effect;
        }
        if(prepared.empty())return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(auto& change:prepared){
            auto& recipient=*change.player;
            for(const auto& old:change.removed)
                remove_health_effect_locked(recipient,old.instance,now);
            *recipient.effect_registry=std::move(change.registry);
            *recipient.health_regeneration=std::move(change.timer);
            ++recipient.next_health_effect;
            recipient.health_effects.emplace(change.entry.instance,change.entry);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({
                health_effect_add(change.entry,recipient.id,now)});
        }
        for(auto& change:npc_prepared){
            npc_combat_.at(change.id)=std::move(change.actor);
            auto& live=npc_combat_.at(change.id);
            npc_party_defense_effects_.insert_or_assign({live.hex_scope(),skill},change.entry.instance);
            for(auto& [id,peer]:players_){
                for(const auto& old:change.removed)
                    peer.outbox->enqueue({health_effect_remove(change.id,old.instance)});
                peer.outbox->enqueue({health_effect_add(change.entry,change.id,now),
                    emotes::effects_2022(change.id,live.actor(live.lifetime())->effects)});
            }
        }
        next_npc_hex_=next_npc_effect;
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
public:
    std::optional<unsigned> shields_up_player_armor(skills::effects::Scope target,
            std::uint64_t now,skills::effects::ShieldsUpDamage kind)const{
        std::lock_guard lock(mutex_);
        const auto found=players_.find(target.actor);
        if(found==players_.end()||found->second.effect_scope!=target||
           !found->second.effect_registry)return {};
        return skills::effects::shields_up_armor(*found->second.effect_registry,target,now,kind);
    }
