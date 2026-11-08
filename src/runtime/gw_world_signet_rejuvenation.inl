// Included inside Instance. This trusted player-recipient transaction is not
// a client skill binding; the 2022 visual consumer remains unverified.
public:
    bool valid_signet_rejuvenation_target(skills::effects::Scope source,
            skills::effects::Scope target,std::uint64_t now)const{
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||caster->second.precise_health<=0)return false;
        const auto from=healing_position_locked(caster->second,now);
        if(!from)return false;
        if(const auto player=players_.find(target.actor);player!=players_.end()){
            if(player->second.effect_scope!=target||
               !live_heal_target_locked(caster->second,player->second,1248.f,true))return false;
            const auto to=healing_position_locked(player->second,now);
            if(!to||std::hypot(from->x-to->x,from->z-to->z)>1248.f)return false;
            if(navigation_){const auto line=layered_->trace(caster->second.surface,*from,*to);
                if(!line||line->plane!=player->second.surface.plane)return false;}
            return true;
        }
        const auto npc=npcs_.find(target.actor);
        const auto body=npc_combat_.find(target.actor);
        if(npc==npcs_.end()||body==npc_combat_.end()||
           body->second.hex_scope()!=target||
           !body->second.healable_ally(body->second.lifetime())||
           body->second.healing_allowed(body->second.lifetime())!=std::optional(true)||
           npc->second.plane!=caster->second.surface.plane||
           std::hypot(from->x-npc->second.x,from->z-npc->second.y)>1248.f)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*from,
                {npc->second.x,npc->second.y});
            if(!line||line->plane!=npc->second.plane)return false;}
        return true;
    }

    bool stage_signet_rejuvenation_player(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::RejuvenationPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(skills::effects::signet_rejuvenation(plan.rank)!=plan)return false;
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor),ally=players_.find(target.actor);
        if(caster==players_.end()||ally==players_.end()||
           caster->second.effect_scope!=source||ally->second.effect_scope!=target||
           !caster->second.resources||!resurrection_clock_valid(caster->second,now)||
           !live_heal_target_locked(caster->second,ally->second,plan.range,true))return false;
        if(!settle_timed_effects_locked(now))return false;
        const auto from=healing_position_locked(caster->second,now);
        const auto to=healing_position_locked(ally->second,now);
        if(!from||!to||caster->second.surface.plane!=ally->second.surface.plane||
           std::hypot(from->x-to->x,from->z-to->z)>plan.range||
           caster->second.knocked_down_until>now)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*from,*to);
            if(!line||line->plane!=ally->second.surface.plane)return false;}
        if(!advance_resource_event_locked(caster->second,now)||
           !advance_health_locked(ally->second,now)||
           caster->second.precise_health<=0||ally->second.precise_health<=0)return false;
        bool active_spell=false;
        if(ally->second.casting_skill&&ally->second.casting_begins<=now&&
           now<ally->second.casting_ends){
            const auto* row=skills::find_skill(ally->second.casting_skill);
            if(!row)return false;
            active_spell=skills::effects::is_spell(*row);
        }
        const bool attacking=ally->second.ordinary_animation||
            now<ally->second.melee_skill_until;
        const auto quote=skills::effects::quote_signet_rejuvenation(
            plan,active_spell||attacking);
        if(!quote)return false;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        const auto before=ally->second.precise_health;
        health_event_locked(ally->second,*quote,now,true);
        publish_health_locked(ally->second);
        const auto gained=ally->second.precise_health-before;
        if(gained>0)for(auto& [id,peer]:players_)peer.outbox->enqueue({
            skills::particles::health_change_2022(target.actor,source.actor,gained,
                ally->second.resources->max_health)});
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }

    bool stage_signet_rejuvenation_npc(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::RejuvenationPlan& plan,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(skills::effects::signet_rejuvenation(plan.rank)!=plan)return false;
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
        const auto from=healing_position_locked(caster->second,now);
        if(!from||std::hypot(from->x-placed->second.x,
                              from->z-placed->second.y)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,
                *from,{placed->second.x,placed->second.y});
            if(!line||line->plane!=placed->second.plane)return false;}
        if(!advance_resource_event_locked(caster->second,now)||
           caster->second.precise_health<=0)return false;
        advance_npc_conditions_locked(target.actor,live->second,now);
        live=npc_combat_.find(target.actor);
        if(live==npc_combat_.end()||!live->second.matches(life)||
           !live->second.healable_ally(life)||
           live->second.healing_allowed(life)!=std::optional(true))return false;
        const auto action=live->second.active_action(life,now);
        const auto quote=skills::effects::quote_signet_rejuvenation(plan,
            bool(action&&(action->kind==combat::npcs::control::Action::attack||
                          action->kind==combat::npcs::control::Action::spell)));
        if(!quote)return false;
        auto proposed=live->second;
        const auto before=proposed.actor(life)->health->current;
        if(!proposed.heal_ally(life,*quote,now))return false;
        const auto healed=proposed.actor(life)->health->current-before;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        live->second=std::move(proposed);
        for(auto& [id,peer]:players_){
            std::vector<Packet> packets{
                combat::detail::fraction(target.actor,*live->second.actor(life)->health)};
            if(healed>0)packets.push_back(
                skills::particles::health_change_2022(target.actor,source.actor,
                    healed,live->second.actor(life)->health->maximum));
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
