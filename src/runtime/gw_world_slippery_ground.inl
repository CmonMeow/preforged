private:
    std::mt19937 slippery_ground_rng_{std::random_device{}()};
public:
    bool slippery_ground(skills::effects::Scope scope,skills::effects::Scope target,
            const skills::effects::SlipperyGroundPlan& plan,std::uint64_t now,
            std::vector<Packet>* deferred=nullptr){
        if(skills::effects::slippery_ground(plan.rank)!=plan||now>UINT64_MAX-plan.knockdown_ms)return false;
        std::lock_guard lock(mutex_);
        settle_damage_fields_locked(now);
        const auto source=players_.find(scope.actor);
        if(source==players_.end()||source->second.effect_scope!=scope||!source->second.resources||
           !source->second.team_token||!advance_health_locked(source->second,now)||
           source->second.precise_health<=0||source->second.knocked_down_until>now)return false;
        auto& caster=source->second;const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        using A=skills::effects::SlipperyGroundActor;
        const A owner{scope,origin->x,origin->z,static_cast<unsigned>(caster.surface.plane),true,false,false,false};
        std::vector<A> actors;
        for(auto& [id,p]:players_){
            if(!p.resources||!p.team_token||p.team_token==caster.team_token||p.surface.plane!=caster.surface.plane)continue;
            if(now<p.health_updated||!advance_health_locked(p,now))return false;
            if(p.precise_health<=0)continue;
            const auto point=healing_position_locked(p,now),future=healing_position_locked(p,now+1);
            if(!point||!future)return false;
            const bool moving=std::hypot(future->x-point->x,future->z-point->z)>.000001;
            actors.push_back({p.effect_scope,point->x,point->z,static_cast<unsigned>(p.surface.plane),true,true,
                p.conditions.has(combat::conditions::Kind::blind,now),moving});
        }
        for(auto& [id,live]:npc_combat_){
            const auto body=npcs_.find(id);const auto life=live.lifetime();
            if(body==npcs_.end()||body->second.plane!=caster.surface.plane||!live.hostile_to_players(life))continue;
            if(now<live.health_clock(life).value_or(now))return false;
            advance_npc_conditions_locked(id,live,now);refresh_npc_movement_locked(id,now);
            const auto* actor=live.actor(life);if(!actor||!actor->health||actor->health->current<=0)continue;
            const auto conditions=live.condition_snapshot(life);if(!conditions)return false;
            actors.push_back({live.hex_scope(),body->second.x,body->second.y,body->second.plane,true,true,
                conditions->has(combat::conditions::Kind::blind,now),npc_moving_at_locked(id,now)});
        }
        const auto center=std::find_if(actors.begin(),actors.end(),[&](const A& a){return a.scope==target;});
        if(center==actors.end())return false;
        if(navigation_){const auto line=layered_->trace(caster.surface,*origin,{float(center->x),float(center->z)});
            if(!line||line->plane!=center->plane)return false;}
        const auto draw=plan.failure_percent?std::optional(std::uniform_int_distribution<unsigned>(0,99)(slippery_ground_rng_)):std::nullopt;
        const auto proposal=skills::effects::prepare_slippery_ground(plan,owner,target,actors,draw);if(!proposal)return false;
        if(proposal->failed)return true;
        using K=combat::npcs::control::Knockdown;
        std::vector<skills::effects::Scope> accepted;
        for(const auto victim:proposal->knockdowns){
            if(const auto p=players_.find(victim.actor);p!=players_.end()){
                if(p->second.effect_scope!=victim)return false;
                if(p->second.knocked_down_until>now)continue;
                if(p->second.control_epoch==UINT64_MAX)return false;
                accepted.push_back(victim);
            }else{
                auto probe=npc_combat_.at(victim.actor);const auto result=probe.knock_down(probe.lifetime(),now,plan.knockdown_ms);
                if(result==K::invalid||result==K::unknown)return false;
                if(result==K::accepted)accepted.push_back(victim);
            }
        }
        std::vector<Packet> packets;
        for(const auto victim:accepted){
            if(const auto p=players_.find(victim.actor);p!=players_.end()){
                if(!knockdown_player_locked(p->second,plan.knockdown_ms,now,&packets))throw std::logic_error("preflighted Slippery Ground player control failed");
            }else if(knockdown_npc_locked(victim,plan.knockdown_ms,now,&packets)!=K::accepted)
                throw std::logic_error("preflighted Slippery Ground NPC control failed");
        }
        const auto particle=skills::particles::target_body_2022(target.actor,2191);if(particle)packets.push_back(*particle);
        if(deferred)deferred->insert(deferred->end(),packets.begin(),packets.end());
        else for(auto& [id,peer]:players_)peer.outbox->enqueue(packets);
        return true;
    }
