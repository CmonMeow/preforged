// Included inside Instance. Death ownership survives hex cleanup and refusals.
private:
    skills::effects::PutridBileEvents putrid_bile_events_;
    struct PutridProgress {std::vector<skills::effects::Scope> recipients;std::size_t cursor=0;std::optional<std::uint64_t> waiting;};
    std::map<std::uint64_t,PutridProgress> putrid_progress_;
    std::set<std::uint64_t> putrid_processing_;
    std::set<skills::effects::Scope> putrid_dead_;
    std::map<std::uint64_t,std::string> putrid_refusals_;
    bool refuse_putrid_locked(const skills::effects::PutridBileDeathEvent& event,const std::string& reason){
        auto& previous=putrid_refusals_[event.serial];
        if(previous!=reason){previous=reason;std::osyncstream(std::clog)<<"WORLD putrid_pending serial="<<event.serial
            <<" death_at="<<event.at<<" target="<<event.link.target.actor<<" reason="<<reason<<'\n';}
        return false;
    }
    bool consume_putrid_locked(const skills::effects::PutridBileDeathEvent& event){
        if(!putrid_bile_events_.consume(event))return false;
        putrid_progress_.erase(event.serial);putrid_refusals_.erase(event.serial);return true;
    }
    struct PutridNode {
        skills::effects::PutridBileLink link;unsigned plane=0;double x=0,y=0;
        std::vector<skills::effects::Scope> recipients;
    };
    std::optional<std::uint64_t> next_putrid_death_locked()const{
        const auto event=putrid_bile_events_.first_due(UINT64_MAX,putrid_processing_);
        return event?std::optional(event->at):std::nullopt;
    }
    // Collect the potentially reachable death graph before copying authority.
    // Only actual projected deaths fire its edges; listing a link is not death.
    bool prepare_putrid_graph_locked(const skills::effects::PutridBileDeathEvent& event,
            std::map<skills::effects::Scope,PutridNode>& graph){
        using namespace skills::effects;
        graph.emplace(event.link.target,PutridNode{event.link,event.plane,event.x,event.y,{}});
        std::vector<Scope> work{event.link.target};
        for(std::size_t index=0;index<work.size();++index){
            const auto dead=work[index];auto& node=graph.at(dead);const auto plan=putrid_bile(node.link.rank);if(!plan)return false;
            const auto caster=players_.find(node.link.caster.actor);
            if(caster==players_.end()||caster->second.effect_scope!=node.link.caster||!caster->second.team_token)continue;
            const auto team=caster->second.team_token;
            for(const auto scope:event.population){
                if(scope==dead||putrid_dead_.contains(scope))continue;
                bool selected=false;unsigned plane=0;double x=0,y=0;
                if(auto found=players_.find(scope.actor);found!=players_.end()){
                    auto& p=found->second;
                    if(p.effect_scope!=scope||!p.resources)continue;
                    // Validate history before using a later corpse/plane/pose
                    // as proof that this lifetime was ineligible at death time.
                    if(std::max({p.health_updated,p.energy_updated,p.last_move,p.turn_updated,p.steering?p.steering_tick:0})>event.at)
                        return refuse_putrid_locked(event,"player-clock actor="+std::to_string(scope.actor));
                    if(!p.team_token||p.team_token==team||p.precise_health<=0||p.surface.plane!=node.plane)continue;
                    const auto point=healing_position_locked(p,event.at);if(!point)return refuse_putrid_locked(event,"player-position actor="+std::to_string(scope.actor));
                    if(std::hypot(point->x-node.x,point->z-node.y)>plan->death_radius)continue;
                    if(!advance_health_locked(p,event.at))return refuse_putrid_locked(event,"player-health actor="+std::to_string(scope.actor));
                    if(p.effect_scope!=scope||p.precise_health<=0)continue;
                    plane=p.surface.plane;x=point->x;y=point->z;selected=true;
                }else{
                    auto npc_found=npc_combat_.find(scope.actor);auto placed=npcs_.find(scope.actor);
                    if(npc_found==npc_combat_.end()||placed==npcs_.end()||npc_found->second.hex_scope()!=scope)continue;
                    const auto life=npc_found->second.lifetime();
                    bool future=event.at<npc_found->second.health_clock(life).value_or(event.at)||event.at<npc_found->second.energy_clock(life).value_or(event.at);
                    if(const auto motion=quest_npc_motion_.find(scope.actor);motion!=quest_npc_motion_.end()&&motion->second.scope==scope)future|=motion->second.updated>event.at;
                    if(const auto motion=npc_flare_ai_.find(scope.actor);motion!=npc_flare_ai_.end()&&motion->second.policy.source==scope)future|=motion->second.updated>event.at;
                    if(const auto motion=npc_elemental_ai_.find(scope.actor);motion!=npc_elemental_ai_.end()&&motion->second.motion.policy.source==scope)future|=motion->second.motion.updated>event.at;
                    if(future)return refuse_putrid_locked(event,"npc-clock actor="+std::to_string(scope.actor));
                    if(!npc_found->second.hostile_to_players(life)||placed->second.plane!=node.plane)continue;
                    refresh_npc_movement_locked(scope.actor,event.at);
                    npc_found=npc_combat_.find(scope.actor);placed=npcs_.find(scope.actor);
                    if(npc_found==npc_combat_.end()||placed==npcs_.end()||npc_found->second.hex_scope()!=scope)continue;
                    if(std::hypot(placed->second.x-node.x,placed->second.y-node.y)>plan->death_radius)continue;
                    advance_npc_conditions_locked(scope.actor,npc_found->second,event.at);
                    npc_found=npc_combat_.find(scope.actor);placed=npcs_.find(scope.actor);
                    if(npc_found==npc_combat_.end()||placed==npcs_.end()||npc_found->second.hex_scope()!=scope)continue;
                    const auto actor=npc_found->second.actor(npc_found->second.lifetime());
                    if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)continue;
                    plane=placed->second.plane;x=placed->second.x;y=placed->second.y;selected=true;
                }
                if(!selected)continue;
                node.recipients.push_back(scope);
                if(graph.contains(scope))continue;
                const auto link=putrid_bile_links_.lookup(scope);
                if(!link||event.at<link->applied||event.at>=link->expires)continue;
                std::optional<Entry> hex;
                if(const auto p=players_.find(scope.actor);p!=players_.end()){
                    if(p->second.effect_registry)hex=p->second.effect_registry->lookup(scope,link->effect);
                }else if(const auto n=npc_combat_.find(scope.actor);n!=npc_combat_.end())hex=n->second.hex_effect(n->second.lifetime(),link->effect);
                if(!hex||hex->skill!=2058||hex->cause!=link->caster.actor||hex->rank!=link->rank||!hex->expires||event.at>=*hex->expires)continue;
                graph.emplace(scope,PutridNode{*link,plane,x,y,{}});work.push_back(scope);
            }
        }
        if(const auto progress=putrid_progress_.find(event.serial);progress!=putrid_progress_.end())
            graph.at(event.link.target).recipients.assign(progress->second.recipients.begin()+progress->second.cursor,progress->second.recipients.end());
        return true;
    }
    bool project_putrid_tree_locked(const skills::effects::Scope dead,
            const std::map<skills::effects::Scope,PutridNode>& graph,DamageAdmissionBatch& batch,
            std::set<skills::effects::Scope>& exploded,std::uint64_t now){
        if(!exploded.insert(dead).second)return true;
        const auto& node=graph.at(dead);const auto plan=*skills::effects::putrid_bile(node.link.rank);
        const combat::Damage damage{combat::Kind::armor_ignoring,double(plan.death_damage),node.link.caster_level,2058};
        for(const auto scope:node.recipients){
            if(exploded.contains(scope)||putrid_dead_.contains(scope))continue;
            bool killed=false;
            if(const auto found=players_.find(scope.actor);found!=players_.end()){
                auto& p=found->second;if(p.effect_scope!=scope)continue;
                auto* state=admission_player_locked(batch,p,now);if(!state)return false;if(state->health.current<=0)continue;
                auto actor=incoming_spell_actor_locked(p,{p.id,scope.generation,
                    combat::Health{state->health.current,state->health.capacity.maximum()},{},p.effects,true},now);
                if(!project_batch_player_damage_locked(batch,p,actor,damage,now))return false;
                killed=state->health.current<=0;
            }else{
                const auto n=npc_combat_.find(scope.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=scope)continue;
                auto proposed=batch.npcs.contains(scope)?batch.npcs.at(scope):n->second;
                const auto actor=proposed.actor(proposed.lifetime());if(!actor||!actor->health||actor->health->current<=0)continue;
                if(!preflight_npc_reversal_or_damage_locked(proposed,damage,now))return false;
                killed=proposed.actor(proposed.lifetime())->health->current<=0;batch.npcs.insert_or_assign(scope,std::move(proposed));
            }
            if(killed&&graph.contains(scope)&&!project_putrid_tree_locked(scope,graph,batch,exploded,now))return false;
        }
        return true;
    }
    bool attempt_putrid_death_locked(const skills::effects::PutridBileDeathEvent& event){
        using namespace skills::effects;
        if(putrid_processing_.contains(event.serial))return true;
        const auto source=players_.find(event.link.caster.actor);
        if(source==players_.end()||source->second.effect_scope!=event.link.caster||!source->second.team_token){
            return consume_putrid_locked(event);}
        putrid_processing_.insert(event.serial);const bool newly_dead=putrid_dead_.insert(event.link.target).second;
        struct Reset {std::set<std::uint64_t>& processing;std::uint64_t serial;std::set<Scope>& dead;Scope target;bool added;
            ~Reset(){processing.erase(serial);if(added)dead.erase(target);}} reset{putrid_processing_,event.serial,putrid_dead_,event.link.target,newly_dead};
        if(const auto progress=putrid_progress_.find(event.serial);progress!=putrid_progress_.end()&&progress->second.waiting){
            const auto nested=putrid_bile_events_.lookup(*progress->second.waiting);
            if(nested&&!attempt_putrid_death_locked(*nested))return false;
            progress->second.waiting.reset();
        }
        std::map<Scope,HealingSeedReaction> facts;std::map<Scope,PutridNode> graph;bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            graph.clear();if(!prepare_putrid_graph_locked(event,graph))return false;
            DamageAdmissionBatch batch;batch.seed_facts=&facts;std::set<Scope> exploded;
            if(project_putrid_tree_locked(event.link.target,graph,batch,exploded,event.at)){admitted=true;break;}
            if(!batch.missing_seed)return refuse_putrid_locked(event,"projected-damage-or-protection");
            const auto p=players_.find(batch.missing_seed->actor);
            if(p==players_.end()||p->second.effect_scope!=*batch.missing_seed)return false;
            const auto fact=prepare_healing_seed_direct_locked(p->second,1.,event.at);if(!fact)return refuse_putrid_locked(event,"seed-recipient-facts actor="+std::to_string(p->first));
            facts[*batch.missing_seed]=*fact;
        }
        if(!admitted)return refuse_putrid_locked(event,"admission-restart-limit");
        auto [progress_it,inserted]=putrid_progress_.try_emplace(event.serial,PutridProgress{graph.at(event.link.target).recipients,0});
        // Each hit is committed once. If an unmodelled linked-end callback
        // defers a nested event, retain this exact cursor instead of replaying.
        while(progress_it->second.cursor<progress_it->second.recipients.size()){
            const auto scope=progress_it->second.recipients[progress_it->second.cursor];
            const auto plan=*putrid_bile(event.link.rank);
            const combat::Damage damage{combat::Kind::armor_ignoring,double(plan.death_damage),event.link.caster_level,2058};
            if(!putrid_dead_.contains(scope)){
                if(auto found=players_.find(scope.actor);found!=players_.end()){
                    auto& p=found->second;if(p.effect_scope==scope&&p.resources&&p.precise_health>0){
                        auto actor=incoming_spell_actor_locked(p,{p.id,scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true},event.at);
                        const auto hit=combat::apply(actor,damage,true);
                        if(!commit_player_reactive_hit_locked(p,hit,event.link.caster.actor,event.at,false,nullptr,false,false,nullptr,event.link.caster))
                            return false;
                    }
                }else{
                    auto live=npc_combat_.find(scope.actor);const auto placed=npcs_.find(scope.actor);
                    const auto owner=players_.find(event.link.caster.actor);
                    if(live!=npc_combat_.end()&&placed!=npcs_.end()&&live->second.hex_scope()==scope&&owner!=players_.end()&&owner->second.effect_scope==event.link.caster){
                        const auto actor=live->second.actor(live->second.lifetime());
                        if(actor&&actor->health&&actor->health->current>0&&
                           !commit_npc_reversal_or_damage_locked(owner->second,placed->second,live->second,damage,event.at))return false;
                    }
                }
            }
            ++progress_it->second.cursor;
            while(const auto nested=putrid_bile_events_.first_due(event.at,putrid_processing_))
                if(!attempt_putrid_death_locked(*nested)){progress_it->second.waiting=nested->serial;return false;}
        }
        return consume_putrid_locked(event);
    }
    bool dispatch_putrid_deaths_locked(std::uint64_t now){
        if(!putrid_processing_.empty())return true;
        while(const auto event=putrid_bile_events_.first_due(now))if(!attempt_putrid_death_locked(*event))return false;
        return true;
    }
    void putrid_bile_death_locked(const skills::effects::PutridBileLink& link,
            unsigned plane,double x,double y,std::uint64_t now){
        std::vector<skills::effects::Scope> population;population.reserve(players_.size()+npc_combat_.size());
        for(const auto& [id,p]:players_)population.push_back(p.effect_scope);
        for(const auto& [id,n]:npc_combat_)population.push_back(n.hex_scope());
        const auto event=putrid_bile_events_.reserve(link,plane,x,y,now,std::move(population));
        if(!event)throw std::logic_error("invalid Putrid Bile death reservation");
        (void)attempt_putrid_death_locked(*event);
    }
public:
    std::size_t pending_putrid_deaths()const{std::lock_guard lock(mutex_);return putrid_bile_events_.size();}
    std::vector<std::string> putrid_death_diagnostics()const{
        std::lock_guard lock(mutex_);std::vector<std::string> result;
        for(const auto& [serial,reason]:putrid_refusals_)result.push_back(std::to_string(serial)+":"+reason);
        return result;
    }
private:
