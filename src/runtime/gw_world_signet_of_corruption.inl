    bool signet_of_corruption(skills::effects::Scope source_scope,skills::effects::Scope target,
            const skills::effects::SignetOfCorruptionPlan& plan,unsigned level,std::uint64_t now){
        const auto canonical=skills::effects::signet_of_corruption(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255)return false;
        std::lock_guard lock(mutex_);
        auto source=players_.find(source_scope.actor);
        if(source==players_.end()||source->second.effect_scope!=source_scope||!source->second.resources||
           !advance_resource_event_locked(source->second,now)||source->second.precise_health<=0||
           source->second.knocked_down_until>now)return false;
        auto& caster=source->second;
        const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        float cx=0,cy=0;std::uint16_t plane=0;
        if(auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target||!p->second.resources||p->second.precise_health<=0||
               !caster.team_token||!p->second.team_token||caster.team_token==p->second.team_token||
               !advance_health_locked(p->second,now))return false;
            const auto at=healing_position_locked(p->second,now);if(!at)return false;
            cx=at->x;cy=at->z;plane=p->second.surface.plane;
        }else{
            auto n=npc_combat_.find(target.actor);auto body=npcs_.find(target.actor);
            if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=target||
               !n->second.hostile_to_players(n->second.lifetime()))return false;
            advance_npc_conditions_locked(target.actor,n->second,now);
            refresh_npc_movement_locked(target.actor,now);
            n=npc_combat_.find(target.actor);body=npcs_.find(target.actor);
            if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=target)return false;
            const auto* actor=n->second.actor(n->second.lifetime());
            if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)return false;
            cx=body->second.x;cy=body->second.y;plane=body->second.plane;
        }
        if(plane!=caster.surface.plane||std::hypot(cx-origin->x,cy-origin->z)>plan.target_range)return false;
        std::vector<std::uint32_t> moving;
        for(const auto& [id,body]:npcs_)if(body.plane==plane&&
            std::hypot(body.x-cx,body.y-cy)<=plan.nearby_radius+100)moving.push_back(id);
        for(const auto id:moving){auto n=npc_combat_.find(id);if(n!=npc_combat_.end()){
            advance_npc_conditions_locked(id,n->second,now);refresh_npc_movement_locked(id,now);}}
        std::vector<skills::effects::CorruptionFoe> foes;
        for(auto& [id,p]:players_){
            if(!p.resources||p.precise_health<=0||p.surface.plane!=plane||!caster.team_token||
               !p.team_token||caster.team_token==p.team_token)continue;
            const auto at=healing_position_locked(p,now);if(!at)return false;
            if(std::hypot(at->x-cx,at->z-cy)>plan.nearby_radius&&id!=target.actor)continue;
            if(!advance_health_locked(p,now))return false;
            foes.push_back({id,p.effect_scope.generation,at->x,at->z,true,true,
                p.conditions.count(now)>0,p.effect_registry&&p.effect_registry->has(p.effect_scope,skills::effects::Category::hex,now)});
        }
        for(auto& [id,n]:npc_combat_){
            const auto body=npcs_.find(id);if(body==npcs_.end()||body->second.plane!=plane||
                !n.hostile_to_players(n.lifetime()))continue;
            const auto* actor=n.actor(n.lifetime());
            if(!actor||!actor->damageable||!actor->health||actor->health->current<=0||
               (std::hypot(body->second.x-cx,body->second.y-cy)>plan.nearby_radius&&id!=target.actor))continue;
            const auto conditions=n.condition_snapshot(n.lifetime());
            foes.push_back({id,n.hex_scope().generation,body->second.x,body->second.y,true,true,
                conditions&&conditions->count(now)>0,n.newest_hex(n.lifetime(),now).has_value()});
        }
        const auto found=std::find_if(foes.begin(),foes.end(),[&](const auto& f){return f.actor==target.actor&&f.generation==target.generation;});
        if(found==foes.end())return false;
        const auto area=skills::effects::signet_of_corruption_area(plan,*found,foes);
        if(!area||area->contacts.empty())return false;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(plan.damage),level,plan.skill};
        for(const auto& contact:area->contacts){
            if(auto p=players_.find(contact.actor);p!=players_.end()){
                if(p->second.effect_scope.generation!=contact.generation)return false;
                auto actor=incoming_spell_actor_locked(p->second,{p->second.id,contact.generation,
                    combat::Health{p->second.precise_health,p->second.resources->max_health},{},p->second.effects,true},now);
                const auto hit=combat::apply(actor,damage,true);if(hit.reason!=combat::Reason::accepted||!prepare_player_reactive_seed_locked(p->second,hit,now))return false;
            }else{
                auto n=npc_combat_.find(contact.actor);if(n==npc_combat_.end()||n->second.hex_scope().generation!=contact.generation)return false;
                auto preview=n->second;if(!preflight_npc_reversal_or_damage_locked(preview,damage,now))return false;
            }
        }
        for(const auto& contact:area->contacts){
            if(auto p=players_.find(contact.actor);p!=players_.end()){
                auto& victim=p->second;if(victim.effect_scope.generation!=contact.generation||victim.precise_health<=0)continue;
                auto actor=incoming_spell_actor_locked(victim,{victim.id,contact.generation,
                    combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true},now);
                const auto hit=combat::apply(actor,damage,true);
                if(hit.reason!=combat::Reason::accepted)throw std::logic_error("validated Corruption player hit failed");
                if(!commit_player_reactive_hit_locked(victim,hit,source_scope.actor,now))throw std::logic_error("validated Corruption player conversion failed");
            }else{
                auto n=npc_combat_.find(contact.actor);auto body=npcs_.find(contact.actor);
                if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope().generation!=contact.generation)continue;
                const auto* actor=n->second.actor(n->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)continue;
                if(!commit_npc_reversal_or_damage_locked(caster,body->second,n->second,damage,now))throw std::logic_error("validated Corruption NPC conversion failed");
            }
        }
        if(area->energy){gain_energy_locked(caster,area->energy,now);caster.outbox->enqueue(owner_vitals(caster));}
        publish_target_body_locked(target.actor,plan.skill,source_scope.actor);
        if(caster.passive_recovery)caster.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
        settle_arcane_returns_locked(now);return true;
    }
