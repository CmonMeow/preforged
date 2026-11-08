public:
    bool elemental(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::ElementalPlan& plan,unsigned level,std::uint64_t now,std::vector<Packet>* completion_visuals=nullptr,std::optional<skills::sacrifice::Loss>* paid_sacrifice=nullptr,skills::effects::ElementalOutcome* outcome=nullptr,std::optional<std::uint64_t> completion_event={},std::function<void()>* publication=nullptr){
        if(publication)*publication={};if(publication&&!completion_visuals)return false;
        if(outcome)*outcome={};
        if(completion_visuals)completion_visuals->clear();
        if(paid_sacrifice)paid_sacrifice->reset();
        if(!skills::effects::valid_elemental(plan)||!level||level>255||(plan.skill==1068&&!outcome)||(completion_event&&!*completion_event))return false;
        if(completion_event){const auto* row=skills::find_skill(plan.skill);if(!row||!skills::effects::is_spell(*row))return false;}
        if(plan.skill==170||plan.skill==1086){const auto q=skills::effects::earthquake(plan.skill,plan.rank);return q&&earthquake(scope,target,*q,level,now,completion_visuals,completion_event,publication);}
        std::lock_guard lock(mutex_);
        DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        auto* feedback_visuals=publication?nullptr:completion_visuals;
        const auto execute=[&]()->bool{
        std::optional<skills::effects::SpiritProximity> spirit_census;
        if(plan.skill==919||plan.skill==1227){
            spirit_census=settled_spirit_light_proximity_locked(scope,now);
            if(*spirit_census==skills::effects::SpiritProximity::unknown)return false;
        }
        if(plan.skill==163){
            const bool accepted=target==scope&&whirlwind_locked(scope,plan,level,now,feedback_visuals,completion_event,publication!=nullptr);
            dispatch_fevered_boundary_locked(now);return accepted;
        }
        auto perform_condition_batch=[&]()->bool{
            const auto a=players_.find(scope.actor),b=players_.find(target.actor);
            if(a!=players_.end()&&b==players_.end()&&!plan.projectile&&!plan.caster_center){
                if(a->second.effect_scope!=scope||!a->second.resources||a->second.precise_health<=0||a->second.knocked_down_until>now||
                   now<a->second.health_updated||now<a->second.energy_updated||!advance_health_locked(a->second,now)||a->second.precise_health<=0)return false;
                const auto initial=npc_combat_.find(target.actor);
                if(initial==npc_combat_.end()||initial->second.hex_scope()!=target)return false;
                advance_npc_conditions_locked(target.actor,initial->second,now);refresh_npc_movement_locked(target.actor,now);
                const auto body=npcs_.find(target.actor);const auto found=npc_combat_.find(target.actor);
                if(body==npcs_.end()||found==npc_combat_.end()||found->second.hex_scope()!=target||a->second.surface.plane!=body->second.plane)return false;
                const auto origin=healing_position_locked(a->second,now);
                if(!origin||std::hypot(origin->x-body->second.x,origin->z-body->second.y)>plan.range)return false;
                if(navigation_){const auto line=layered_->trace(a->second.surface,*origin,{body->second.x,body->second.y});if(!line||line->plane!=body->second.plane)return false;}
                const auto life=found->second.lifetime();const auto* actor=found->second.actor(life);
                if(!found->second.hostile_to_players(life)||!actor||!actor->damageable||!actor->health||(actor->health->current<=0&&plan.skill!=189))return false;
                return elemental_impact_locked(a->second,target.actor,plan,level,body->second.x,body->second.y,body->second.plane,now,feedback_visuals,paid_sacrifice,outcome,completion_event,spirit_census);
            }
            if(a!=players_.end()&&b==players_.end()&&(plan.skill==194||plan.skill==186)){
                const auto n=npcs_.find(target.actor);const auto c=npc_combat_.find(target.actor);
                if(n==npcs_.end()||c==npc_combat_.end()||a->second.effect_scope!=scope||
                   !a->second.resources||a->second.precise_health<=0||a->second.knocked_down_until>now||
                   a->second.surface.plane!=n->second.plane||elemental_projectiles_.size()>=4096)return false;
                const auto& live=c->second;const auto& life=live.lifetime();const auto* actor=live.actor(life);
                if(target!=skills::effects::Scope{instance_scope_,life.actor,life.actor_generation}||
                   !live.hostile_to_players(life)||!actor->damageable||!actor->health||actor->health->current<=0||!actor->fire_armor)return false;
                const auto origin=healing_position_locked(a->second,now);
                if(!origin||std::hypot(origin->x-n->second.x,origin->z-n->second.y)>plan.range)return false;
                if(navigation_){const auto line=layered_->trace(a->second.surface,*origin,{n->second.x,n->second.y});
                    if(!line||line->plane!=n->second.plane)return false;}
                const auto aim=projectiles::intercept(projectile_policy_,origin->x,origin->z,n->second.x,n->second.y,0,0);
                if(!aim||now>UINT64_MAX-aim->flight_ms||!advance_health_locked(a->second,now)||a->second.precise_health<=0)return false;
                if(completion_event&&!complete_ether_renewal_locked(scope,plan.skill,*completion_event,now,true))return false;
                if(a->second.precise_health<=0)return true;
                elemental_projectiles_.push_back({scope,target,plan,level,aim->x,aim->y,n->second.plane,now+aim->flight_ms});return true;
            }
            if(a==players_.end()||b==players_.end()||a->second.effect_scope!=scope||b->second.effect_scope!=target||!a->second.team_token||!a->second.resources||!b->second.resources||a->second.precise_health<=0||(b->second.precise_health<=0&&plan.skill!=189)||a->second.knocked_down_until>now||a->second.surface.plane!=b->second.surface.plane||now<a->second.health_updated||now<b->second.health_updated)return false;
            if(plan.caster_center?target!=scope:(!b->second.team_token||a->second.team_token==b->second.team_token))return false;
            const auto origin=healing_position_locked(a->second,now),destination=healing_position_locked(b->second,now);
            if(!origin||!destination||std::hypot(origin->x-destination->x,origin->z-destination->z)>plan.range)return false;
            if(!plan.projectile&&!plan.caster_center&&navigation_){const auto line=layered_->trace(a->second.surface,*origin,*destination);
                if(!line||line->plane!=b->second.surface.plane)return false;}
            if(plan.projectile){
                if(elemental_projectiles_.size()>=4096)return false;const auto future=now<=UINT64_MAX-10?healing_position_locked(b->second,now+10):std::nullopt;
                const double vx=future?(future->x-destination->x)*100:0,vy=future?(future->z-destination->z)*100:0;
                const auto aim=projectiles::intercept(projectile_policy_,origin->x,origin->z,destination->x,destination->z,vx,vy);if(!aim||now>UINT64_MAX-aim->flight_ms)return false;
                if(navigation_){const auto line=layered_->trace(a->second.surface,*origin,*destination);if(!line||line->plane!=b->second.surface.plane)return false;}
                if(!advance_health_locked(a->second,now)||!advance_health_locked(b->second,now)||a->second.precise_health<=0||b->second.precise_health<=0)return false;
                if(completion_event&&!complete_ether_renewal_locked(scope,plan.skill,*completion_event,now,true))return false;
                if(a->second.precise_health<=0)return true;
                elemental_projectiles_.push_back({scope,target,plan,level,aim->x,aim->y,static_cast<std::uint16_t>(b->second.surface.plane),now+aim->flight_ms});return true;
            }
            if(!advance_health_locked(a->second,now)||a->second.precise_health<=0)return false;
            return elemental_impact_locked(a->second,target.actor,plan,level,destination->x,destination->z,static_cast<std::uint16_t>(b->second.surface.plane),now,feedback_visuals,paid_sacrifice,outcome,completion_event,spirit_census);
        };
        auto accepted=perform_condition_batch();
        dispatch_fevered_boundary_locked(now);
        return accepted;
        };
        const bool accepted=execute();auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();
        return accepted;
    }
