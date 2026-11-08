// Fixed-location cold field; shared timeline and area damage transaction.
private:
    std::uint64_t snow_storm_refused_=0;
    bool dispatch_snow_storm_pulse_locked(const skills::effects::DamageFieldPulse& pulse){
        const auto source=players_.find(pulse.caster.actor);
        if(source==players_.end()||source->second.effect_scope!=pulse.caster||!source->second.team_token)return false;
        const combat::Damage damage{combat::Kind::cold,double(pulse.plan.damage),pulse.level,2222};
        const AreaDamageCentre centre{{pulse.x,pulse.y},pulse.plane};
        const auto admitted=admit_area_damage_pulse_locked(pulse.caster,damage,centre,pulse.plan.radius,
            pulse.at,source->second.team_token,std::nullopt,{},{});
        if(!admitted){++snow_storm_refused_;std::cerr<<"Snow Storm pulse refused before mutation: cohort/protection admission\n";return true;}
        commit_area_damage_pulse_locked(pulse.caster,damage,pulse.at,*admitted,std::nullopt);
        return true;
    }
public:
    std::uint64_t snow_storm_refused_pulses()const{std::lock_guard lock(mutex_);return snow_storm_refused_;}
    bool complete_snow_storm(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::SnowStormPlan& plan,unsigned level,std::uint64_t event,
            std::uint64_t now,std::vector<Packet>* visuals=nullptr){
        if(visuals)visuals->clear();if(!event)return false;
        return stage_snow_storm(source,target,plan,level,now,visuals,event);
    }
    bool stage_snow_storm(skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::SnowStormPlan& plan,unsigned level,std::uint64_t now,
            std::vector<Packet>* visuals=nullptr,std::optional<std::uint64_t> event={}){
        if(visuals)visuals->clear();const auto canonical=skills::effects::snow_storm(plan.deldrimor_rank);
        if(!canonical||*canonical!=plan||!level||level>255||source==target||
           now>UINT64_MAX-plan.pulses*plan.interval)return false;
        const auto field=skills::effects::damage_field(2222,plan.deldrimor_rank);if(!field)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        if(event&&!complete_ether_renewal_locked(source,2222,*event,now,true,false))return false;
        std::optional<AreaDamageCentre> centre;
        const auto prepare=[&]()->bool{
            centre.reset();auto caster=players_.find(source.actor);
            if(caster==players_.end()||caster->second.effect_scope!=source||!caster->second.resources||
               !caster->second.team_token||caster->second.precise_health<=0||caster->second.knocked_down_until>now||
               now<caster->second.health_updated||!advance_health_locked(caster->second,now)||caster->second.precise_health<=0)return false;
            if(auto p=players_.find(target.actor);p!=players_.end()){
                if(p->second.effect_scope!=target||!p->second.resources||!p->second.team_token||
                   p->second.team_token==caster->second.team_token||now<p->second.health_updated||
                   !advance_health_locked(p->second,now)||p->second.precise_health<=0)return false;
                const auto point=healing_position_locked(p->second,now);if(!point)return false;
                centre=AreaDamageCentre{*point,p->second.surface.plane};
            }else{
                auto n=npc_combat_.find(target.actor);
                if(n==npc_combat_.end()||n->second.hex_scope()!=target||
                   n->second.health_clock(n->second.lifetime()).value_or(now)>now)return false;
                advance_npc_conditions_locked(target.actor,n->second,now);refresh_npc_movement_locked(target.actor,now);
                n=npc_combat_.find(target.actor);const auto body=npcs_.find(target.actor);
                if(n==npc_combat_.end()||body==npcs_.end()||n->second.hex_scope()!=target||
                   !n->second.hostile_to_players(n->second.lifetime()))return false;
                const auto actor=n->second.actor(n->second.lifetime());
                if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)return false;
                centre=AreaDamageCentre{{body->second.x,body->second.y},body->second.plane};
            }
            caster=players_.find(source.actor);
            if(caster==players_.end()||caster->second.effect_scope!=source||caster->second.precise_health<=0||
               caster->second.knocked_down_until>now||!centre||centre->plane!=caster->second.surface.plane||centre->plane>65535)return false;
            const auto origin=healing_position_locked(caster->second,now);
            if(!origin||std::hypot(origin->x-centre->point.x,origin->z-centre->point.z)>plan.range)return false;
            if(navigation_){const auto trace=layered_->trace(caster->second.surface,*origin,centre->point);
                if(!trace||trace->plane!=centre->plane)return false;}
            return true;
        };
        if(!prepare())return false;
        auto checked=damage_fields_;
        if(!checked.add(source,level,centre->point.x,centre->point.z,static_cast<std::uint16_t>(centre->plane),*field,now))return false;
        if(event&&!complete_ether_renewal_locked(source,2222,*event,now,true))return false;
        if(event&&!prepare())return true; // Consumed completion; reward killed/retired an actor.
        if(!damage_fields_.add(source,level,centre->point.x,centre->point.z,static_cast<std::uint16_t>(centre->plane),*field,now))return false;
        const auto packets=skills::particles::snow_storm_ground_2022(source.actor,centre->point.x,centre->point.z,static_cast<std::uint16_t>(centre->plane));
        if(visuals)*visuals=packets;else for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);
        return true;
    }
