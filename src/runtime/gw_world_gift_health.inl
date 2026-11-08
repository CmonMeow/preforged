// Trusted World completion stages. Normal cast admission and payment remain
// separate; these entrypoints are not exposed to decoded traffic.
public:
    bool valid_gift_health_target(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now)const{
        if(source==target)return false;
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin)return false;
        const auto player=players_.find(target.actor);
        if(player!=players_.end()){
            if(player->second.effect_scope!=target||
               !live_heal_target_locked(caster->second,player->second,1248.f,false))return false;
            const auto destination=healing_position_locked(player->second,now);
            return destination&&std::hypot(origin->x-destination->x,
                origin->z-destination->z)<=1248.f;
        }
        const auto npc=npcs_.find(target.actor);
        const auto body=npc_combat_.find(target.actor);
        if(npc==npcs_.end()||body==npc_combat_.end()||
           body->second.hex_scope()!=target||
           !body->second.healable_ally(body->second.lifetime())||
           body->second.healing_allowed(body->second.lifetime())!=std::optional(true)||
           npc->second.plane!=caster->second.surface.plane||
           std::hypot(origin->x-npc->second.x,origin->z-npc->second.y)>1248.f)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,
                *origin,{npc->second.x,npc->second.y});
            if(!line||line->plane!=npc->second.plane)return false;}
        return true;
    }
    bool stage_gift_health_player(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::GiftHealthPlan& plan,
            unsigned divine_rank,std::uint64_t event,std::uint64_t now,
            std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        const auto canonical=skills::effects::gift_health(plan.rank);
        if(!canonical||*canonical!=plan||divine_rank>20||!event||source==target)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor),ally=players_.find(target.actor);
        if(caster==players_.end()||ally==players_.end()||
           caster->second.effect_scope!=source||ally->second.effect_scope!=target||
           !caster->second.cooldown_authority||
           (divine_rank&&!monk_primary(caster->second))||
           caster->second.knocked_down_until>now||
           !live_heal_target_locked(caster->second,ally->second,plan.range,false))return false;
        const auto origin=healing_position_locked(caster->second,now);
        const auto destination=healing_position_locked(ally->second,now);
        if(!origin||!destination||caster->second.surface.plane!=ally->second.surface.plane||
           std::hypot(origin->x-destination->x,origin->z-destination->z)>plan.range)return false;
        if(!advance_resource_event_locked(caster->second,now)||
           !advance_health_locked(ally->second,now)||
           caster->second.precise_health<=0||ally->second.precise_health<=0)return false;
        const auto bar=caster->second.cooldown_authority->snapshot(source);
        if(!bar)return false;
        const auto old=gift_health_disables_.find(source);
        const auto proposal=(old==gift_health_disables_.end()?
            skills::effects::GiftHealthDisables(source):old->second).propose(source,plan,*bar,event,now);
        if(!proposal)return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,1121,*completion_event,now,true))return false;
        auto& recipient=ally->second;
        const auto previous=recipient.precise_health;
        health_event_locked(recipient,plan.healing+std::lround(3.2*divine_rank),now,true);
        publish_health_locked(recipient);
        const auto gained=recipient.precise_health-previous;
        if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
            skills::particles::health_change_2022(target.actor,source.actor,gained,
                recipient.resources->max_health)});
        gift_health_disables_.insert_or_assign(source,*proposal);
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
    bool stage_gift_health_npc(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::GiftHealthPlan& plan,
            unsigned divine_rank,std::uint64_t event,std::uint64_t now,
            std::function<void()>* publication=nullptr,
            std::optional<std::uint64_t> completion_event={}){
        if(publication)*publication={};
        const auto canonical=skills::effects::gift_health(plan.rank);
        if(!canonical||*canonical!=plan||divine_rank>20||!event||source==target)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor);
        const auto initial=npc_combat_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.cooldown_authority||caster->second.precise_health<=0||
           caster->second.knocked_down_until>now||
           (divine_rank&&!monk_primary(caster->second))||
           initial==npc_combat_.end()||initial->second.hex_scope()!=target)return false;
        const auto life=initial->second.lifetime();
        if(!initial->second.healable_ally(life)||
           initial->second.healing_allowed(life)!=std::optional(true)||
           now<initial->second.health_clock(life).value_or(now))return false;
        refresh_npc_movement_locked(target.actor,now);
        auto body=npc_combat_.find(target.actor);auto placement=npcs_.find(target.actor);
        if(body==npc_combat_.end()||placement==npcs_.end()||
           !body->second.matches(life)||
           placement->second.plane!=caster->second.surface.plane)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin||std::hypot(origin->x-placement->second.x,
                origin->z-placement->second.y)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,
                *origin,{placement->second.x,placement->second.y});
            if(!line||line->plane!=placement->second.plane)return false;}
        if(!advance_resource_event_locked(caster->second,now)||
           caster->second.precise_health<=0)return false;
        advance_npc_conditions_locked(target.actor,body->second,now);
        body=npc_combat_.find(target.actor);placement=npcs_.find(target.actor);
        if(body==npc_combat_.end()||placement==npcs_.end()||
           !body->second.matches(life)||!body->second.healable_ally(life)||
           body->second.healing_allowed(life)!=std::optional(true))return false;
        const auto bar=caster->second.cooldown_authority->snapshot(source);
        if(!bar)return false;
        const auto old=gift_health_disables_.find(source);
        const auto proposal=(old==gift_health_disables_.end()?
            skills::effects::GiftHealthDisables(source):old->second).propose(
                source,plan,*bar,event,now);
        if(!proposal)return false;
        auto proposed=body->second;
        const auto before=proposed.actor(life)->health->current;
        if(!proposed.heal_ally(life,plan.healing+std::lround(3.2*divine_rank),now))
            return false;
        const auto health=*proposed.actor(life)->health;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(completion_event&&!complete_ether_renewal_locked(source,1121,*completion_event,now,true))return false;
        body->second=std::move(proposed);
        gift_health_disables_.insert_or_assign(source,*proposal);
        for(auto& [id,peer]:players_){
            std::vector<Packet> packets{combat::detail::fraction(target.actor,health)};
            if(health.current>before)packets.push_back(
                skills::particles::health_change_2022(target.actor,source.actor,
                    health.current-before,health.maximum));
            peer.outbox->enqueue(packets);
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
