public:
    bool earthquake(skills::effects::Scope scope,skills::effects::Scope selected,const skills::effects::EarthquakePlan& plan,
            unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr,std::optional<std::uint64_t> completion_event={},
            std::function<void()>* publication=nullptr){
        using namespace skills::effects;using K=combat::npcs::control::Knockdown;
        if(visuals)visuals->clear();if(publication)*publication={};if(publication&&!visuals)return false;
        const auto canonical=skills::effects::earthquake(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255||now>UINT64_MAX-plan.knockdown_ms||(completion_event&&!*completion_event))return false;
        std::lock_guard lock(mutex_);DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        const auto execute=[&]()->bool{
        settle_damage_fields_locked(now);
        const auto caster_at=[&]()->Player*{const auto p=players_.find(scope.actor);return p!=players_.end()&&p->second.effect_scope==scope?&p->second:nullptr;};
        auto* caster=caster_at();
        if(!caster||!caster->team_token||!advance_resource_event_locked(*caster,now)||caster->precise_health<=0||caster->knocked_down_until>now)return false;
        const auto team=caster->team_token;const auto origin=healing_position_locked(*caster,now);if(!origin)return false;
        navigation::Point center{};std::uint16_t plane=0;
        if(const auto victim=players_.find(selected.actor);victim!=players_.end()){
            auto& p=victim->second;
            if(p.effect_scope!=selected||!p.resources||!advance_health_locked(p,now)||p.precise_health<=0||!p.team_token||p.team_token==team)return false;
            const auto point=healing_position_locked(p,now);if(!point)return false;center=*point;plane=p.surface.plane;
        }else{
            auto n=npc_combat_.find(selected.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=selected||!n->second.hostile_to_players(n->second.lifetime()))return false;
            advance_npc_conditions_locked(selected.actor,n->second,now);refresh_npc_movement_locked(selected.actor,now);
            n=npc_combat_.find(selected.actor);const auto placed=npcs_.find(selected.actor);
            if(n==npc_combat_.end()||placed==npcs_.end()||n->second.hex_scope()!=selected)return false;
            const auto actor=n->second.actor(n->second.lifetime());if(!actor||!actor->health||actor->health->current<=0||!actor->damageable)return false;
            center={placed->second.x,placed->second.y};plane=placed->second.plane;
        }
        if(plane!=caster->surface.plane||std::hypot(center.x-origin->x,center.z-origin->z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->surface,*origin,center);if(!line||line->plane!=plane)return false;}
        std::vector<Scope> recipients,npc_recipients;
        for(auto& [id,p]:players_){
            if(!p.resources||!p.team_token||p.team_token==team||p.precise_health<=0||p.surface.plane!=plane)continue;
            const auto point=healing_position_locked(p,now);if(!point)return false;
            if(std::hypot(point->x-center.x,point->z-center.z)>plan.radius)continue;
            if(now<p.health_updated||!advance_health_locked(p,now))return false;if(p.precise_health>0)recipients.push_back(p.effect_scope);
        }
        std::vector<Scope> ids;for(const auto& [id,n]:npc_combat_)ids.push_back(n.hex_scope());
        for(const auto owner:ids){
            auto n=npc_combat_.find(owner.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=owner||!n->second.hostile_to_players(n->second.lifetime()))continue;
            refresh_npc_movement_locked(owner.actor,now);const auto placed=npcs_.find(owner.actor);
            if(placed==npcs_.end()||placed->second.plane!=plane||std::hypot(placed->second.x-center.x,placed->second.y-center.z)>plan.radius)continue;
            if(now<n->second.health_clock(n->second.lifetime()).value_or(now))return false;
            advance_npc_conditions_locked(owner.actor,n->second,now);n=npc_combat_.find(owner.actor);
            if(n==npc_combat_.end()||n->second.hex_scope()!=owner)continue;
            const auto actor=n->second.actor(n->second.lifetime());if(actor&&actor->health&&actor->health->current>0&&actor->damageable)npc_recipients.push_back(owner);
        }
        const combat::Damage damage{combat::Kind::earth,plan.damage,level,plan.skill};
        const auto allows=[&](const Player& p,const Registry& registry){return p.knocked_down_until<=now&&
            !dont_trip_active(registry,p.effect_scope,now)&&!active_dolyak(registry,p.effect_scope,now)&&
            !balanced_stance_active(registry,p.effect_scope,now)&&!unstoppable_active(registry,p.effect_scope,now);};
        std::map<Scope,HealingSeedReaction> facts;bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            for(const auto owner:recipients){
                const auto found=players_.find(owner.actor);if(found==players_.end()||found->second.effect_scope!=owner)continue;
                auto& p=found->second;auto* state=admission_player_locked(batch,p,now);if(!state)return false;if(state->health.current<=0)continue;
                auto actor=incoming_spell_actor_locked(p,{p.id,owner.generation,combat::Health{state->health.current,state->health.capacity.maximum()},p.reviewed_earth_armor,p.effects,true},now);
                const combat::DamageProjection projection=[&](combat::Actor& current,const combat::Damage& hit){return project_batch_player_damage_locked(batch,p,current,hit,now);};
                const auto result=earthquake_target(plan,actor,p.conditions,level,now,allows(p,state->registry),projection);
                if(!result){valid=false;break;}if(result->knockdown&&p.control_epoch==UINT64_MAX)return false;
            }
            if(batch.missing_seed){const auto owner=*batch.missing_seed;const auto p=players_.find(owner.actor);
                if(p==players_.end()||p->second.effect_scope!=owner)return false;
                const auto fact=prepare_healing_seed_direct_locked(p->second,1.,now);if(!fact)return false;facts[owner]=*fact;continue;}
            if(!valid)return false;
            for(const auto owner:npc_recipients){const auto n=npc_combat_.find(owner.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=owner)continue;
                auto preview=batch.npcs.contains(owner)?batch.npcs.at(owner):n->second;
                if(!preflight_npc_reversal_or_damage_locked(preview,damage,now))return false;
                const auto life=preview.lifetime();if(preview.actor(life)->health->current>0){
                    const auto control=preview.knock_down(life,now,plan.knockdown_ms);if(control==K::invalid||control==K::unknown)return false;
                }
            }
            admitted=true;break;
        }
        if(!admitted)return false;
        if(completion_event&&!complete_ether_renewal_locked(scope,plan.skill,*completion_event,now,true))return false;
        caster=caster_at();if(!caster||caster->precise_health<=0)return true;
        auto* deferred=publication?nullptr:visuals;
        const auto body=[&](Scope owner){Packet packet{159,0};u32(packet,20);u32(packet,owner.actor);u32(packet,scope.actor);u32(packet,304);
            if(deferred)deferred->push_back(packet);else for(auto& [id,peer]:players_)peer.outbox->enqueue({packet});};
        for(const auto owner:recipients){
            auto found=players_.find(owner.actor);if(found==players_.end()||found->second.effect_scope!=owner||!found->second.resources||found->second.precise_health<=0)continue;
            auto& p=found->second;if(publication)body(owner);
            auto actor=incoming_spell_actor_locked(p,{p.id,owner.generation,combat::Health{p.precise_health,p.resources->max_health},p.reviewed_earth_armor,p.effects,true},now,true);
            const auto hit=combat::apply(actor,damage,true);
            if(!commit_player_reactive_hit_locked(p,hit,scope.actor,now,false,nullptr,false,true,nullptr,scope))throw std::logic_error("admitted Earthquake player damage failed");
            if(!publication)body(owner);
            found=players_.find(owner.actor);if(found==players_.end()||found->second.effect_scope!=owner||found->second.precise_health<=0)continue;
            auto& current=found->second;if(current.passive_recovery)current.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            if(current.effect_registry&&allows(current,*current.effect_registry)&&!knockdown_player_locked(current,plan.knockdown_ms,now,deferred))throw std::logic_error("admitted Earthquake control failed");
        }
        for(const auto owner:npc_recipients){
            auto n=npc_combat_.find(owner.actor);auto placed=npcs_.find(owner.actor);
            if(n==npc_combat_.end()||placed==npcs_.end()||n->second.hex_scope()!=owner)continue;
            const auto actor=n->second.actor(n->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)continue;
            if(publication)body(owner);caster=caster_at();
            if(!commit_npc_reversal_or_damage_locked(caster,placed->second,n->second,damage,now,nullptr,nullptr,nullptr,false,scope))throw std::logic_error("admitted Earthquake NPC damage failed");
            if(!publication)body(owner);n=npc_combat_.find(owner.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=owner)continue;
            const auto recipient=n->second.actor(n->second.lifetime());if(!recipient||!recipient->health||recipient->health->current<=0)continue;
            const auto knock=knockdown_npc_locked(owner,plan.knockdown_ms,now,deferred);
            if(knock==K::invalid||knock==K::unknown)throw std::logic_error("admitted Earthquake NPC control failed");
        }
        dispatch_fevered_boundary_locked(now);return true;
        };
        const bool accepted=execute();auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();return accepted;
    }
