public:
    // Trusted, target-scoped completion. The ordinary paid client cast is
    // deliberately separate until its end-effect protocol is reviewed.
    bool stage_you_move_like_a_dwarf(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::YouMoveLikeADwarfPlan& plan,
            unsigned level,std::uint64_t now,std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(skills::effects::you_move_like_a_dwarf(plan.norn_rank)!=plan||
           !level||level>255||source==target||now>UINT64_MAX-plan.cripple_ms||
           now>UINT64_MAX-plan.knockdown_ms)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!caster->second.team_token||
           !advance_health_locked(caster->second,now)||caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);if(!origin)return false;
        const auto selected=players_.find(target.actor);
        const auto found=npc_combat_.find(target.actor);
        std::optional<navigation::Point> destination;unsigned plane=0;
        if(selected!=players_.end()){
            auto& victim=selected->second;
            if(victim.effect_scope!=target||!victim.resources||!victim.team_token||
               victim.team_token==caster->second.team_token||
               !advance_health_locked(victim,now)||victim.precise_health<=0)return false;
            destination=healing_position_locked(victim,now);plane=victim.surface.plane;
        }else{
            const auto placed=npcs_.find(target.actor);
            if(placed==npcs_.end()||found==npc_combat_.end()||
               found->second.hex_scope()!=target||
               !found->second.hostile_to_players(found->second.lifetime()))return false;
            refresh_npc_movement_locked(target.actor,now);
            advance_npc_conditions_locked(target.actor,found->second,now);
            const auto* actor=found->second.actor(found->second.lifetime());
            if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)return false;
            destination=navigation::Point{placed->second.x,placed->second.y};plane=placed->second.plane;
        }
        constexpr float range=1248.f; // shared ranged-foe policy, pending retail distance evidence
        if(!destination||caster->second.surface.plane!=plane||
           std::hypot(origin->x-destination->x,origin->z-destination->z)>range)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,*destination);
            if(!line||line->plane!=plane)return false;}
        const combat::Damage damage{combat::Kind::armor_ignoring,double(plan.damage),level,2358};
        std::optional<combat::Hit> preview_hit;
        std::optional<combat::conditions::State> player_cripple;
        bool player_knock=false,npc_cripple=false,npc_knock=false;
        if(selected!=players_.end()){
            auto& victim=selected->second;
            auto actor=incoming_spell_actor_locked(victim,{victim.id,victim.effect_scope.generation,
                combat::Health{victim.precise_health,victim.resources->max_health},
                std::nullopt,victim.effects,true},now);
            preview_hit=combat::apply(actor,damage,true);
            if(preview_hit->reason!=combat::Reason::accepted||!prepare_player_reactive_seed_locked(victim,*preview_hit,now))return false;
            player_knock=victim.knocked_down_until<=now&&
                !(victim.effect_registry&&(
                    skills::effects::active_dolyak(*victim.effect_registry,target,now)||
                    skills::effects::balanced_stance_active(*victim.effect_registry,target,now)||
                    skills::effects::unstoppable_active(*victim.effect_registry,target,now)));
            if(player_knock&&victim.control_epoch==UINT64_MAX)return false;
            if((victim.effects&0x10)==0){
                auto proposed=victim.conditions;
                if(!proposed.apply(combat::conditions::Kind::crippled,source.actor,now,plan.cripple_ms))return false;
                player_cripple=proposed;
                if(victim.next_health_effect==UINT32_MAX)return false;
            }
        }else{
            auto proposal=found->second;const auto life=proposal.lifetime();
            using K=combat::npcs::control::Knockdown;
            const auto control=proposal.knock_down(life,now,plan.knockdown_ms);
            if(control==K::invalid||control==K::unknown)return false;
            npc_knock=control==K::accepted;
            npc_cripple=proposal.allows_conditions(life)&&
                !(proposal.actor(life)->effects&0x10);
            if(npc_cripple&&!proposal.apply_condition(life,combat::conditions::Kind::crippled,
                    source.actor,now,plan.cripple_ms))return false;
            if(!preflight_npc_reversal_or_damage_locked(proposal,damage,now))return false;
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)
            queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        const auto caster_body=combat::detail::integer(21,source.actor,1912);
        const auto target_body=skills::particles::targeted_body_2022(target.actor,source.actor,2358);
        if(!target_body)throw std::logic_error("validated dwarf target body missing");
        for(auto& [id,peer]:players_)peer.outbox->enqueue({caster_body,*target_body});
        if(selected!=players_.end()){
            auto& victim=selected->second;
            if(player_knock&&!knockdown_player_locked(victim,plan.knockdown_ms,now))
                throw std::logic_error("preflighted dwarf player knockdown failed");
            if(player_cripple)commit_conditions_locked(victim,*player_cripple,
                {combat::conditions::Kind::crippled},now);
            if(victim.precise_health>0){
                auto actor=incoming_spell_actor_locked(victim,{victim.id,victim.effect_scope.generation,
                    combat::Health{victim.precise_health,victim.resources->max_health},
                    std::nullopt,victim.effects,true},now);
                const auto hit=combat::apply(actor,damage,true);
                if(hit.reason!=combat::Reason::accepted)throw std::logic_error("preflighted dwarf player damage failed");
                if(!commit_player_reactive_hit_locked(victim,hit,source.actor,now,false,nullptr,false,false))
                    throw std::logic_error("preflighted dwarf player protection failed");
            }
        }else{
            auto& npc=found->second;const auto life=npc.lifetime();
            if(npc_knock&&knockdown_npc_locked(target,plan.knockdown_ms,now)!=combat::npcs::control::Knockdown::accepted)
                throw std::logic_error("preflighted dwarf NPC knockdown failed");
            if(npc_cripple){
                if(!npc.apply_condition(life,combat::conditions::Kind::crippled,source.actor,now,plan.cripple_ms))
                    throw std::logic_error("preflighted dwarf NPC cripple failed");
                for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,npc.actor(life)->effects)});
                npc_condition_credit_[{target.actor,source.actor}]={target,source};
                publish_npc_fragility_locked(target.actor,npc);commit_applications_locked(target,now);
            }
            if(npc.actor(life)->health->current>0){
                const auto placed=npcs_.find(target.actor);
                if(!commit_npc_reversal_or_damage_locked(caster->second,placed->second,npc,damage,now))
                    throw std::logic_error("preflighted dwarf NPC damage failed");
            }
        }
        captures.clear();auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();};
        defer_or_publish_completion_locked(publication,std::move(publish));
        return true;
    }
