// Trusted completion components. Normal Runtime casting remains gated until
// NPC party membership and captured body ordering are reviewed.
private:
    struct HealingBurstRecipient {std::uint32_t id,amount;};
    std::optional<std::vector<HealingBurstRecipient>> healing_burst_party_locked(
            skills::effects::Scope source,navigation::Point center,
            unsigned plane,std::uint32_t skip,
            const skills::effects::HealingBurstPlan& plan,
            std::uint64_t now){
        std::vector<HealingBurstRecipient> recipients;
        const auto party=parties_.group(static_cast<std::uint16_t>(source.actor));
        if(!party||!plan.divine_party_heal)return recipients;
        for(const auto member:parties_.members(party)){
            if(member==skip)continue;
            const auto found=players_.find(member);
            if(found==players_.end()||!found->second.resources||
               found->second.precise_health<=0||
               found->second.surface.plane!=plane)continue;
            const auto point=healing_position_locked(found->second,now);
            if(!point)return {};
            const auto distance=std::hypot(point->x-center.x,
                point->z-center.z);
            const auto amount=skills::effects::healing_burst_recipient(
                plan,false,true,static_cast<float>(distance));
            if(!amount)return {};
            if(!*amount)continue;
            if(!advance_health_locked(found->second,now))return {};
            if(found->second.precise_health>0)
                recipients.push_back({member,*amount});
        }
        return recipients;
    }
public:
    bool stage_healing_burst_player(skills::effects::Scope source,
            skills::effects::Scope target,
            const skills::effects::HealingBurstPlan& plan,
            std::uint64_t event,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(!skills::effects::valid_healing_burst(plan)||!event)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor),selected=players_.find(target.actor);
        if(caster==players_.end()||selected==players_.end()||
           caster->second.effect_scope!=source||selected->second.effect_scope!=target||
           !caster->second.cooldown_authority||
           caster->second.knocked_down_until>now||
           (plan.divine_party_heal&&!monk_primary(caster->second))||
           !live_heal_target_locked(caster->second,selected->second,
               plan.target_range,true))return false;
        const auto origin=healing_position_locked(caster->second,now);
        const auto center=healing_position_locked(selected->second,now);
        if(!origin||!center||
           caster->second.surface.plane!=selected->second.surface.plane||
           std::hypot(origin->x-center->x,origin->z-center->z)>plan.target_range||
           !advance_resource_event_locked(caster->second,now)||
           !advance_health_locked(selected->second,now)||
           caster->second.precise_health<=0||selected->second.precise_health<=0)return false;
        const auto bar=caster->second.cooldown_authority->snapshot(source);
        if(!bar)return false;
        const auto old=healing_burst_disables_.find(source);
        const auto disables=(old==healing_burst_disables_.end()?
            skills::effects::HealingBurstDisables(source):old->second).propose(
                source,plan,*bar,event,now);
        if(!disables)return false;
        std::vector<HealingBurstRecipient> recipients{{target.actor,
            *skills::effects::healing_burst_recipient(plan,true,false,0)}};
        auto party_recipients=healing_burst_party_locked(source,*center,
            selected->second.surface.plane,target.actor,plan,now);
        if(!party_recipients)return false;
        recipients.insert(recipients.end(),party_recipients->begin(),
            party_recipients->end());
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(const auto [id,amount]:recipients){
            auto& player=players_.at(id);
            const auto before=player.precise_health;
            health_event_locked(player,amount,now,true);
            if(player.precise_health<=0)kill_locked(player,now);
            publish_health_locked(player);
            const auto gained=player.precise_health-before;
            if(gained>0)for(auto& [peer_id,peer]:players_)
                peer.outbox->enqueue({skills::particles::health_change_2022(
                    id,source.actor,gained,player.resources->max_health)});
        }
        healing_burst_disables_.insert_or_assign(source,*disables);
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
    bool stage_healing_burst_npc(skills::effects::Scope source,
            skills::effects::Scope target,
            const skills::effects::HealingBurstPlan& plan,
            std::uint64_t event,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(!skills::effects::valid_healing_burst(plan)||!event)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor);
        const auto initial=npc_combat_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.cooldown_authority||
           caster->second.knocked_down_until>now||
           (plan.divine_party_heal&&!monk_primary(caster->second))||
           initial==npc_combat_.end()||initial->second.hex_scope()!=target)return false;
        const auto life=initial->second.lifetime();
        if(!initial->second.healable_ally(life)||
           initial->second.healing_allowed(life)!=std::optional(true)||
           now<initial->second.health_clock(life).value_or(now))return false;
        refresh_npc_movement_locked(target.actor,now);
        auto body=npc_combat_.find(target.actor);
        auto placement=npcs_.find(target.actor);
        if(body==npc_combat_.end()||placement==npcs_.end()||
           !body->second.matches(life)||
           placement->second.plane!=caster->second.surface.plane)return false;
        const auto origin=healing_position_locked(caster->second,now);
        const navigation::Point center{placement->second.x,placement->second.y};
        if(!origin||std::hypot(origin->x-center.x,
            origin->z-center.z)>plan.target_range)return false;
        if(navigation_){
            const auto line=layered_->trace(caster->second.surface,*origin,center);
            if(!line||line->plane!=placement->second.plane)return false;
        }
        if(!advance_resource_event_locked(caster->second,now)||
           caster->second.precise_health<=0)return false;
        advance_npc_conditions_locked(target.actor,body->second,now);
        body=npc_combat_.find(target.actor);
        placement=npcs_.find(target.actor);
        if(body==npc_combat_.end()||placement==npcs_.end()||
           !body->second.matches(life)||!body->second.healable_ally(life)||
           body->second.healing_allowed(life)!=std::optional(true))return false;
        const auto bar=caster->second.cooldown_authority->snapshot(source);
        if(!bar)return false;
        const auto old=healing_burst_disables_.find(source);
        const auto disables=(old==healing_burst_disables_.end()?
            skills::effects::HealingBurstDisables(source):old->second).propose(
                source,plan,*bar,event,now);
        if(!disables)return false;
        auto party_recipients=healing_burst_party_locked(source,center,
            placement->second.plane,target.actor,plan,now);
        if(!party_recipients)return false;
        auto proposed=body->second;
        const auto before=proposed.actor(life)->health->current;
        const auto amount=skills::effects::healing_burst_recipient(
            plan,true,false,0);
        if(!amount||!proposed.heal_ally(life,*amount,now))return false;
        const auto health=*proposed.actor(life)->health;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        body->second=std::move(proposed);
        for(auto& [id,peer]:players_){
            std::vector<Packet> packets{combat::detail::fraction(target.actor,health)};
            if(health.current>before)
                packets.push_back(skills::particles::health_change_2022(
                    target.actor,source.actor,health.current-before,health.maximum));
            peer.outbox->enqueue(packets);
        }
        for(const auto [id,party_amount]:*party_recipients){
            auto& player=players_.at(id);
            const auto previous=player.precise_health;
            health_event_locked(player,party_amount,now,true);
            if(player.precise_health<=0)kill_locked(player,now);
            publish_health_locked(player);
            const auto gained=player.precise_health-previous;
            if(gained>0)for(auto& [peer_id,peer]:players_)
                peer.outbox->enqueue({skills::particles::health_change_2022(
                    id,source.actor,gained,player.resources->max_health)});
        }
        healing_burst_disables_.insert_or_assign(source,*disables);
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
