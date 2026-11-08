// Included inside Instance. Trusted integration;normal casting remains gated.
public:
    bool stage_action_damage_hex(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::ActionDamageHexPlan& plan,unsigned level,std::uint64_t now,
            std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();const auto canonical=skills::effects::action_damage_hex(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255||now>UINT64_MAX-plan.duration)return false;
        std::lock_guard lock(mutex_);if(!settle_timed_effects_locked(now))return false;
        auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.resources||!caster->second.effect_registry||
           !advance_resource_event_locked(caster->second,now)||caster->second.precise_health<=0||caster->second.knocked_down_until>now)return false;
        const auto origin=healing_position_locked(caster->second,now);if(!origin)return false;
        float x=0,y=0;std::uint16_t plane=0;combat::weapon::AttackActionFacts action;
        auto player=players_.find(target.actor);auto npc=npc_combat_.find(target.actor);
        if(player!=players_.end()){
            auto& p=player->second;
            if(p.effect_scope!=target||!p.resources||!p.effect_registry||!p.health_regeneration||!p.team_token||
               !caster->second.team_token||p.team_token==caster->second.team_token||!advance_resource_event_locked(p,now)||p.precise_health<=0)return false;
            const auto point=healing_position_locked(p,now);if(!point)return false;x=point->x;y=point->z;plane=p.surface.plane;
            action=player_attack_action_locked(p,now);
        }else{
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
            advance_npc_conditions_locked(target.actor,npc->second,now);refresh_npc_movement_locked(target.actor,now);
            npc=npc_combat_.find(target.actor);const auto body=npcs_.find(target.actor);
            if(npc==npc_combat_.end()||body==npcs_.end()||npc->second.hex_scope()!=target||!npc->second.hostile_to_players(npc->second.lifetime()))return false;
            const auto* actor=npc->second.actor(npc->second.lifetime());if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)return false;
            x=body->second.x;y=body->second.y;plane=body->second.plane;
            if(const auto token=npc->second.active_action(npc->second.lifetime(),now)){
                action.kind=token->kind==combat::npcs::control::Action::attack?combat::weapon::AttackTargetAction::attack:combat::weapon::AttackTargetAction::skill;action.skill=token->skill;
            }
        }
        if(plane!=caster->second.surface.plane||std::hypot(x-origin->x,y-origin->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,{x,y});if(!line||line->plane!=plane)return false;}
        const auto reaction=skills::effects::prepare_action_damage_hex(plan,action);if(!reaction)return false;
        const auto duration=skills::effects::hex_duration(plan.skill,plan.duration,source,caster->second.effect_registry->active(source,now),now);if(!duration)return false;
        // Hex Breaker prevents the selected application,including its initial
        // conditional damage. Probe before publishing any recipient state.
        if(player!=players_.end()){
            const auto blocked=prevent_selected_hex_locked(caster->second,player->second,plan.skill,now,false);if(!blocked)return false;
            if(*blocked){const auto done=prevent_selected_hex_locked(caster->second,player->second,plan.skill,now);return done&&*done;}
        }
        // Retain scope identities across callbacks;never carry recipient pointers.
        std::vector<skills::effects::Scope> contacts;
        if(reaction->initial_damage){
            if(reaction->damage_radius>0){
                std::vector<skills::effects::Scope> moving;
                for(const auto&[id,live]:npc_combat_)moving.push_back(live.hex_scope());
                for(const auto scope:moving){const auto live=npc_combat_.find(scope.actor);
                    if(live!=npc_combat_.end()&&live->second.hex_scope()==scope)refresh_npc_movement_locked(scope.actor,now);
                }
                caster=players_.find(source.actor);
                if(caster==players_.end()||caster->second.effect_scope!=source||caster->second.precise_health<=0)return false;
            }
            for(const auto&[id,p]:players_){
                if(!p.resources||p.precise_health<=0||p.surface.plane!=plane||!p.team_token||p.team_token==caster->second.team_token)continue;
                const auto at=healing_position_locked(p,now);if(!at)return false;
                if(p.effect_scope==target||(reaction->damage_radius>0&&std::hypot(at->x-x,at->z-y)<=reaction->damage_radius))contacts.push_back(p.effect_scope);
            }
            for(const auto&[id,live]:npc_combat_){const auto body=npcs_.find(id);const auto* actor=live.actor(live.lifetime());
                if(body==npcs_.end()||body->second.plane!=plane||!live.hostile_to_players(live.lifetime())||!actor||!actor->health||actor->health->current<=0||!actor->damageable)continue;
                if(live.hex_scope()==target||(reaction->damage_radius>0&&std::hypot(body->second.x-x,body->second.y-y)<=reaction->damage_radius))contacts.push_back(live.hex_scope());
            }
        }
        const combat::Damage damage{combat::Kind::armor_ignoring,double(reaction->initial_damage),level,plan.skill};
        for(const auto scope:contacts){
            if(auto p=players_.find(scope.actor);p!=players_.end()){
                if(p->second.effect_scope!=scope)continue;
                if(!advance_resource_event_locked(p->second,now))return false;if(p->second.precise_health<=0)continue;
                auto actor=incoming_spell_actor_locked(p->second,{scope.actor,scope.generation,combat::Health{p->second.precise_health,p->second.resources->max_health},{},p->second.effects,true},now);
                if(combat::apply(actor,damage,true).reason!=combat::Reason::accepted)return false;
            }else{
                auto live=npc_combat_.find(scope.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=scope)continue;
                advance_npc_conditions_locked(scope.actor,live->second,now);live=npc_combat_.find(scope.actor);
                if(live==npc_combat_.end()||live->second.hex_scope()!=scope)continue;const auto* actor=live->second.actor(live->second.lifetime());
                if(!actor||!actor->health||actor->health->current<=0)continue;
                auto preview=live->second;if(preview.apply_damage(preview.lifetime(),damage,now).reason!=combat::Reason::accepted)return false;
            }
        }
        caster=players_.find(source.actor);player=players_.find(target.actor);npc=npc_combat_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||caster->second.precise_health<=0)return false;
        unsigned effect=0;
        if(player!=players_.end()){
            auto& p=player->second;if(p.effect_scope!=target||p.precise_health<=0||p.next_health_effect>UINT32_MAX)return false;
            // Settlement may have retired a target action;never replace the
            // preflight predicate with an animation-derived approximation.
            const auto current=skills::effects::prepare_action_damage_hex(plan,player_attack_action_locked(p,now));
            if(!current||current->initial_damage!=reaction->initial_damage)return false;
            effect=static_cast<unsigned>(p.next_health_effect);
            const auto application=skills::effects::prepare_action_damage_hex_application(*p.effect_registry,source,target,effect,plan,action,now,duration);if(!application)return false;
            auto health=*p.health_regeneration;
            if(!health.apply(target,{effect,source.actor,plan.skill,-int(plan.pips),now,*application->entry.expires}))return false;
            *p.effect_registry=application->registry;*p.health_regeneration=std::move(health);++p.next_health_effect;
            std::vector<Packet> packets;
            for(const auto& old:application->change.removed){p.health_effects.erase(old.instance);packets.push_back(health_effect_remove(p.id,old.instance));}
            p.health_effects.emplace(effect,application->entry);packets.push_back(health_effect_add(application->entry,p.id,now));
            for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);publish_status_locked(p,now);
        }else{
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target||next_npc_hex_==UINT32_MAX)return false;
            auto proposed=npc->second;const auto application=proposed.apply_action_damage_hex(proposed.lifetime(),source,plan,next_npc_hex_,now,duration);
            if(!application||application->reaction.initial_damage!=reaction->initial_damage)return false;
            effect=next_npc_hex_++;npc->second=std::move(proposed);npc_condition_credit_[{target.actor,source.actor}]={target,source};
            for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,npc->second.actor(npc->second.lifetime())->effects)});
        }
        // Application reactions can kill/retire recipients. Publish ownership
        // first,then re-resolve each scoped damage contact after callbacks.
        caster=players_.find(source.actor);player=players_.find(target.actor);
        if(caster!=players_.end()&&caster->second.effect_scope==source&&caster->second.passive_recovery)
            caster->second.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
        if(player!=players_.end()&&player->second.effect_scope==target&&player->second.passive_recovery)
            player->second.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
        commit_applications_locked(target,now);
        for(const auto scope:contacts)reactive_hex_damage_locked(scope,{scope,source,effect,level},plan.skill,reaction->initial_damage,now);
        const auto remaining_player=players_.find(target.actor);const auto remaining_npc=npc_combat_.find(target.actor);
        if((remaining_player!=players_.end()&&remaining_player->second.effect_scope==target)||
           (remaining_npc!=npc_combat_.end()&&remaining_npc->second.hex_scope()==target))publish_target_body_locked(target.actor,plan.skill,source.actor,visuals);
        return true;
    }
private:
