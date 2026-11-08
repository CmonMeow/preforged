// Trusted, target-scoped effect completion. A normal client cast is not bound.
public:
    bool stage_finish_him(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::FinishHimPlan& plan,
            unsigned level,std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(skills::effects::finish_him(plan.norn_rank)!=plan||!level||level>255||source==target)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!caster->second.team_token||
           caster->second.knocked_down_until>now||
           !advance_health_locked(caster->second,now)||caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin)return false;
        auto selected=players_.find(target.actor);
        auto npc=npc_combat_.find(target.actor);
        std::optional<navigation::Point> destination;unsigned plane=0;
        if(selected!=players_.end()){
            auto& victim=selected->second;
            if(victim.effect_scope!=target||!victim.resources||!victim.team_token||
               victim.team_token==caster->second.team_token||
               !advance_health_locked(victim,now)||victim.precise_health<=0)return false;
            destination=healing_position_locked(victim,now);plane=victim.surface.plane;
        }else{
            const auto placed=npcs_.find(target.actor);
            if(placed==npcs_.end()||npc==npc_combat_.end()||
               npc->second.hex_scope()!=target||
               !npc->second.hostile_to_players(npc->second.lifetime()))return false;
            refresh_npc_movement_locked(target.actor,now);
            advance_npc_conditions_locked(target.actor,npc->second,now);
            const auto* actor=npc->second.actor(npc->second.lifetime());
            if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)return false;
            destination=navigation::Point{placed->second.x,placed->second.y};plane=placed->second.plane;
        }
        if(!destination||caster->second.surface.plane!=plane||
           std::hypot(origin->x-destination->x,origin->z-destination->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,*destination);
            if(!line||line->plane!=plane)return false;}
        std::optional<skills::effects::FinishHimResult> player_result;
        bool player_condition=false;
        std::optional<combat::npcs::LiveActor> npc_proposal;
        bool npc_condition=false;
        std::uint32_t npc_condition_effects=0;
        if(selected!=players_.end()){
            auto& victim=selected->second;
            auto actor=incoming_spell_actor_locked(victim,{victim.id,victim.effect_scope.generation,
                combat::Health{victim.precise_health,victim.resources->max_health},
                std::nullopt,victim.effects,true},now);
            auto threshold_capacity=victim.resources->max_health;
            if(victim.temporary_health){
                threshold_capacity=victim.temporary_health->permanent();
                if(victim.conditions.has(combat::conditions::Kind::deep_wound,now))
                    threshold_capacity-=std::min(100u,threshold_capacity/5);
            }
            player_result=skills::effects::resolve_finish_him(plan,actor,victim.conditions,
                level,source.actor,now,(actor.effects&0x10)==0,threshold_capacity);
            if(!player_result)return false;
            if(player_result->hit&&!prepare_player_reactive_seed_locked(victim,*player_result->hit,now))return false;
            player_condition=player_result->triggered&&(actor.effects&0x10)==0;
            if(player_condition&&
               victim.next_health_effect>UINT32_MAX-2)return false;
        }else{
            const auto life=npc->second.lifetime();const auto* actor=npc->second.actor(life);
            if(actor->health->current*2<actor->health->maximum){
                npc_proposal=npc->second;
                if(npc_proposal->allows_conditions(life)&&
                   !(npc_proposal->actor(life)->effects&0x10)){
                    if(!npc_proposal->apply_condition(life,combat::conditions::Kind::cracked_armor,
                            source.actor,now,plan.conditions_ms))return false;
                    if(!npc_proposal->apply_condition(life,combat::conditions::Kind::deep_wound,
                            source.actor,now,plan.conditions_ms))return false;
                    npc_condition=true;
                    npc_condition_effects=npc_proposal->actor(life)->effects;
                }
                auto preview=*npc_proposal;
                if(!preflight_npc_reversal_or_damage_locked(preview,
                    {combat::Kind::armor_ignoring,double(plan.damage),level,2353},now))return false;
            }
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        if(player_result&&player_result->triggered){
            auto& victim=selected->second;
            if(player_condition)
                commit_conditions_locked(victim,player_result->conditions,
                    {combat::conditions::Kind::cracked_armor,combat::conditions::Kind::deep_wound},now);
            if(victim.precise_health>0){
                auto actor=incoming_spell_actor_locked(victim,{victim.id,victim.effect_scope.generation,
                    combat::Health{victim.precise_health,victim.resources->max_health},
                    std::nullopt,victim.effects,true},now);
                const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,
                    double(plan.damage),level,2353},true);
                if(hit.reason!=combat::Reason::accepted)throw std::logic_error("preflighted Finish Him damage rejected");
                if(!commit_player_reactive_hit_locked(victim,hit,source.actor,now,false,nullptr,false,false))
                    throw std::logic_error("preflighted Finish Him protection commit rejected");
            }
        }
        if(npc_proposal){
            const auto life=npc->second.lifetime();
            npc->second=std::move(*npc_proposal);
            if(npc_condition)for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,npc_condition_effects)});
            if(!commit_npc_reversal_or_damage_locked(caster->second,npcs_.at(target.actor),npc->second,
                {combat::Kind::armor_ignoring,double(plan.damage),level,2353},now))
                throw std::logic_error("preflighted Finish Him NPC protection commit rejected");
            if(npc_condition){npc_condition_credit_[{target.actor,source.actor}]={target,source};
                publish_npc_fragility_locked(target.actor,npc->second);commit_applications_locked(target,now);}
        }
        captures.clear();auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();};
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
