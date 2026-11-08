// Included inside Instance. Called after AttackOutcomes consumes the contact.
    bool whirling_resolved_locked(const combat::weapon::ResolvedAttack& event){
        using Scope=skills::effects::Scope;
        const auto source=players_.find(event.target.actor);
        if(source==players_.end()||source->second.effect_scope!=event.target||!source->second.effect_registry)return true;
        const auto reaction=skills::effects::whirling_resolved_retaliation(*source->second.effect_registry,event.target,event);
        if(!reaction)return true;
        const auto now=event.at;
        if(!advance_health_locked(source->second,now)||source->second.precise_health<=0)return false;
        const auto center=healing_position_locked(source->second,now);if(!center)return false;
        const auto plane=source->second.surface.plane;
        const auto team=source->second.team_token;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(reaction->damage),source->second.ordinary_level,450};
        // Settle prospective recipients before selecting any damage proposal.
        for(auto&[id,victim]:players_){
            if(!team||!victim.team_token||team==victim.team_token||!victim.resources||victim.surface.plane!=plane)continue;
            const auto point=healing_position_locked(victim,now);if(!point)return false;
            if(std::hypot(point->x-center->x,point->z-center->z)>reaction->radius)continue;
            if(!advance_health_locked(victim,now))return false;
        }
        std::vector<unsigned> npc_ids;
        for(const auto&[id,live]:npc_combat_)npc_ids.push_back(id);
        for(const auto id:npc_ids){
            auto live=npc_combat_.find(id);auto npc=npcs_.find(id);
            if(live==npc_combat_.end()||npc==npcs_.end()||npc->second.plane!=plane||!live->second.hostile_to_players(live->second.lifetime()))continue;
            refresh_npc_movement_locked(id,now);
            npc=npcs_.find(id);live=npc_combat_.find(id);
            if(npc==npcs_.end()||live==npc_combat_.end())continue;
            if(std::hypot(npc->second.x-center->x,npc->second.y-center->z)>reaction->radius)continue;
            advance_npc_conditions_locked(id,live->second,now);
        }
        const auto current_source=players_.find(event.target.actor);
        if(current_source==players_.end()||current_source->second.effect_scope!=event.target||current_source->second.precise_health<=0)return false;
        std::vector<Scope> player_targets,npc_targets;
        for(auto&[id,victim]:players_){
            if(!team||!victim.team_token||team==victim.team_token||!victim.resources||victim.surface.plane!=plane)continue;
            const auto point=healing_position_locked(victim,now);if(!point)return false;
            if(skills::effects::whirling_retaliation_target(*reaction,victim.effect_scope,true,victim.precise_health>0,
                    std::hypot(point->x-center->x,point->z-center->z)))player_targets.push_back(victim.effect_scope);
        }
        for(const auto&[id,live]:npc_combat_){
            const auto npc=npcs_.find(id);const auto life=live.lifetime();
            if(npc==npcs_.end()||npc->second.plane!=plane||!live.hostile_to_players(life))continue;
            if(std::hypot(npc->second.x-center->x,npc->second.y-center->z)>reaction->radius)continue;
            const auto* actor=live.actor(life);
            if(actor&&actor->damageable&&actor->health&&actor->health->current>0)npc_targets.push_back(live.hex_scope());
        }
        const auto player_hit=[&](Player& victim){
            auto actor=incoming_spell_actor_locked(victim,{victim.id,victim.effect_scope.generation,
                combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true},now);
            // Armor-ignoring physical damage still receives equipment's flat
            // physical reduction. Do not classify this as a weapon contact.
            actor.incoming_reduction+=victim.absorption;
            return combat::apply(actor,damage,true);
        };
        // Admit the entire selected batch before the first base hit. No copied
        // Health/registry proposal is retained across shared reaction callbacks.
        for(const auto scope:player_targets){
            const auto found=players_.find(scope.actor);
            if(found==players_.end()||found->second.effect_scope!=scope||found->second.precise_health<=0)continue;
            const auto hit=player_hit(found->second);
            if(hit.reason!=combat::Reason::accepted||!prepare_player_reactive_seed_locked(found->second,hit,now))return false;
        }
        for(const auto scope:npc_targets){
            const auto found=npc_combat_.find(scope.actor);
            if(found==npc_combat_.end()||found->second.hex_scope()!=scope)continue;
            auto preview=found->second;
            const auto* actor=preview.actor(preview.lifetime());
            if(!actor||!actor->health||actor->health->current<=0)continue;
            if(!preflight_npc_reversal_or_damage_locked(preview,damage,now))return false;
        }
        struct Contact {Scope scope;combat::Hit hit;};
        std::vector<Contact> contacts;
        bool accepted=true;
        const auto refused=[&](Scope scope){
            accepted=false;
            std::cerr<<"Whirling Defense late recipient refusal event="<<event.event<<" actor="<<scope.actor<<" generation="<<scope.generation<<std::endl;
        };
        for(const auto scope:player_targets){
            const auto found=players_.find(scope.actor);
            if(found==players_.end()||found->second.effect_scope!=scope||!found->second.resources||found->second.precise_health<=0)continue;
            const auto hit=player_hit(found->second);std::optional<combat::Hit> actual;
            if(hit.reason!=combat::Reason::accepted||
               !commit_player_reactive_hit_locked(found->second,hit,event.target.actor,now,true,nullptr,false,false,&actual)){
                refused(scope);continue;
            }
            if(actual&&actual->applied>0)contacts.push_back({scope,*actual});
        }
        for(const auto scope:npc_targets){
            const auto found=npc_combat_.find(scope.actor);const auto placed=npcs_.find(scope.actor);
            if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=scope)continue;
            const auto* actor=found->second.actor(found->second.lifetime());
            if(!actor||!actor->health||actor->health->current<=0)continue;
            const auto caster=players_.find(event.target.actor);
            Player* cause=caster!=players_.end()&&caster->second.effect_scope==event.target?&caster->second:nullptr;
            std::optional<combat::Hit> actual;
            if(!commit_npc_reversal_or_damage_locked(cause,placed->second,found->second,damage,now,nullptr,nullptr,&actual,true)){
                refused(scope);continue;
            }
            if(actual&&actual->applied>0)contacts.push_back({scope,*actual});
        }
        // Only explicit physical reactions are deferred until all base hits.
        // Shared conversion/Seed/end reactions may change later base recipients.
        std::vector<MarkOfPainTrigger> marks;
        for(const auto& contact:contacts)
            if(const auto mark=mark_of_pain_trigger_locked(contact.scope,0,contact.hit.applied,now))marks.push_back(*mark);
        for(const auto& contact:contacts)barbs_reaction_locked(contact.scope,0,contact.hit.applied,now);
        for(const auto& mark:marks)commit_mark_of_pain_locked(mark,now);
        for(const auto scope:player_targets){
            const auto found=players_.find(scope.actor);
            if(found!=players_.end()&&found->second.effect_scope==scope&&found->second.precise_health<=0)kill_locked(found->second,now);
        }
        for(const auto scope:npc_targets){
            const auto found=npc_combat_.find(scope.actor);const auto placed=npcs_.find(scope.actor);
            if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=scope)continue;
            const auto* actor=found->second.actor(found->second.lifetime());
            if(!actor||!actor->health||actor->health->current>0)continue;
            const auto caster=players_.find(event.target.actor);
            record_npc_death_locked(caster!=players_.end()&&caster->second.effect_scope==event.target?&caster->second:nullptr,
                placed->second,found->second,now);
        }
        return accepted;
    }
