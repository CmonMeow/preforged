// Included inside Instance. Trusted World stage;normal casting is still gated.
public:
    bool stage_icy_prism(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::IcyPrismPlan& plan,unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();const auto canonical=skills::effects::icy_prism(plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255)return false;
        std::lock_guard lock(mutex_);if(!settle_timed_effects_locked(now))return false;
        auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.resources||
           !advance_resource_event_locked(caster->second,now)||caster->second.precise_health<=0||caster->second.knocked_down_until>now)return false;
        std::vector<skills::effects::Scope> moving;for(const auto&[id,live]:npc_combat_)moving.push_back(live.hex_scope());
        for(const auto scope:moving){const auto live=npc_combat_.find(scope.actor);
            if(live!=npc_combat_.end()&&live->second.hex_scope()==scope)refresh_npc_movement_locked(scope.actor,now);}
        auto player=players_.find(target.actor);auto npc=npc_combat_.find(target.actor);
        std::optional<skills::effects::Registry> registry;float x=0,y=0;std::size_t plane=0;
        if(player!=players_.end()){
            auto& p=player->second;if(p.effect_scope!=target||!p.resources||!p.effect_registry||!advance_resource_event_locked(p,now)||p.precise_health<=0)return false;
            const auto at=healing_position_locked(p,now);if(!at)return false;x=at->x;y=at->z;plane=p.surface.plane;registry=*p.effect_registry;
        }else{
            if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
            advance_npc_conditions_locked(target.actor,npc->second,now);npc=npc_combat_.find(target.actor);const auto body=npcs_.find(target.actor);
            if(npc==npc_combat_.end()||body==npcs_.end()||npc->second.hex_scope()!=target)return false;
            if(const auto clock=npc->second.health_clock(npc->second.lifetime());clock&&*clock!=now)return false;
            x=body->second.x;y=body->second.y;plane=body->second.plane;registry=npc->second.hex_registry_snapshot(npc->second.lifetime(),now);
        }
        caster=players_.find(source.actor);if(!registry||caster==players_.end()||caster->second.effect_scope!=source||caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);
        if(!origin||plane!=caster->second.surface.plane||std::hypot(x-origin->x,y-origin->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,{x,y});if(!line||line->plane!=plane)return false;}
        std::vector<skills::effects::IcyPrismRecipient> candidates;
        for(auto&[id,p]:players_){const auto at=healing_position_locked(p,now);if(!at)return false;
            candidates.push_back({p.effect_scope,at->x,at->z,p.surface.plane,bool(p.team_token&&caster->second.team_token&&p.team_token!=caster->second.team_token),
                p.resources&&p.precise_health>0,bool(p.resources),p.reviewed_cold_armor});}
        for(const auto&[id,live]:npc_combat_){const auto body=npcs_.find(id);const auto* actor=live.actor(live.lifetime());
            if(body==npcs_.end()||!actor)continue;
            candidates.push_back({live.hex_scope(),body->second.x,body->second.y,body->second.plane,live.hostile_to_players(live.lifetime()),
                actor->health&&actor->health->current>0,actor->damageable,actor->cold_armor});}
        const auto selected=skills::effects::select_icy_prism(plan,{source,target,x,y,plane,*registry},candidates,now);if(!selected)return false;
        std::vector<skills::effects::IcyPrismDamageInput> inputs;
        for(const auto scope:selected->recipients){
            if(auto p=players_.find(scope.actor);p!=players_.end()){
                if(p->second.effect_scope!=scope||!advance_resource_event_locked(p->second,now)||p->second.precise_health<=0)return false;
                inputs.push_back({scope,incoming_spell_actor_locked(p->second,{scope.actor,scope.generation,combat::Health{p->second.precise_health,p->second.resources->max_health},{},p->second.effects,true,{},{},p->second.reviewed_cold_armor},now),p->second.conditions});
            }else{
                auto live=npc_combat_.find(scope.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=scope)return false;
                advance_npc_conditions_locked(scope.actor,live->second,now);live=npc_combat_.find(scope.actor);
                if(live==npc_combat_.end()||live->second.hex_scope()!=scope)return false;
                if(const auto clock=live->second.health_clock(live->second.lifetime());clock&&*clock!=now)return false;
                const auto* actor=live->second.actor(live->second.lifetime());const auto conditions=live->second.condition_snapshot(live->second.lifetime());
                if(!actor||!conditions)return false;inputs.push_back({scope,*actor,*conditions});
                auto preview=live->second;if(!preflight_npc_reversal_or_damage_locked(preview,{combat::Kind::cold,double(scope==target?plan.damage:plan.secondary_damage),level,903},now))return false;
            }
        }
        const auto proposed=skills::effects::prepare_icy_prism_damage(*selected,inputs,level,now);if(!proposed)return false;
        for(const auto& result:*proposed)if(const auto p=players_.find(result.owner.actor);p!=players_.end())
            if(!prepare_player_reactive_seed_locked(p->second,result.hit,now))return false;
        // Selected actors may have settled end reactions;verify the conditional
        // primary registry still describes the same completion-time state.
        player=players_.find(target.actor);npc=npc_combat_.find(target.actor);
        if(player!=players_.end()){if(player->second.effect_scope!=target||!player->second.effect_registry||player->second.precise_health<=0)return false;registry=*player->second.effect_registry;}
        else{if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;registry=npc->second.hex_registry_snapshot(npc->second.lifetime(),now);}
        if(npc!=npc_combat_.end()&&(npc->second.hex_scope()!=target||!npc->second.actor(npc->second.lifetime())->health||npc->second.actor(npc->second.lifetime())->health->current<=0))return false;
        caster=players_.find(source.actor);if(caster==players_.end()||caster->second.effect_scope!=source||caster->second.precise_health<=0)return false;
        if(!registry||skills::effects::water_hex(*registry,target,now)!=selected->expanded)return false;
        for(const auto scope:selected->recipients){
            caster=players_.find(source.actor);if(caster==players_.end()||caster->second.effect_scope!=source)break;
            const combat::Damage damage{combat::Kind::cold,double(scope==target?plan.damage:plan.secondary_damage),level,903};
            if(auto p=players_.find(scope.actor);p!=players_.end()){
                auto& victim=p->second;if(victim.effect_scope!=scope||!victim.resources||victim.precise_health<=0)continue;
                auto actor=incoming_spell_actor_locked(victim,{scope.actor,scope.generation,combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true,{},{},victim.reviewed_cold_armor},now);
                if(actor.cold_armor)actor.cold_armor=victim.conditions.armor(*actor.cold_armor,now);
                const auto hit=combat::apply(actor,damage,true);if(hit.reason!=combat::Reason::accepted)throw std::logic_error("preflighted Icy Prism player failed");
                if(!commit_player_reactive_hit_locked(victim,hit,source.actor,now))throw std::logic_error("preflighted Icy Prism player conversion failed");
                if(victim.passive_recovery)victim.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            }else{
                const auto live=npc_combat_.find(scope.actor);const auto body=npcs_.find(scope.actor);
                if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=scope)continue;
                const auto* actor=live->second.actor(live->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)continue;
                if(!commit_npc_reversal_or_damage_locked(caster->second,body->second,live->second,damage,now))throw std::logic_error("preflighted Icy Prism NPC failed");
            }
        }
        caster=players_.find(source.actor);if(caster!=players_.end()&&caster->second.effect_scope==source&&caster->second.passive_recovery)caster->second.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
        player=players_.find(target.actor);npc=npc_combat_.find(target.actor);
        if((player!=players_.end()&&player->second.effect_scope==target)||(npc!=npc_combat_.end()&&npc->second.hex_scope()==target))publish_target_body_locked(target.actor,903,source.actor,visuals);
        return true;
    }
private:

