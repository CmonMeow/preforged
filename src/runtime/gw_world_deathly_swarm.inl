// Delayed105 contacts: refresh candidates and scoped authority per contact.
private:
    void resolve_deathly_swarms_locked(std::uint64_t now){
        if(!timed_effect_boundary_){settle_damage_fields_locked(now);return;}
        auto queued=std::move(deathly_swarms_);deathly_swarms_.clear();
        for(auto& shot:queued){
            if(!shot.chain.pending())continue;
            if(shot.chain.due()>now){deathly_swarms_.push_back(std::move(shot));continue;}
            const auto source_scope=shot.chain.caster();
            const auto source_at=[&]()->Player*{
                const auto found=players_.find(source_scope.actor);
                return found!=players_.end()&&found->second.effect_scope==source_scope&&
                    found->second.surface.plane==shot.plane&&found->second.precise_health>0?&found->second:nullptr;
            };
            auto* caster=source_at();if(!caster||!advance_health_locked(*caster,now)||!source_at())continue;
            while(shot.chain.pending()&&shot.chain.due()<=now){
                caster=source_at();if(!caster){shot.chain.cancel();break;}
                const auto team=caster->team_token;
                std::vector<skills::effects::SwarmCandidate> candidates;
                for(auto&[id,p]:players_)if(p.surface.plane==shot.plane){
                    if(p.resources&&!advance_health_locked(p,now))continue;
                    const auto position=healing_position_locked(p,now);if(!position)continue;
                    candidates.push_back({p.effect_scope,position->x,position->z,p.resources&&p.precise_health>0,
                        p.team_token&&team&&p.team_token!=team});
                }
                // Condition settlement and death callbacks can retire quest NPCs.
                // Never retain map iterators or actor references over that boundary.
                std::vector<skills::effects::Scope> npc_scopes;
                for(const auto&[id,live]:npc_combat_)npc_scopes.push_back(live.hex_scope());
                for(const auto expected:npc_scopes){
                    auto live=npc_combat_.find(expected.actor);
                    if(live==npc_combat_.end()||live->second.hex_scope()!=expected)continue;
                    advance_npc_conditions_locked(expected.actor,live->second,now);
                    live=npc_combat_.find(expected.actor);
                    if(live==npc_combat_.end()||live->second.hex_scope()!=expected)continue;
                    refresh_npc_movement_locked(expected.actor,now);
                    const auto npc=npcs_.find(expected.actor);if(npc==npcs_.end()||npc->second.plane!=shot.plane)continue;
                    const auto life=live->second.lifetime();const auto* actor=live->second.actor(life);
                    candidates.push_back({expected,npc->second.x,npc->second.y,
                        actor&&actor->health&&actor->health->current>0,live->second.hostile_to_players(life)});
                }
                // Stable actor order is private-server tie policy, not a retail claim.
                std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.scope.actor<b.scope.actor;});
                caster=source_at();if(!caster){shot.chain.cancel();break;}
                const auto strike=shot.chain.advance(source_scope,now,candidates);if(!strike)break;
                // A refused contact consumes its due ordinal, matching the previous
                // fail-closed policy. Never replay it at the next tick or duplicate damage.
                const auto& plan=shot.chain.plan();
                const combat::Damage damage{combat::Kind::cold,plan.damage,shot.level,105};
                auto player=players_.find(strike->target.actor);
                if(player!=players_.end()){
                    if(player->second.effect_scope!=strike->target||!player->second.resources||player->second.precise_health<=0)continue;
                    const auto actor_at=[&](const Player& p,combat::Health health){
                        return incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,health,{},p.effects,true,
                            {},p.reviewed_fire_armor,p.reviewed_cold_armor},now,true);
                    };
                    if(!admit_player_damage_sequence_locked(player->second,{damage},now,actor_at))continue;
                    player=players_.find(strike->target.actor);
                    caster=source_at();if(!caster){shot.chain.cancel();break;}
                    if(player==players_.end()||player->second.effect_scope!=strike->target||
                       !player->second.resources||player->second.precise_health<=0)continue;
                    auto& p=player->second;
                    auto actor=actor_at(p,{p.precise_health,p.resources->max_health});
                    const auto hit=combat::apply(actor,damage,true);
                    if(hit.reason!=combat::Reason::accepted)continue;
                    if(p.passive_recovery)p.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
                    if(!commit_player_reactive_hit_locked(p,hit,source_scope.actor,now,false,nullptr,false,true,nullptr,source_scope))continue;
                    if(const auto current=players_.find(strike->target.actor);current!=players_.end()&&current->second.effect_scope==strike->target)
                        publish_target_body_locked(strike->target.actor,105);
                }else{
                    auto npc=npcs_.find(strike->target.actor);auto live=npc_combat_.find(strike->target.actor);
                    if(npc==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=strike->target)continue;
                    auto preview=live->second;if(!preflight_npc_reversal_or_damage_locked(preview,damage,now))continue;
                    caster=source_at();if(!caster){shot.chain.cancel();break;}
                    const bool committed=commit_npc_reversal_or_damage_locked(caster,npc->second,live->second,damage,now,
                        nullptr,nullptr,nullptr,false,source_scope);
                    live=npc_combat_.find(strike->target.actor);
                    if(committed&&live!=npc_combat_.end()&&live->second.hex_scope()==strike->target)
                        publish_target_body_locked(strike->target.actor,105);
                }
            }
            if(shot.chain.pending())deathly_swarms_.push_back(std::move(shot));
        }
    }
