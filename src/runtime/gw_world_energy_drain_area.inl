// Included inside Instance. Shared mixed player/NPC recipients for the
// damage/area branch;single-target Energy return/healing retains its producer.
private:
    bool energy_drain_area_locked(skills::effects::Scope scope,skills::effects::Scope target,
            const skills::effects::EnergyDrainPlan& plan,unsigned level,std::uint64_t now,
            std::vector<Packet>* visuals,std::optional<std::uint64_t> completion_event){
        if(plan.skill!=39&&plan.skill!=42&&plan.skill!=59)return false;
        auto caster=players_.find(scope.actor);
        if(caster==players_.end()||caster->second.effect_scope!=scope||!caster->second.resources||
           caster->second.precise_health<=0||caster->second.knocked_down_until>now||
           now<caster->second.health_updated||now<caster->second.energy_updated)return false;
        auto& source=caster->second;const auto origin=healing_position_locked(source,now);if(!origin)return false;
        float x=0,y=0;std::size_t plane=0;
        const auto selected=players_.find(target.actor);
        if(selected!=players_.end()){
            auto& p=selected->second;const auto point=healing_position_locked(p,now);
            if(p.effect_scope!=target||!p.resources||p.precise_health<=0||!source.team_token||!p.team_token||
               source.team_token==p.team_token||!point)return false;
            x=point->x;y=point->z;plane=p.surface.plane;
        }else{
            const auto n=npcs_.find(target.actor);auto live=npc_combat_.find(target.actor);
            if(n==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=target||
               !live->second.hostile_to_players(live->second.lifetime()))return false;
            refresh_npc_movement_locked(target.actor,now);x=n->second.x;y=n->second.y;plane=n->second.plane;
        }
        if(source.surface.plane!=plane||std::hypot(x-origin->x,y-origin->z)>plan.range)return false;
        if(navigation_){const auto trace=layered_->trace(source.surface,*origin,{x,y});if(!trace||trace->plane!=plane)return false;}
        const auto included=[&](unsigned id,float px,float py){return id==target.actor||
            (plan.radius&&std::hypot(px-x,py-y)<=plan.radius);};
        struct PlayerDrain {skills::effects::Scope scope;skills::effects::EnergyDrainResult drain;
            skills::effects::MindWrackEnergy before,after;bool energy_committed=false;};
        struct NpcDrain {skills::effects::Scope scope;skills::effects::EnergyDrainResult drain;
            skills::effects::MindWrackEnergy before,after;bool energy_committed=false;};
        std::vector<PlayerDrain> player_drains;std::vector<NpcDrain> npc_drains;
        bool found=false;
        if(!advance_resource_event_locked(source,now)||source.precise_health<=0)return false;
        for(auto&[id,p]:players_){
            if(!source.team_token||!p.team_token||source.team_token==p.team_token||!p.resources||
               p.precise_health<=0||p.surface.plane!=plane)continue;
            const auto point=healing_position_locked(p,now);if(!point)return false;
            if(!included(id,point->x,point->z))continue;
            if(now<p.health_updated||now<p.energy_updated||!p.resources->max_energy||
               (plan.weakness_ms&&now>UINT64_MAX-plan.weakness_ms))return false;
            if(!advance_resource_event_locked(p,now))return false;if(p.precise_health<=0)continue;
            if(!settle_mind_wrack_caster_locked(p.effect_scope,now))return false;
            if(p.precise_health<=0)continue;
            player_drains.push_back({p.effect_scope,{},{},{},false});found|=p.effect_scope==target;
        }
        for(auto&[id,live]:npc_combat_){
            const auto n=npcs_.find(id);const auto life=live.lifetime();
            if(n==npcs_.end()||n->second.plane!=plane||!live.hostile_to_players(life))continue;
            refresh_npc_movement_locked(id,now);if(!included(id,n->second.x,n->second.y))continue;
            if(now<live.health_clock(life).value_or(now))return false;
            advance_npc_conditions_locked(id,live,now);const auto* actor=live.actor(life);
            if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)continue;
            skills::effects::MindWrackEnergy before{};
            if(plan.drain_each||id==target.actor){
                if(!live.energy(life)||!settle_mind_wrack_caster_locked(live.hex_scope(),now)||!live.advance_energy(life,now))return false;
                actor=live.actor(life);if(!actor||!actor->health||actor->health->current<=0)continue;
                const auto e=*live.energy(life);const auto units=live.precise_energy_units(life);
                before={e.current,e.maximum,0,unsigned(units.value_or(0)%overcast::energy_unit)};
            }
            npc_drains.push_back({live.hex_scope(),{},before,{},false});found|=live.hex_scope()==target;
        }
        caster=players_.find(scope.actor);
        if(!found||caster==players_.end()||caster->second.effect_scope!=scope||caster->second.precise_health<=0)return false;
        // Seed admission may advance real timelines. Rebuild every Energy and
        // Health proposal after discovering a new fact, before spending either.
        std::map<skills::effects::Scope,skills::effects::HealingSeedReaction> seed_facts;
        combat::Damage damage{combat::Kind::armor_ignoring,0,level,plan.skill};
        bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            std::optional<skills::effects::EnergyDrainResult> primary;
            if(const auto player=players_.find(target.actor);player!=players_.end()){
                if(player->second.effect_scope!=target||!player->second.resources||player->second.precise_health<=0)return false;
                primary=skills::effects::drain_energy(plan,*player->second.resources);
            }else{
                const auto npc=npc_combat_.find(target.actor);
                if(npc==npc_combat_.end()||npc->second.hex_scope()!=target)return false;
                const auto life=npc->second.lifetime();const auto e=npc->second.energy(life);const auto actor=npc->second.actor(life);
                if(!e||!actor||!actor->health||actor->health->current<=0)return false;
                primary=skills::effects::drain_energy(plan,{static_cast<unsigned>(std::ceil(actor->health->current)),actor->health->maximum,e->current,e->maximum});
            }
            if(!primary)return false;damage.amount=double(primary->damage);
            DamageAdmissionBatch batch;batch.seed_facts=&seed_facts;bool valid=true;
            for(auto& c:player_drains){
                const auto found=players_.find(c.scope.actor);if(found==players_.end()||found->second.effect_scope!=c.scope)continue;
                const auto& player=found->second;auto* state=admission_player_locked(batch,player,now);
                if(!state)return false;if(state->health.current<=0)continue;
                if(plan.drain_each||c.scope==target){
                    const auto drain=skills::effects::drain_energy(plan,*player.resources);if(!drain)return false;c.drain=*drain;
                    if(player.overcast_curve&&!overcast::prepare_loss(*player.overcast_curve,c.scope,*player.resources,
                            player.energy_regeneration,player.energy_debt,c.drain.lost,now,0,overcast::LossRule::full_cost,player.energy_fractional_tail))return false;
                }
                if(damage.amount>0){
                    auto actor=incoming_spell_actor_locked(player,{player.id,c.scope.generation,
                        combat::Health{state->health.current,state->health.capacity.maximum()},{},player.effects,true},now);
                    if(!project_batch_player_damage_locked(batch,player,actor,damage,now)){valid=false;break;}
                }
                if(plan.weakness_ms&&state->health.current>0){
                    auto conditions=player.conditions;
                    if(!conditions.apply(combat::conditions::Kind::weakness,scope.actor,now,plan.weakness_ms))return false;
                    const auto old=player.conditions.lookup(combat::conditions::Kind::weakness),current=conditions.lookup(combat::conditions::Kind::weakness);
                    if((!old||old->expires!=current->expires||!player.condition_effects.contains(combat::conditions::Kind::weakness))&&
                       player.next_health_effect>UINT32_MAX)return false;
                    if(!project_batch_condition_reactions_locked(batch,player,player.conditions,conditions,now)){valid=false;break;}
                }
            }
            if(batch.missing_seed){
                const auto owner=*batch.missing_seed;auto player=players_.find(owner.actor);
                if(player==players_.end()||player->second.effect_scope!=owner)return false;
                const auto fact=prepare_healing_seed_direct_locked(player->second,1.,now);if(!fact)return false;
                seed_facts[owner]=*fact;continue;
            }
            if(!valid)return false;
            for(auto& c:npc_drains){
                const auto found=npc_combat_.find(c.scope.actor);
                if(found==npc_combat_.end()||found->second.hex_scope()!=c.scope)continue;
                auto preview=batch.npcs.contains(c.scope)?batch.npcs.at(c.scope):found->second;
                const auto life=preview.lifetime();const auto actor=preview.actor(life);
                if(!actor||!actor->health||actor->health->current<=0)continue;
                if(plan.drain_each||c.scope==target){
                    const auto e=preview.energy(life);if(!e)return false;
                    const auto drain=skills::effects::drain_energy(plan,{static_cast<unsigned>(std::ceil(actor->health->current)),actor->health->maximum,e->current,e->maximum});
                    if(!drain||!preview.lose_energy(life,drain->lost))return false;c.drain=*drain;
                }
                if(damage.amount>0&&!preflight_npc_reversal_or_damage_locked(preview,damage,now))return false;
                if(plan.weakness_ms&&preview.actor(life)->health->current>0&&preview.allows_conditions(life)&&
                   !preview.apply_condition(life,combat::conditions::Kind::weakness,scope.actor,now,plan.weakness_ms))return false;
                for(const auto& event:preview.take_fragility_hits(life))
                    if(event.reversal&&!validate_npc_reversal_receipts_locked(c.scope,{event.reversal}))return false;
            }
            admitted=true;break;
        }
        if(!admitted)return false;
        if(completion_event&&!complete_ether_renewal_locked(scope,plan.skill,*completion_event,now,true))return false;
        const auto credit=[&]()->Player*{const auto found=players_.find(scope.actor);
            return found!=players_.end()&&found->second.effect_scope==scope?&found->second:nullptr;};
        if(!credit()||credit()->precise_health<=0)return true;
        for(auto& c:player_drains){
            auto found=players_.find(c.scope.actor);
            if(found==players_.end()||found->second.effect_scope!=c.scope||found->second.precise_health<=0)continue;
            auto& player=found->second;
            if(plan.drain_each||c.scope==target){
                // Callback-bearing admission invalidates saved full Energy states.
                // Prepare the actual loss again from this scoped recipient now.
                const auto drain=skills::effects::drain_energy(plan,*player.resources);
                if(!drain)throw std::logic_error("preflighted current player Energy drain failed");
                c.before=mind_wrack_energy_locked(player);c.drain=*drain;
                if(player.overcast_curve){
                    auto energy=overcast::prepare_loss(*player.overcast_curve,c.scope,*player.resources,player.energy_regeneration,
                        player.energy_debt,c.drain.lost,now,0,overcast::LossRule::full_cost,player.energy_fractional_tail);
                    if(!energy)throw std::logic_error("preflighted current player Energy loss failed");
                    commit_overcast_energy_locked(player,std::move(energy->state));
                }else player.resources->energy=c.drain.target_energy;
                c.after=mind_wrack_energy_locked(player);c.energy_committed=true;
                if(c.drain.lost)player.outbox->enqueue(owner_vitals(player));
            }
            if(damage.amount>0){
                auto actor=incoming_spell_actor_locked(player,{player.id,c.scope.generation,
                    combat::Health{player.precise_health,player.resources->max_health},{},player.effects,true},now);
                const auto hit=combat::apply(actor,damage,true);
                if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(player,hit,scope.actor,now,
                        false,nullptr,false,true,nullptr,scope))throw std::logic_error("preflighted player drain damage failed");
            }
            found=players_.find(c.scope.actor);
            if(found!=players_.end()&&found->second.effect_scope==c.scope&&found->second.precise_health>0&&plan.weakness_ms){
                auto conditions=found->second.conditions;
                if(!conditions.apply(combat::conditions::Kind::weakness,scope.actor,now,plan.weakness_ms))
                    throw std::logic_error("preflighted current drain Weakness failed");
                const auto old=found->second.conditions.lookup(combat::conditions::Kind::weakness),current=conditions.lookup(combat::conditions::Kind::weakness);
                if((!old||old->expires!=current->expires||!found->second.condition_effects.contains(combat::conditions::Kind::weakness))&&
                   found->second.next_health_effect>UINT32_MAX)throw std::logic_error("preflighted current drain condition capacity exhausted");
                commit_conditions_locked(found->second,conditions,{combat::conditions::Kind::weakness},now);
            }
        }
        for(auto& c:npc_drains){
            auto found=npc_combat_.find(c.scope.actor);auto placed=npcs_.find(c.scope.actor);
            if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=c.scope)continue;
            auto life=found->second.lifetime();const auto actor=found->second.actor(life);
            if(!actor||!actor->health||actor->health->current<=0)continue;
            if(plan.drain_each||c.scope==target){
                const auto e=found->second.energy(life);const auto units=found->second.precise_energy_units(life);if(!e)return false;
                c.before={e->current,e->maximum,0,unsigned(units.value_or(0)%overcast::energy_unit)};
                const auto drain=skills::effects::drain_energy(plan,{static_cast<unsigned>(std::ceil(actor->health->current)),actor->health->maximum,e->current,e->maximum});
                if(!drain||!found->second.lose_energy(life,drain->lost))throw std::logic_error("preflighted current NPC drain failed");
                c.drain=*drain;const auto after=*found->second.energy(life);const auto after_units=found->second.precise_energy_units(life);
                c.after={after.current,after.maximum,0,unsigned(after_units.value_or(0)%overcast::energy_unit)};
                c.energy_committed=true;
            }
            if(damage.amount>0&&!commit_npc_reversal_or_damage_locked(credit(),placed->second,found->second,
                    damage,now,nullptr,nullptr,nullptr,false,scope))throw std::logic_error("preflighted NPC drain damage failed");
            found=npc_combat_.find(c.scope.actor);
            if(found==npc_combat_.end()||found->second.hex_scope()!=c.scope)continue;
            life=found->second.lifetime();const auto current=found->second.actor(life);
            if(plan.weakness_ms&&current&&current->health&&current->health->current>0&&found->second.allows_conditions(life)){
                if(!found->second.apply_condition(life,combat::conditions::Kind::weakness,scope.actor,now,plan.weakness_ms))throw std::logic_error("preflighted NPC drain Weakness failed");
                npc_condition_credit_[{c.scope.actor,scope.actor}]={c.scope,scope};publish_npc_fragility_locked(c.scope.actor,found->second);
                found=npc_combat_.find(c.scope.actor);
                if(found!=npc_combat_.end()&&found->second.hex_scope()==c.scope)
                    for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(c.scope.actor,found->second.actor(life)->effects)});
            }
        }
        for(const auto& c:player_drains)if(c.energy_committed){
            const auto found=players_.find(c.scope.actor);
            if(found!=players_.end()&&found->second.effect_scope==c.scope&&found->second.precise_health>0)
                observe_mind_wrack_energy_locked(found->second,c.before,now,c.after);
        }
        for(const auto& c:npc_drains)if(c.energy_committed)
            observe_npc_mind_wrack_energy_locked(c.scope,c.before,now,c.after);
        if(plan.skill!=59)publish_target_body_locked(target.actor,plan.skill,scope.actor,visuals);
        return true;
    }
public:
