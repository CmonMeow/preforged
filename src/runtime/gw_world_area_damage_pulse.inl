// Shared whole-cohort area damage admission/commit. Callers own geometry,
// source policy, timing and visuals; reactions use the same projected state.
private:
    struct AreaDamageCentre {navigation::Point point;std::size_t plane;};
    struct AreaDamageCondition {combat::conditions::Kind kind;std::uint64_t duration;};
    struct AreaDamageRecipient {skills::effects::Scope scope;bool condition=false;bool opposed_owned=false;};
    struct AreaDamageRecipients {std::vector<AreaDamageRecipient> players,npcs;};
    combat::Actor area_damage_actor_locked(const Player& p,combat::Health health,std::uint64_t at)const{
        combat::Actor actor{p.id,p.effect_scope.generation,health,p.reviewed_earth_armor,p.effects,true,
            p.reviewed_lightning_armor,p.reviewed_fire_armor,p.reviewed_cold_armor};
        actor.physical_armor=p.reviewed_physical_armor;actor.holy_armor=p.reviewed_holy_armor;
        return incoming_spell_actor_locked(p,actor,at,true);
    }
    std::optional<AreaDamageRecipients> admit_area_damage_pulse_locked(
            skills::effects::Scope source,const combat::Damage& damage,const AreaDamageCentre& centre,
            float radius,std::uint64_t at,unsigned team,std::optional<AreaDamageCondition> condition,
            const std::function<std::optional<bool>(const Player&)>& player_condition,
            const std::function<std::optional<bool>(const combat::npcs::LiveActor&)>& npc_condition,std::optional<skills::effects::Scope> excluded={}){
        AreaDamageRecipients recipients;
        for(auto&[id,p]:players_){
            if(excluded&&p.effect_scope==*excluded)continue;
            if(!p.resources||p.precise_health<=0||!p.team_token||p.team_token==team||p.surface.plane!=centre.plane)continue;
            if(!advance_health_locked(p,at))return {};
            const auto point=healing_position_locked(p,at);if(!point)return {};
            if(p.precise_health<=0||std::hypot(point->x-centre.point.x,point->z-centre.point.z)>radius)continue;
            const auto applies=condition?player_condition(p):std::optional(false);if(!applies)return {};
            recipients.players.push_back({p.effect_scope,*applies});
        }
        std::vector<unsigned> ids;for(const auto&[id,n]:npcs_)ids.push_back(id);
        for(const auto id:ids){auto live=npc_combat_.find(id);if(live==npc_combat_.end())continue;
            const auto scope=live->second.hex_scope();if(excluded&&scope==*excluded)continue;
            const auto caster=players_.find(source.actor);
            const bool current_source=caster!=players_.end()&&caster->second.effect_scope==source;
            const auto foe=current_source?player_npc_attack_relation_locked(caster->second,live->second):std::optional(live->second.hostile_to_players(live->second.lifetime()));
            if(foe&&!*foe)continue;
            advance_npc_conditions_locked(id,live->second,at);refresh_npc_movement_locked(id,at);
            live=npc_combat_.find(id);const auto body=npcs_.find(id);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=scope||body->second.plane!=centre.plane)continue;
            const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
            if(!actor||!actor->damageable||!actor->health||actor->health->current<=0||
               std::hypot(body->second.x-centre.point.x,body->second.y-centre.point.z)>radius)continue;
            if(!foe)return {};
            const bool owned=!live->second.hostile_to_players(life);
            bool applies=false;
            if(condition&&live->second.allows_conditions(life)){
                const auto classified=npc_condition(live->second);if(!classified)return {};applies=*classified;
            }
            recipients.npcs.push_back({scope,applies,owned});
        }
        std::map<skills::effects::Scope,skills::effects::HealingSeedReaction> facts;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            for(const auto& recipient:recipients.players){
                const auto p=players_.find(recipient.scope.actor);if(p==players_.end()||p->second.effect_scope!=recipient.scope)continue;
                auto* state=admission_player_locked(batch,p->second,at);if(!state){valid=false;break;}
                if(state->health.current<=0)continue;
                auto actor=area_damage_actor_locked(p->second,
                    combat::Health{state->health.current,state->health.capacity.maximum()},at);
                if(!project_batch_player_damage_locked(batch,p->second,actor,damage,at)){valid=false;break;}
                if(!recipient.condition||state->health.current<=0)continue;
                auto conditions=p->second.conditions;
                if(!conditions.apply(condition->kind,source.actor,at,condition->duration)){valid=false;break;}
                const auto before=p->second.conditions.lookup(condition->kind),after=conditions.lookup(condition->kind);
                if((!before||before->expires!=after->expires||!p->second.condition_effects.contains(condition->kind))&&p->second.next_health_effect>UINT32_MAX){valid=false;break;}
                if(!project_batch_condition_reactions_locked(batch,p->second,p->second.conditions,conditions,at)){valid=false;break;}
            }
            if(batch.missing_seed){const auto target=*batch.missing_seed;const auto p=players_.find(target.actor);
                if(p==players_.end()||p->second.effect_scope!=target)return {};
                const auto fact=prepare_healing_seed_direct_locked(p->second,1.,at);if(!fact)return {};
                facts[target]=*fact;continue;}
            if(!valid)return {};
            for(const auto& recipient:recipients.npcs){const auto live=npc_combat_.find(recipient.scope.actor);
                if(live==npc_combat_.end()||live->second.hex_scope()!=recipient.scope)continue;
                auto preview=batch.npcs.contains(recipient.scope)?batch.npcs.at(recipient.scope):live->second;const auto life=preview.lifetime();
                if(!preflight_npc_reversal_or_damage_locked(preview,damage,at,nullptr,nullptr,nullptr,nullptr,recipient.opposed_owned))return {};
                const auto actor=preview.actor(life);if(!actor||!actor->health)return {};
                if(recipient.condition&&actor->health->current>0&&!preview.apply_condition(life,condition->kind,source.actor,at,condition->duration))return {};
                for(const auto& hit:preview.take_fragility_hits(life))if(hit.reversal&&!validate_npc_reversal_receipts_locked(recipient.scope,{hit.reversal}))return {};
                batch.npcs.insert_or_assign(recipient.scope,std::move(preview));
            }
            return recipients;
        }
        return {};
    }
    void commit_area_damage_pulse_locked(skills::effects::Scope source_scope,const combat::Damage& damage,
            std::uint64_t at,const AreaDamageRecipients& recipients,std::optional<AreaDamageCondition> condition,
            const std::function<void(skills::effects::Scope)>& particle={}){
        for(const auto& recipient:recipients.players){auto found=players_.find(recipient.scope.actor);
            if(found==players_.end()||found->second.effect_scope!=recipient.scope||!found->second.resources||found->second.precise_health<=0)continue;
            auto& p=found->second;
            auto actor=area_damage_actor_locked(p,combat::Health{p.precise_health,p.resources->max_health},at);
            const auto hit=combat::apply(actor,damage,true);
            if(particle)particle(recipient.scope);
            if(!commit_player_reactive_hit_locked(p,hit,source_scope.actor,at,false,nullptr,false,true,nullptr,source_scope))throw std::logic_error("admitted area pulse player hit failed");
            found=players_.find(recipient.scope.actor);
            if(!recipient.condition||found==players_.end()||found->second.effect_scope!=recipient.scope||found->second.precise_health<=0)continue;
            auto conditions=found->second.conditions;
            if(!conditions.apply(condition->kind,source_scope.actor,at,condition->duration))throw std::logic_error("admitted area pulse player condition failed");
            commit_conditions_locked(found->second,conditions,{condition->kind},at);
        }
        for(const auto& recipient:recipients.npcs){auto found=npc_combat_.find(recipient.scope.actor);const auto body=npcs_.find(recipient.scope.actor);
            if(found==npc_combat_.end()||body==npcs_.end()||found->second.hex_scope()!=recipient.scope)continue;
            const auto actor=found->second.actor(found->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)continue;
            const auto source=players_.find(source_scope.actor);
            Player* credit=source!=players_.end()&&source->second.effect_scope==source_scope?&source->second:nullptr;
            if(recipient.opposed_owned){const auto foe=credit?player_npc_attack_relation_locked(*credit,found->second):std::nullopt;if(!foe||!*foe)continue;}
            if(particle)particle(recipient.scope);
            if(!commit_npc_reversal_or_damage_locked(credit,body->second,found->second,damage,at,nullptr,nullptr,nullptr,false,source_scope,nullptr,recipient.opposed_owned))throw std::logic_error("admitted area pulse NPC hit failed");
            found=npc_combat_.find(recipient.scope.actor);
            if(!recipient.condition||found==npc_combat_.end()||found->second.hex_scope()!=recipient.scope)continue;
            const auto life=found->second.lifetime();const auto current=found->second.actor(life);
            if(!current||!current->health||current->health->current<=0||!found->second.allows_conditions(life))continue;
            if(!found->second.apply_condition(life,condition->kind,source_scope.actor,at,condition->duration))throw std::logic_error("admitted area pulse NPC condition failed");
            npc_condition_credit_[{recipient.scope.actor,source_scope.actor}]={recipient.scope,source_scope};
            for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(recipient.scope.actor,found->second.actor(life)->effects)});
            const bool reacted=publish_npc_fragility_locked(recipient.scope.actor,found->second);
            found=npc_combat_.find(recipient.scope.actor);
            if(found!=npc_combat_.end()&&found->second.hex_scope()==recipient.scope&&!reacted)commit_applications_locked(recipient.scope,at);
        }
        dispatch_fevered_boundary_locked(at);
    }
