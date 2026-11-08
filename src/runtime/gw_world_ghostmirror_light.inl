// Included inside Instance after the shared living-spirit proximity query.
public:
    bool stage_ghostmirror_light_player(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::GhostmirrorLightPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        if(skills::effects::ghostmirror_light(plan.rank)!=plan||source==target)return false;
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor),ally=players_.find(target.actor);
        if(caster==players_.end()||ally==players_.end()||
           caster->second.effect_scope!=source||ally->second.effect_scope!=target||
           !caster->second.resources||!resurrection_clock_valid(caster->second,now)||
           !live_heal_target_locked(caster->second,ally->second,1248.f,false))return false;
        if(!settle_timed_effects_locked(now))return false;
        const auto origin=healing_position_locked(caster->second,now);
        const auto destination=healing_position_locked(ally->second,now);
        if(!origin||!destination||caster->second.surface.plane!=ally->second.surface.plane||
           std::hypot(origin->x-destination->x,origin->z-destination->z)>1248.f||
           caster->second.knocked_down_until>now)return false;
        if(!advance_resource_event_locked(caster->second,now)||
           !advance_health_locked(ally->second,now)||
           caster->second.precise_health<=0||ally->second.precise_health<=0)return false;
        const auto quote=skills::effects::quote_ghostmirror_light(
            plan,spirit_light_proximity_locked(source,now));
        if(!quote)return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,1741,*completion_event,now,true))return false;
        const auto before=ally->second.precise_health;
        health_event_locked(ally->second,quote->recipient_heal,now,true);
        publish_health_locked(ally->second);
        const auto gained=ally->second.precise_health-before;
        if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
            skills::particles::health_change_2022(target.actor,source.actor,gained,
                ally->second.resources->max_health)});
        if(quote->caster_heal){
            const auto caster_before=caster->second.precise_health;
            health_event_locked(caster->second,quote->caster_heal,now,true);
            publish_health_locked(caster->second);
            const auto caster_gained=caster->second.precise_health-caster_before;
            if(caster_gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
                skills::particles::health_change_2022(source.actor,source.actor,caster_gained,
                    caster->second.resources->max_health)});
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }

    bool stage_ghostmirror_light_npc(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::GhostmirrorLightPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        if(skills::effects::ghostmirror_light(plan.rank)!=plan||source==target)return false;
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        auto live=npc_combat_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!resurrection_clock_valid(caster->second,now)||
           live==npc_combat_.end()||live->second.hex_scope()!=target)return false;
        const auto life=live->second.lifetime();
        if(!live->second.healable_ally(life)||
           live->second.healing_allowed(life)!=std::optional(true)||
           now<live->second.health_clock(life).value_or(now))return false;
        if(!settle_timed_effects_locked(now))return false;
        refresh_npc_movement_locked(target.actor,now);
        live=npc_combat_.find(target.actor);
        const auto placed=npcs_.find(target.actor);
        if(live==npc_combat_.end()||placed==npcs_.end()||
           !live->second.matches(life)||!live->second.healable_ally(life)||
           live->second.healing_allowed(life)!=std::optional(true)||
           placed->second.plane!=caster->second.surface.plane||
           caster->second.knocked_down_until>now)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin||std::hypot(origin->x-placed->second.x,
                              origin->z-placed->second.y)>1248.f)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,
                *origin,{placed->second.x,placed->second.y});
            if(!line||line->plane!=placed->second.plane)return false;}
        if(!advance_resource_event_locked(caster->second,now)||
           caster->second.precise_health<=0)return false;
        advance_npc_conditions_locked(target.actor,live->second,now);
        live=npc_combat_.find(target.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||
           !live->second.healable_ally(life)||
           live->second.healing_allowed(life)!=std::optional(true))return false;
        const auto quote=skills::effects::quote_ghostmirror_light(
            plan,spirit_light_proximity_locked(source,now));
        if(!quote)return false;
        auto proposed=live->second;
        const auto before=proposed.actor(life)->health->current;
        if(!proposed.heal_ally(life,quote->recipient_heal,now))return false;
        const auto healed=proposed.actor(life)->health->current-before;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,1741,*completion_event,now,true))return false;
        live->second=std::move(proposed);
        for(auto& [id,peer]:players_){
            std::vector<Packet> packets{
                combat::detail::fraction(target.actor,*live->second.actor(life)->health)};
            if(healed>0)packets.push_back(
                skills::particles::health_change_2022(target.actor,source.actor,
                    healed,live->second.actor(life)->health->maximum));
            peer.outbox->enqueue(packets);
        }
        if(quote->caster_heal){
            const auto caster_before=caster->second.precise_health;
            health_event_locked(caster->second,quote->caster_heal,now,true);
            publish_health_locked(caster->second);
            const auto caster_gained=caster->second.precise_health-caster_before;
            if(caster_gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
                skills::particles::health_change_2022(source.actor,source.actor,caster_gained,
                    caster->second.resources->max_health)});
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
