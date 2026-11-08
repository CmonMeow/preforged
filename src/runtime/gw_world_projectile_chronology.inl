// Included inside Instance. Existing queues remain the delivery authorities.
private:
    std::uint64_t timed_delivery_releases_=0;
    std::size_t timed_delivery_count_locked()const{
        auto count=spear_light_projectiles_.size()+ordinary_projectiles_.size()+controlled_spirit_projectiles_.size()+projectiles_.size()+disrupting_daggers_.size()+lightning_projectiles_.size()+water_projectiles_.size()+
            elemental_projectiles_.size()+stone_projectiles_.size()+javelin_projectiles_.size()+deathly_swarms_.size();
        for(const auto&[id,ai]:npc_flare_ai_)count+=ai.shots.size();
        for(const auto&[id,ai]:npc_elemental_ai_)count+=ai.ledger.in_flight();
        for(const auto&[id,ai]:npc_ordinary_ai_)count+=ai.ledger.in_flight();
        return count;
    }
    template<class Shot> static std::vector<Shot> take_timed_projectiles_locked(std::vector<Shot>& queue,std::uint64_t at){
        std::vector<Shot> due;
        for(auto it=queue.begin();it!=queue.end();){
            if(it->arrives>at){++it;continue;}
            due.push_back(std::move(*it));it=queue.erase(it);
        }
        return due;
    }
    // Authoritative observations may advance while a tick waits for World.
    // One frontier drives scheduling, fact admission and delivery, so retained
    // arrows resume at the newer observation rather than stalling at arrival.
    // Only current scopes contribute clocks; recycled numeric actors do not.
    std::uint64_t attack_participant_observer_frontier_locked(skills::effects::Scope caster,skills::effects::Scope target,std::uint64_t at)const{
        const auto observe=[&](skills::effects::Scope scope){
            if(const auto player=players_.find(scope.actor);player!=players_.end()&&player->second.effect_scope==scope){
                const auto& p=player->second;
                at=std::max({at,p.health_updated,p.energy_updated,p.last_move,p.steering?p.steering_tick:0});
                if(p.critical_profile)at=std::max(at,p.ordinary_weapon_updated);
            }else if(const auto npc=npc_combat_.find(scope.actor);npc!=npc_combat_.end()&&npc->second.hex_scope()==scope){
                const auto life=npc->second.lifetime();
                at=std::max({at,npc->second.health_clock(life).value_or(0),npc->second.energy_clock(life).value_or(0)});
                if(const auto motion=quest_npc_motion_.find(scope.actor);motion!=quest_npc_motion_.end())at=std::max(at,motion->second.updated);
            }
        };
        observe(caster);observe(target);
        if(const auto ledger=attack_outcomes_.find(caster);ledger!=attack_outcomes_.end())at=std::max(at,ledger->second.clock());
        return at;
    }
    std::uint64_t ordinary_observer_frontier_locked(const PendingOrdinary& shot)const{
        auto at=attack_participant_observer_frontier_locked(shot.caster,shot.target,shot.arrives);
        if(shot.attack_plan&&(shot.attack_plan->skill==908||shot.attack_plan->skill==909)){
            const auto source=players_.find(shot.caster.actor);
            if(source!=players_.end()&&source->second.effect_scope==shot.caster&&source->second.precise_health>0&&source->second.cooldown_authority){
                if(const auto bar=source->second.cooldown_authority->snapshot(shot.caster))at=std::max(at,bar->published.at);
            }
        }
        return splinter_attack_observer_frontier_locked(shot.caster,shot.target,at);
    }
    std::uint64_t controlled_spirit_observer_frontier_locked(const PendingControlledSpiritProjectile& shot)const{
        auto at=shot.arrives;
        if(const auto owner=players_.find(shot.owner.actor);
           owner!=players_.end()&&owner->second.effect_scope==shot.owner)
            at=std::max({at,owner->second.health_updated,owner->second.energy_updated,
                owner->second.last_move,owner->second.steering?owner->second.steering_tick:0});
        if(const auto live=npc_combat_.find(shot.target.actor);
           live!=npc_combat_.end()&&live->second.hex_scope()==shot.target){
            at=std::max({at,live->second.health_clock(live->second.lifetime()).value_or(0),
                live->second.energy_clock(live->second.lifetime()).value_or(0)});
            if(const auto motion=quest_npc_motion_.find(shot.target.actor);motion!=quest_npc_motion_.end())
                at=std::max(at,motion->second.updated);
        }
        return at;
    }
    std::optional<std::uint64_t> next_timed_projectile_locked()const{
        std::optional<std::uint64_t> next;
        const auto visit=[&](std::uint64_t at){if(!next||at<*next)next=at;};
        for(const auto& shot:ordinary_projectiles_)if(!shot.visual_pending)visit(ordinary_observer_frontier_locked(shot));
        for(const auto& shot:controlled_spirit_projectiles_)visit(controlled_spirit_observer_frontier_locked(shot));
        for(const auto& shot:projectiles_)visit(shot.arrives);
        for(const auto& shot:disrupting_daggers_)visit(shot.arrives);
        for(const auto& shot:lightning_projectiles_)visit(shot.arrives);
        for(const auto& shot:water_projectiles_)visit(shot.flight.arrives());
        for(const auto& shot:elemental_projectiles_)visit(shot.arrives);
        for(const auto& shot:spear_light_projectiles_)if(!shot.visual_pending)visit(shot.flight.arrives());
        for(const auto& shot:stone_projectiles_)if(!shot.visual_pending&&shot.release!=UINT64_MAX)
            visit(shot.launched?shot.arrives:shot.release);
        for(const auto& shot:javelin_projectiles_)if(shot.flight.pending())visit(shot.flight.arrives());
        for(const auto& shot:deathly_swarms_)if(shot.chain.pending())visit(shot.chain.due());
        for(const auto&[id,ai]:npc_flare_ai_)for(const auto& shot:ai.shots)visit(shot.arrives);
        for(const auto&[id,ai]:npc_elemental_ai_)if(const auto next=ai.ledger.next_arrival())visit(*next);
        for(const auto&[id,ai]:npc_ordinary_ai_)for(const auto release:ai.ledger.released_ids())if(const auto shot=ai.ledger.released(release))visit(npc_ordinary_observer_frontier_locked(ai,*shot));
        return next;
    }
    bool timed_projectile_facts_ready_locked(std::uint64_t at)const{
        const auto ready=[&](skills::effects::Scope scope){
            if(const auto p=players_.find(scope.actor);p!=players_.end()&&p->second.effect_scope==scope){
                const auto& actor=p->second;
                return std::max({actor.health_updated,actor.energy_updated,actor.last_move,
                    actor.steering?actor.steering_tick:0})<=at;
            }
            if(const auto n=npc_combat_.find(scope.actor);n!=npc_combat_.end()&&n->second.hex_scope()==scope){
                if(n->second.health_clock(n->second.lifetime()).value_or(0)>at)return false;
                if(const auto motion=quest_npc_motion_.find(scope.actor);motion!=quest_npc_motion_.end()&&motion->second.updated>at)return false;
            }
            return true; // Retired scopes are terminal, not future observations.
        };
        for(const auto& shot:ordinary_projectiles_)if(!shot.visual_pending&&ordinary_observer_frontier_locked(shot)<=at){
            if(!ready(shot.caster)||!ready(shot.target))return false;
            if(const auto outcomes=attack_outcomes_.find(shot.caster);outcomes!=attack_outcomes_.end()&&outcomes->second.clock()>at)return false;
        }
        for(const auto& shot:controlled_spirit_projectiles_)
            if(controlled_spirit_observer_frontier_locked(shot)<=at&&
               (!ready(shot.owner)||!ready(shot.target)))return false;
        for(const auto& shot:projectiles_)if(shot.arrives<=at&&(!ready(shot.caster)||!ready(shot.target)))return false;
        for(const auto& shot:disrupting_daggers_)if(shot.arrives<=at&&(!ready(shot.caster)||!ready(shot.target)))return false;
        for(const auto& shot:lightning_projectiles_)if(shot.arrives<=at&&(!ready(shot.caster)||!ready(shot.target)))return false;
        for(const auto& shot:water_projectiles_)if(shot.flight.arrives()<=at&&(!ready(shot.caster)||!ready(shot.target)))return false;
        for(const auto& shot:spear_light_projectiles_)if(!shot.visual_pending&&shot.flight.arrives()<=at&&(!ready(shot.caster)||!ready(shot.target)))return false;
        for(const auto& shot:elemental_projectiles_)if(shot.arrives<=at){
            if(!ready(shot.caster)||!ready(shot.target))return false;
            if(shot.plan.radius)for(const auto&[id,actor]:players_)if(!ready(actor.effect_scope))return false;
        }
        for(const auto& shot:stone_projectiles_)if(!shot.visual_pending&&shot.release!=UINT64_MAX&&
                (shot.launched?shot.arrives:shot.release)<=at&&(!ready(shot.caster)||!ready(shot.target)))return false;
        const auto population_ready=[&]{
            for(const auto&[id,actor]:players_)if(!ready(actor.effect_scope))return false;
            for(const auto&[id,actor]:npc_combat_)if(!ready(actor.hex_scope()))return false;
            return true;
        };
        for(const auto& rift:spirit_rifts_)if(rift.armed&&rift.pulse.at<=at&&!population_ready())return false;
        for(const auto& shot:javelin_projectiles_)if(shot.flight.pending()&&shot.flight.arrives()<=at&&
            (!ready(shot.caster)||!population_ready()))return false;
        for(const auto& shot:deathly_swarms_)if(shot.chain.pending()&&shot.chain.due()<=at&&
            (!ready(shot.chain.caster())||!population_ready()))return false;
        const auto current_npc=[&](skills::effects::Scope scope){
            const auto live=npc_combat_.find(scope.actor);
            return npcs_.contains(scope.actor)&&live!=npc_combat_.end()&&live->second.hex_scope()==scope;
        };
        for(const auto&[id,ai]:npc_flare_ai_)if(current_npc(ai.policy.source))
            for(const auto& shot:ai.shots)if(shot.arrives<=at&&(!ready(ai.policy.source)||!ready(shot.target)))return false;
        for(const auto&[id,ai]:npc_elemental_ai_)if(current_npc(ai.motion.policy.source))
            for(const auto release:ai.ledger.released_ids()){
                const auto probe=ai.ledger.prepare_impact(release,ai.motion.policy.source,{},at);
                if(probe.release.arrives>at)continue;
                if(!ready(ai.motion.policy.source)||!ready(probe.release.cast.use.target))return false;
                if(probe.release.cast.use.plan.radius())for(const auto&[actor,p]:players_)if(!ready(p.effect_scope))return false;
            }
        for(const auto&[id,ai]:npc_ordinary_ai_)if(current_npc(ai.source))
            for(const auto release:ai.ledger.released_ids())if(const auto shot=ai.ledger.released(release);shot&&npc_ordinary_observer_frontier_locked(ai,*shot)<=at)
                if(ai.ledger.clock()>at||!ready(ai.source)||!ready(shot->attack.target))return false;
        return true;
    }
    void resolve_npc_deliveries_locked(std::uint64_t at){
        // Keep identities, not iterators, across recipient/death callbacks.
        std::vector<skills::effects::Scope> sources;
        for(const auto&[id,ai]:npc_flare_ai_)sources.push_back(ai.policy.source);
        for(const auto source:sources){const auto ai=npc_flare_ai_.find(source.actor);
            if(ai==npc_flare_ai_.end()||ai->second.policy.source!=source)continue;
            if(std::none_of(ai->second.shots.begin(),ai->second.shots.end(),[&](const auto& shot){return shot.arrives<=at;}))continue;
            if(const auto live=npc_combat_.find(source.actor);live!=npc_combat_.end()&&live->second.hex_scope()==source)
                advance_npc_conditions_locked(source.actor,live->second,at);
            // Condition/death callbacks may retire an owner. Reacquire the
            // exact lifetime before using any controller reference again.
            const auto current=npc_flare_ai_.find(source.actor);
            if(current!=npc_flare_ai_.end()&&current->second.policy.source==source)
                resolve_npc_flare_releases_locked(current->second,at);
        }
        sources.clear();for(const auto&[id,ai]:npc_elemental_ai_)sources.push_back(ai.motion.policy.source);
        for(const auto source:sources){const auto ai=npc_elemental_ai_.find(source.actor);
            if(ai==npc_elemental_ai_.end()||ai->second.motion.policy.source!=source)continue;
            const auto deadline=ai->second.ledger.next_arrival();if(!deadline||*deadline>at)continue;
            if(const auto live=npc_combat_.find(source.actor);live!=npc_combat_.end()&&live->second.hex_scope()==source)
                advance_npc_conditions_locked(source.actor,live->second,at);
            const auto current=npc_elemental_ai_.find(source.actor);
            if(current!=npc_elemental_ai_.end()&&current->second.motion.policy.source==source)
                resolve_npc_elemental_releases_locked(current->second,at);
        }
        sources.clear();for(const auto&[id,ai]:npc_ordinary_ai_)sources.push_back(ai.source);
        for(const auto source:sources){const auto ai=npc_ordinary_ai_.find(source.actor);
            if(ai==npc_ordinary_ai_.end()||ai->second.source!=source)continue;
            const auto deadline=ai->second.ledger.next_arrival();if(!deadline||*deadline>at)continue;
            const auto live=npc_combat_.find(source.actor);const auto body=npcs_.find(source.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=source){retire_npc_ordinary_locked(source,at);continue;}
            advance_npc_conditions_locked(source.actor,live->second,at);
            const auto current=npc_ordinary_ai_.find(source.actor);
            const auto current_body=npcs_.find(source.actor);const auto current_live=npc_combat_.find(source.actor);
            if(current==npc_ordinary_ai_.end()||current->second.source!=source)continue;
            if(current_body==npcs_.end()||current_live==npc_combat_.end()||current_live->second.hex_scope()!=source){
                retire_npc_ordinary_locked(source,at);continue;
            }
            resolve_npc_ordinary_locked(current->second,current_body->second,current_live->second,at);
        }
    }
