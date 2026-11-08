    bool whirlwind_locked(skills::effects::Scope scope,const skills::effects::ElementalPlan& plan,unsigned level,std::uint64_t now,std::vector<Packet>* visuals,std::optional<std::uint64_t> completion_event={},bool completion_publication=false){
        const auto source=players_.find(scope.actor);
        if(source==players_.end()||source->second.effect_scope!=scope||!source->second.resources||!source->second.team_token||
           now<source->second.health_updated||now>UINT64_MAX-2000||!advance_resource_event_locked(source->second,now)||
           source->second.precise_health<=0||source->second.knocked_down_until>now)return false;
        auto& caster=source->second;const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        const float radius=caster.overcast_curve&&caster.overcast_curve->active()?252.f:166.f;
        using Candidate=std::pair<skills::effects::Scope,bool>;
        std::vector<Candidate> player_targets,npc_targets;
        auto player_result=[&](Player& p){
            return skills::effects::elemental_target(plan,incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
                combat::Health{p.precise_health,p.resources->max_health},std::nullopt,p.effects,true,std::nullopt,
                p.reviewed_fire_armor,p.reviewed_cold_armor},now),p.conditions,level,caster.id,now,false,1,true,{},player_damage_projection_locked(p,now));
        };
        for(auto&[id,p]:players_){
            if(!p.resources||!p.team_token||p.team_token==caster.team_token||p.precise_health<=0||p.surface.plane!=caster.surface.plane)continue;
            const auto point=healing_position_locked(p,now);if(!point)return false;
            if(std::hypot(point->x-origin->x,point->z-origin->z)>radius)continue;
            if(!p.reviewed_cold_armor||now<p.health_updated||!advance_health_locked(p,now))return false;
            if(p.precise_health<=0)continue;
            const bool attacking=p.ordinary_animation||now<p.melee_skill_until;
            const auto result=player_result(p);if(!result.accepted)return false;
            if(attacking&&result.actor.health->current>0&&p.control_epoch==UINT64_MAX)return false;
            player_targets.emplace_back(p.effect_scope,attacking);
        }
        const combat::Damage damage{combat::Kind::cold,plan.damage,level,163};
        using K=combat::npcs::control::Knockdown;
        std::vector<skills::effects::Scope> npc_scopes;
        for(const auto& [id,live]:npc_combat_)npc_scopes.push_back(live.hex_scope());
        for(const auto owner:npc_scopes){
            auto live=npc_combat_.find(owner.actor);auto placed=npcs_.find(owner.actor);
            if(live==npc_combat_.end()||placed==npcs_.end()||live->second.hex_scope()!=owner||
               placed->second.plane!=caster.surface.plane||!live->second.hostile_to_players(live->second.lifetime()))continue;
            refresh_npc_movement_locked(owner.actor,now);
            if(std::hypot(placed->second.x-origin->x,placed->second.y-origin->z)>radius)continue;
            if(now<live->second.health_clock(live->second.lifetime()).value_or(now))return false;
            advance_npc_conditions_locked(owner.actor,live->second,now);
            live=npc_combat_.find(owner.actor);placed=npcs_.find(owner.actor);
            if(live==npc_combat_.end()||placed==npcs_.end()||live->second.hex_scope()!=owner)continue;
            const auto life=live->second.lifetime();const auto* actor=live->second.actor(life);
            if(!actor||!actor->health||actor->health->current<=0||!actor->damageable)continue;
            npc_targets.emplace_back(owner,live->second.attacking(life,now));
        }
        // Project the entire ordered cohort, including healing received between
        // hits. Discovering a Seed recipient timeline discards the projection.
        std::map<skills::effects::Scope,skills::effects::HealingSeedReaction> facts;
        bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            for(const auto& [owner,attacking]:player_targets){
                const auto found=players_.find(owner.actor);if(found==players_.end()||found->second.effect_scope!=owner)continue;
                auto& p=found->second;auto* state=admission_player_locked(batch,p,now);if(!state)return false;
                if(state->health.current<=0)continue;
                auto actor=incoming_spell_actor_locked(p,{p.id,owner.generation,
                    combat::Health{state->health.current,state->health.capacity.maximum()},std::nullopt,p.effects,true,std::nullopt,
                    p.reviewed_fire_armor,p.reviewed_cold_armor},now);
                const combat::DamageProjection projection=[&](combat::Actor& current,const combat::Damage& hit){
                    return project_batch_player_damage_locked(batch,p,current,hit,now);};
                const auto result=skills::effects::elemental_target(plan,actor,p.conditions,level,caster.id,now,false,1,true,{},projection);
                if(!result.accepted){valid=false;break;}
                if(attacking&&state->health.current>0&&p.control_epoch==UINT64_MAX)return false;
            }
            if(batch.missing_seed){const auto owner=*batch.missing_seed;const auto p=players_.find(owner.actor);
                if(p==players_.end()||p->second.effect_scope!=owner)return false;
                const auto fact=prepare_healing_seed_direct_locked(p->second,1.,now);if(!fact)return false;
                facts[owner]=*fact;continue;}
            if(!valid)return false;
            for(auto& [owner,knock]:npc_targets){
                const auto live=npc_combat_.find(owner.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=owner)continue;
                auto proposed=batch.npcs.contains(owner)?batch.npcs.at(owner):live->second;const auto life=proposed.lifetime();
                const bool attacking=proposed.attacking(life,now);knock=false;
                if(!preflight_npc_reversal_or_damage_locked(proposed,damage,now))return false;
                if(attacking&&proposed.actor(life)->health->current>0){const auto control=proposed.knock_down(life,now,2000);
                    if(control==K::invalid||control==K::unknown)return false;knock=control==K::accepted;}
            }
            admitted=true;break;
        }
        if(!admitted)return false;
        // Commit only after both populations pass admission. Recompute player
        // HP-dependent results after any earlier death-triggered return heals.
        if(completion_event&&!complete_ether_renewal_locked(scope,plan.skill,*completion_event,now,true))return false;
        if(caster.precise_health<=0)return true;
        for(const auto&[target,attacking]:player_targets){
            auto& p=players_.at(target.actor);if(p.effect_scope!=target||p.precise_health<=0)continue;
            const auto particle=skills::particles::whirlwind_target_2022(p.id,caster.id);
            if(completion_publication)for(auto&[id,peer]:players_)peer.outbox->enqueue({particle});
            const auto result=player_result(p);if(!result.accepted)throw std::logic_error("preflighted Whirlwind player failed");
            for(const auto& hit:result.hits){if(p.precise_health<=0)break;
                if(!commit_player_reactive_hit_locked(p,hit,caster.id,now,false,nullptr,false,true,nullptr,scope))throw std::logic_error("preflighted Whirlwind damage commit failed");}
            if(p.passive_recovery)p.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            if(p.precise_health>0&&attacking&&!knockdown_player_locked(p,2000,now))throw std::logic_error("preflighted Whirlwind player control failed");
            if(!completion_publication){if(visuals)visuals->push_back(particle);else for(auto&[id,peer]:players_)peer.outbox->enqueue({particle});}
        }
        for(const auto&[target,knock]:npc_targets){
            auto live=npc_combat_.find(target.actor);auto placed=npcs_.find(target.actor);
            if(live==npc_combat_.end()||placed==npcs_.end()||live->second.hex_scope()!=target)continue;
            const auto actor=live->second.actor(live->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)continue;
            const auto particle=skills::particles::whirlwind_target_2022(target.actor,caster.id);
            if(completion_publication)for(auto&[id,peer]:players_)peer.outbox->enqueue({particle});
            if(!commit_npc_reversal_or_damage_locked(caster,placed->second,live->second,damage,now))throw std::logic_error("preflighted Whirlwind NPC failed");
            live=npc_combat_.find(target.actor);placed=npcs_.find(target.actor);
            if(live==npc_combat_.end()||placed==npcs_.end()||live->second.hex_scope()!=target)continue;
            const auto current=live->second.actor(live->second.lifetime());if(!current||!current->health)continue;
            if(current->health->current>0&&knock&&knockdown_npc_locked(target,2000,now)!=K::accepted)throw std::logic_error("preflighted Whirlwind NPC control failed");
            if(!completion_publication){if(visuals)visuals->push_back(particle);else for(auto&[id,peer]:players_)peer.outbox->enqueue({particle});}
        }
        return true;
    }
