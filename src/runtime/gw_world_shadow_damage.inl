    bool shadow_damage(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::ShadowPlan& plan,
            unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr,std::function<void()>* publication=nullptr){
        if(visuals)visuals->clear();if(publication)*publication={};if(publication&&!visuals)return false;
        const auto canonical=skills::effects::shadow_damage(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255)return false;
        std::lock_guard lock(mutex_);DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        const auto perform=[&]()->bool{
            const auto a=players_.find(scope.actor),b=players_.find(target.actor);
            if(a==players_.end()||a->second.effect_scope!=scope)return false;
            auto& source=a->second;
            const combat::Damage damage{combat::Kind::armor_ignoring,double(plan.damage),level,plan.skill};
            if(b==players_.end()){
                auto placed=npcs_.find(target.actor);auto found=npc_combat_.find(target.actor);
                if(placed==npcs_.end()||found==npc_combat_.end()||found->second.hex_scope()!=target||!source.resources||
                   source.precise_health<=0||source.knocked_down_until>now||source.surface.plane!=placed->second.plane||now<source.health_updated)return false;
                refresh_npc_movement_locked(target.actor,now);
                const auto origin=healing_position_locked(source,now);
                if(!origin||std::hypot(origin->x-placed->second.x,origin->z-placed->second.y)>plan.range)return false;
                if(navigation_){const auto line=layered_->trace(source.surface,*origin,{placed->second.x,placed->second.y});
                    if(!line||line->plane!=placed->second.plane)return false;}
                if(!advance_health_locked(source,now)||source.precise_health<=0)return false;
                found=npc_combat_.find(target.actor);
                if(found==npc_combat_.end()||found->second.hex_scope()!=target)return false;
                advance_npc_conditions_locked(target.actor,found->second,now);
                found=npc_combat_.find(target.actor);placed=npcs_.find(target.actor);
                if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=target)return false;
                const auto life=found->second.lifetime();const auto* before=found->second.actor(life);
                if(!before||!before->health||before->health->current<=0)return false;
                const bool follow=plan.steal_above_half&&before->health->current>double(before->health->maximum)/2;
                auto proposal=found->second;
                if(!preflight_npc_reversal_or_damage_locked(proposal,damage,now))return false;
                if(follow&&proposal.actor(life)->health->current>0){
                    const auto registry=proposal.owned_effect_snapshot(life);
                    if(registry&&registry->newest_skill(target,307,now)){
                        if(!preview_npc_reversal_life_steal_locked(proposal,target,plan.steal_above_half,now))return false;
                    }else if(!proposal.apply_shadow_followup(life,plan,{source.precise_health,source.resources->max_health},now))return false;
                }
                if(publication)publish_target_body_locked(target.actor,plan.skill);
                if(!commit_npc_reversal_or_damage_locked(source,placed->second,found->second,damage,now))
                    throw std::logic_error("preflighted shadow NPC damage failed");
                found=npc_combat_.find(target.actor);placed=npcs_.find(target.actor);
                if(found!=npc_combat_.end()&&placed!=npcs_.end()&&found->second.hex_scope()==target){
                    auto& live=found->second;const auto* current=live.actor(life);
                    if(follow&&source.precise_health>0&&current&&current->health&&current->health->current>0){
                        const auto registry=live.owned_effect_snapshot(life);
                        if(registry&&registry->newest_skill(target,307,now)){
                            const auto reduction=live.life_steal_reduction(life,now);
                            if(!reduction||(plan.steal_above_half>*reduction&&!commit_reversal_life_steal_transfer_locked(scope,target,plan.steal_above_half,now)))
                                std::cerr<<"Shadow late NPC refusal actor="<<target.actor<<" reason=life_steal"<<std::endl;
                        }else{
                            auto transfer=live.apply_shadow_followup(life,plan,{source.precise_health,source.resources->max_health},now);
                            if(!transfer)throw std::logic_error("preflighted shadow NPC transfer failed");
                            health_event_locked(source,transfer->gained,now);transfer->source.current=source.precise_health;
                            for(auto& [id,peer]:players_)peer.outbox->enqueue(skills::effects::life_steal_packets(scope.actor,target.actor,*transfer));
                            if(transfer->killed)record_npc_death_locked(source,placed->second,live,now);
                        }
                        if(source.precise_health<=0)kill_locked(source,now);
                    }
                    // Life-steal death and source cleanup can retire actors.
                    // Never attach the legacy late particle to a reused ID.
                    const auto current_target=npc_combat_.find(target.actor);
                    if(!publication&&current_target!=npc_combat_.end()&&current_target->second.hex_scope()==target)
                        publish_target_body_locked(target.actor,plan.skill);
                }
                return true;
            }
            auto& victim=b->second;
            if(victim.effect_scope!=target||!source.team_token||!victim.team_token||source.team_token==victim.team_token||
               !source.resources||!victim.resources||source.precise_health<=0||victim.precise_health<=0||source.knocked_down_until>now||
               source.surface.plane!=victim.surface.plane||now<source.health_updated||now<victim.health_updated)return false;
            const auto origin=healing_position_locked(source,now),destination=healing_position_locked(victim,now);
            if(!origin||!destination||std::hypot(origin->x-destination->x,origin->z-destination->z)>plan.range)return false;
            if(!advance_health_locked(source,now)||!advance_health_locked(victim,now)||source.precise_health<=0||victim.precise_health<=0)return false;
            const auto actor_at=[&](Player& p,combat::Health health){return incoming_spell_actor_locked(p,
                {p.id,p.effect_scope.generation,health,{},p.effects,true},now);};
            double projected=0;
            if(!admit_player_damage_sequence_locked(victim,{damage},now,actor_at,&projected)||source.precise_health<=0)return false;
            // The predicate belongs to the pre-hit target. Seed healing from
            // the hit can change survival/stealable Health, never eligibility.
            const bool follow=plan.steal_above_half&&victim.precise_health>double(victim.resources->max_health)/2;
            if(follow&&projected>0&&!skills::effects::steal_life_amount(plan.steal_above_half,
                {source.precise_health,source.resources->max_health},{projected,victim.resources->max_health},shielding_reduction_locked(victim,now)))return false;
            auto actor=actor_at(victim,{victim.precise_health,victim.resources->max_health});const auto hit=combat::apply(actor,damage,true);
            if(follow&&projected>0&&hit.damage<=0&&victim.effect_registry->newest_skill(target,307,now)){
                const auto reduced=std::max(0.,double(plan.steal_above_half)-shielding_reduction_locked(victim,now));
                if(reduced>0&&!preview_player_reversal_life_steal_locked(victim,reduced,now))return false;
            }
            if(publication)publish_target_body_locked(victim.id,plan.skill);
            if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(victim,hit,scope.actor,now,false,nullptr,false,true,nullptr,scope))
                throw std::logic_error("preflighted shadow player damage failed");
            if(follow&&source.precise_health>0&&victim.precise_health>0){
                if(victim.effect_registry->newest_skill(target,307,now)){
                    const auto reduced=std::max(0.,double(plan.steal_above_half)-shielding_reduction_locked(victim,now));
                    if(reduced>0&&!commit_reversal_life_steal_transfer_locked(scope,target,plan.steal_above_half,now))
                        std::cerr<<"Shadow late player refusal actor="<<target.actor<<" reason=life_steal"<<std::endl;
                }else{
                auto transfer=skills::effects::steal_life_amount(plan.steal_above_half,{source.precise_health,source.resources->max_health},
                    {victim.precise_health,victim.resources->max_health},shielding_reduction_locked(victim,now));
                if(!transfer)throw std::logic_error("preflighted shadow player transfer failed");
                health_event_locked(source,transfer->gained,now);health_event_locked(victim,-transfer->stolen,now);
                transfer->source.current=source.precise_health;transfer->target.current=victim.precise_health;transfer->killed=victim.precise_health<=0;
                for(auto& [id,peer]:players_)peer.outbox->enqueue(skills::effects::life_steal_packets(scope.actor,target.actor,*transfer));
                if(victim.passive_recovery&&transfer->stolen>0)victim.passive_recovery->record(recovery::Event::health_loss,now);
                if(victim.precise_health<=0)kill_locked(victim,now);
                }
                if(source.precise_health<=0)kill_locked(source,now);
            }
            if(!publication)publish_target_body_locked(victim.id,plan.skill);return true;
        };
        const bool accepted=perform();dispatch_fevered_boundary_locked(now);
        auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();return accepted;
    }
