public:
    // One mixed player/NPC batch centered on the scoped selected foe. The
    // ordinary paid cast and effect publication are separate from preflight.
    bool stage_you_are_all_weaklings(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::YouAreAllWeaklingsPlan& plan,
            std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(skills::effects::you_are_all_weaklings(plan.norn_rank)!=plan||
           source==target||now>UINT64_MAX-plan.weakness_ms)return false;
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!caster->second.team_token||
           !advance_health_locked(caster->second,now)||caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin)return false;
        std::optional<navigation::Point> center;unsigned plane=0;
        const auto selected_player=players_.find(target.actor);
        if(selected_player!=players_.end()){
            auto& victim=selected_player->second;
            if(victim.effect_scope!=target||!victim.resources||!victim.team_token||
               victim.team_token==caster->second.team_token||
               !advance_health_locked(victim,now)||victim.precise_health<=0)return false;
            center=healing_position_locked(victim,now);plane=victim.surface.plane;
        }else{
            const auto placed=npcs_.find(target.actor);
            const auto live=npc_combat_.find(target.actor);
            if(placed==npcs_.end()||live==npc_combat_.end()||
               live->second.hex_scope()!=target||
               !live->second.hostile_to_players(live->second.lifetime()))return false;
            refresh_npc_movement_locked(target.actor,now);
            advance_npc_conditions_locked(target.actor,live->second,now);
            const auto* actor=live->second.actor(live->second.lifetime());
            if(!actor||!actor->health||actor->health->current<=0)return false;
            center=navigation::Point{placed->second.x,placed->second.y};plane=placed->second.plane;
        }
        if(!center||plane!=caster->second.surface.plane||
           std::hypot(center->x-origin->x,center->z-origin->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,*center);
            if(!line||line->plane!=plane)return false;}
        struct PlayerCondition {Player* player;combat::conditions::State proposed;};
        std::vector<PlayerCondition> people;
        std::vector<std::uint32_t> creatures;
        for(auto& [id,p]:players_){
            if(!p.team_token||p.team_token==caster->second.team_token||!p.resources||
               p.precise_health<=0||p.surface.plane!=plane)continue;
            const auto point=healing_position_locked(p,now);
            if(!point)return false;
            if(std::hypot(point->x-center->x,point->z-center->z)>plan.adjacent)continue;
            if(!advance_health_locked(p,now)||p.precise_health<=0)return false;
            if(p.effects&0x10)continue;
            if(p.next_health_effect==UINT32_MAX)return false;
            auto proposed=p.conditions;
            if(!proposed.apply(combat::conditions::Kind::weakness,source.actor,now,plan.weakness_ms))return false;
            people.push_back({&p,std::move(proposed)});
        }
        for(auto& [id,live]:npc_combat_){
            const auto placed=npcs_.find(id);const auto life=live.lifetime();
            if(placed==npcs_.end()||placed->second.plane!=plane||
               !live.hostile_to_players(life))continue;
            refresh_npc_movement_locked(id,now);
            if(std::hypot(placed->second.x-center->x,placed->second.y-center->z)>plan.adjacent)continue;
            advance_npc_conditions_locked(id,live,now);
            const auto* actor=live.actor(life);
            if(!actor||!actor->health||actor->health->current<=0||
               !live.allows_conditions(life)||(actor->effects&0x10))continue;
            auto proposed=live;
            if(!proposed.apply_condition(life,combat::conditions::Kind::weakness,
                    source.actor,now,plan.weakness_ms))return false;
            creatures.push_back(id);
        }
        const auto caster_body=combat::detail::integer(21,source.actor,1914);
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        for(auto& [id,peer]:players_)peer.outbox->enqueue({caster_body});
        for(auto& candidate:people){
            auto& p=*candidate.player;
            if(p.precise_health<=0)continue;
            commit_conditions_locked(p,candidate.proposed,{combat::conditions::Kind::weakness},now);
            if(const auto body=skills::particles::targeted_body_2022(p.id,source.actor,2359))
                for(auto& [id,peer]:players_)peer.outbox->enqueue({*body});
        }
        for(const auto id:creatures){
            auto& live=npc_combat_.at(id);const auto life=live.lifetime();
            const auto* actor=live.actor(life);
            if(!actor||!actor->health||actor->health->current<=0)continue;
            if(!live.apply_condition(life,combat::conditions::Kind::weakness,
                    source.actor,now,plan.weakness_ms))
                throw std::logic_error("preflighted Norn Weakness failed");
            npc_condition_credit_[{id,source.actor}]={{instance_scope_,id,life.actor_generation},source};
            publish_npc_fragility_locked(id,live);
            for(auto& [peer_id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(id,live.actor(life)->effects)});
            commit_applications_locked({instance_scope_,id,life.actor_generation},now);
            if(const auto body=skills::particles::targeted_body_2022(id,source.actor,2359))
                for(auto& [peer_id,peer]:players_)peer.outbox->enqueue({*body});
        }
        captures.clear();auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();};
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
