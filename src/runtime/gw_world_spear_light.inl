// Included within Instance. World owns collision, clocks and damage reactions.
private:
    std::optional<skills::effects::SpearLightTarget> spear_light_target_locked(skills::effects::Scope scope,const Player& source,std::uint64_t now){
        skills::effects::SpearLightTarget target;
        if(const auto found=players_.find(scope.actor);found!=players_.end()){
            auto& p=found->second;if(p.effect_scope!=scope||!p.resources||!resurrection_clock_valid(p,now)||
                !p.reviewed_holy_traits||!p.reviewed_holy_bonus||!advance_health_locked(p,now))return {};
            const auto position=healing_position_locked(p,now);if(!position)return {};
            static_cast<skills::effects::SpearLightActor&>(target)={scope,position->x,position->z,static_cast<std::uint16_t>(p.surface.plane),p.precise_health>0};
            target.hostile=source.team_token&&p.team_token&&source.team_token!=p.team_token;
            target.traits=*p.reviewed_holy_traits;target.attacking=p.ordinary_animation||now<p.melee_skill_until;return target;
        }
        const auto found=npc_combat_.find(scope.actor);const auto npc=npcs_.find(scope.actor);
        if(found==npc_combat_.end()||npc==npcs_.end()||found->second.hex_scope()!=scope)return {};
        auto& live=found->second;const auto life=live.lifetime();
        if(live.health_clock(life).value_or(0)>now)return {};
        advance_npc_conditions_locked(scope.actor,live,now);refresh_npc_movement_locked(scope.actor,now);
        const auto* actor=live.actor(life);const auto traits=live.holy_traits(life);
        if(!actor||!actor->health||!actor->damageable||!traits)return {};
        static_cast<skills::effects::SpearLightActor&>(target)={scope,npc->second.x,npc->second.y,npc->second.plane,actor->health->current>0};
        target.hostile=live.hostile_to_players(life);target.traits=*traits;target.attacking=live.attacking(life,now);return target;
    }
    void resolve_spear_light_locked(std::uint64_t now){
        if(!timed_effect_boundary_){settle_damage_fields_locked(now);return;}
        std::vector<PendingSpearLight> due;
        for(auto it=spear_light_projectiles_.begin();it!=spear_light_projectiles_.end();){
            if(it->visual_pending||it->flight.arrives()>now){++it;continue;}
            due.push_back(std::move(*it));it=spear_light_projectiles_.erase(it);
        }
        for(auto& shot:due){
            const auto found=players_.find(shot.caster.actor);
            if(found==players_.end()||found->second.effect_scope!=shot.caster||!found->second.resources||
                !resurrection_clock_valid(found->second,now)||!advance_health_locked(found->second,now))continue;
            auto& source=found->second;const auto origin=healing_position_locked(source,now);
            const auto target=spear_light_target_locked(shot.target,source,now);if(!origin||!target)continue;
            const skills::effects::SpearLightActor owner{shot.caster,origin->x,origin->z,static_cast<std::uint16_t>(source.surface.plane),source.precise_health>0};
            const auto impact=shot.flight.impact(owner,*target,shot.level,now);if(!impact)continue;
            if(const auto player=players_.find(shot.target.actor);player!=players_.end()){
                auto& victim=player->second;auto hits=impact->hits;
                for(auto& hit:hits)hit.amount+=*victim.reviewed_holy_bonus;
                if(!admit_player_damage_sequence_locked(victim,hits,now,[&](const Player& live,combat::Health health){
                    return incoming_spell_actor_locked(live,{live.id,shot.target.generation,health,{},live.effects,true},now,true);
                }))continue;
                for(const auto& damage:hits){
                    const auto current_source=players_.find(shot.caster.actor),current_target=players_.find(shot.target.actor);
                    if(current_source==players_.end()||current_source->second.effect_scope!=shot.caster||
                       current_target==players_.end()||current_target->second.effect_scope!=shot.target)break;
                    auto& recipient=current_target->second;
                    if(current_source->second.precise_health<=0||!recipient.resources||recipient.precise_health<=0)break;
                    auto actor=incoming_spell_actor_locked(recipient,{recipient.id,shot.target.generation,combat::Health{recipient.precise_health,recipient.resources->max_health},{},recipient.effects,true},now,true);
                    const auto hit=combat::apply(actor,damage,true);if(hit.reason!=combat::Reason::accepted)break;
                    if(!commit_player_reactive_hit_locked(recipient,hit,shot.caster.actor,now,false,nullptr,false,true,nullptr,shot.caster))break;
                    if(recipient.passive_recovery)recipient.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
                    if(recipient.precise_health<=0)break;
                }
            }else{
                const auto live=npc_combat_.find(shot.target.actor);const auto npc=npcs_.find(shot.target.actor);
                if(live==npc_combat_.end()||npc==npcs_.end())continue;
                auto preview=live->second;const auto life=preview.lifetime();bool ready=true;
                for(const auto& damage:impact->hits){if(!preflight_npc_reversal_or_damage_locked(preview,damage,now)){ready=false;break;}if(preview.actor(life)->health->current<=0)break;}
                if(!ready)continue;
                for(const auto& damage:impact->hits){if(source.precise_health<=0||live->second.actor(life)->health->current<=0)break;if(!commit_npc_reversal_or_damage_locked(source,npc->second,live->second,damage,now))break;}
            }
        }
    }
public:
    bool spear_light(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::SpearLight& plan,
                     unsigned level,std::uint64_t now,std::vector<Packet>* visuals,std::function<void()>* publication){
        if(visuals)visuals->clear();if(publication)*publication={};
        const auto canonical=skills::effects::spear_light(plan.rank);
        if(!visuals||!publication||!canonical||*canonical!=plan||!level||level>255)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto found=players_.find(scope.actor);
        if(found==players_.end()||found->second.effect_scope!=scope||!found->second.resources||spear_light_projectiles_.size()>=4096||
            !resurrection_clock_valid(found->second,now)||!advance_health_locked(found->second,now)||found->second.precise_health<=0||found->second.knocked_down_until>now)return false;
        auto& source=found->second;const auto origin=healing_position_locked(source,now);const auto victim=spear_light_target_locked(target,source,now);
        if(!origin||!victim||!victim->alive||!victim->hostile||!victim->traits.light_sensitive||!victim->attacking)return false;
        if(navigation_){const auto trace=layered_->trace(source.surface,*origin,{victim->x,victim->y});if(!trace||trace->plane!=victim->plane)return false;}
        double vx=0,vy=0;
        if(const auto player=players_.find(target.actor);player!=players_.end()&&now<=UINT64_MAX-10){
            const auto future=healing_position_locked(player->second,now+10);if(!future)return false;
            vx=(future->x-victim->x)*100;vy=(future->z-victim->y)*100;
        }
        skills::effects::SpearLightFlight flight;
        if(!flight.launch(plan,{scope,origin->x,origin->z,static_cast<std::uint16_t>(source.surface.plane),true},*victim,projectile_policy_,vx,vy,now))return false;
        std::vector<std::uint32_t> keys;for(const auto& shot:spear_light_projectiles_)keys.push_back(shot.key);
        for(const auto& shot:ordinary_projectiles_)keys.push_back(shot.visual_sync);
        if(keys.size()>4096)return false;const auto reserved=projectile_sync_tokens_.reserve(1,keys);if(!reserved)return false;const auto key=reserved->front();
        visuals->push_back(skills::particles::spear_light_launch_2022(scope.actor,flight.aim().x,flight.aim().y,victim->plane,float(flight.aim().flight_ms)/1000.f,key));
        spear_light_projectiles_.push_back({scope,target,std::move(flight),level,key,true});
        *publication=[this,scope,key]{std::lock_guard guard(mutex_);for(auto& shot:spear_light_projectiles_)if(shot.caster==scope&&shot.key==key&&shot.visual_seen){shot.visual_pending=false;break;}};
        return true;
    }
