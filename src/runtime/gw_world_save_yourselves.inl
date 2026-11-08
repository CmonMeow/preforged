// Save Yourselves! party snapshot. Runtime payment/animation and all-damage armor
// Runtime owns adrenaline payment and the cast animation.
public:
    bool stage_save_yourselves_party(skills::effects::Scope source,
            const skills::effects::SaveYourselvesPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto canonical=skills::effects::save_yourselves(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto owner=players_.find(source.actor);
        if(owner==players_.end()||owner->second.effect_scope!=source||
           owner->second.knocked_down_until>now||
           !advance_resource_event_locked(owner->second,now)||owner->second.precise_health<=0)return false;
        const auto group=parties_.group(static_cast<std::uint16_t>(source.actor));
        const auto origin=healing_position_locked(owner->second,now);
        if(!origin)return false;
        if(!group)return true; // A solo shout still spends its adrenaline.
        struct Prepared {Player* player;skills::effects::Registry registry;
            skills::regeneration::State timer;skills::effects::Entry entry;
            std::vector<skills::effects::Entry> removed;};
        std::vector<Prepared> prepared;
        for(const auto id:parties_.members(group)){
            if(id==source.actor)continue; // Only other party members gain armor.
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
            const auto entry=skills::effects::prepare_save_yourselves(plan,source,target,
                effect,now,true,true,distance);
            if(!entry)return false;
            auto registry=*recipient.effect_registry;
            auto timer=*recipient.health_regeneration;
            const auto changed=registry.apply(target,*entry,skills::effects::Replace::same_skill);
            if(!changed.accepted)return false;
            for(const auto& old:changed.removed)timer.remove(target,old.instance);
            if(!timer.apply(target,{effect,source.actor,plan.skill,0,now,*entry->expires}))return false;
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
               party_owner->second.precise_health<=0||
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
            const auto changed=proposed.apply_save_yourselves(proposed.lifetime(),source,
                plan,next_npc_effect,now,true,distance);
            if(!changed)return false;
            const auto entry=proposed.hex_effect(proposed.lifetime(),next_npc_effect);
            if(!entry)return false;
            npc_prepared.push_back({npc_scope.actor,std::move(proposed),*entry,
                changed->removed});
            ++next_npc_effect;
        }
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
            npc_save_yourselves_effects_.emplace(live.hex_scope(),change.entry.instance);
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
