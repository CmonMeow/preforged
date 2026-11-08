// Trusted effect completion only. Paid97 Runtime/gateway remains unbound.
private:
    std::mt19937_64 necrotic_rng_{std::random_device{}()};
    std::function<std::size_t(std::size_t)> necrotic_draw_;
    std::map<skills::effects::Scope,std::uint64_t> necrotic_attempts_;
    std::optional<skills::effects::TraversalCorpse> traversal_corpse_locked(skills::effects::Scope scope,std::uint64_t now)const{
        using skills::effects::TraversalCorpse;
        if(const auto p=players_.find(scope.actor);p!=players_.end()){
            const auto& body=p->second;
            if(body.effect_scope!=scope||!body.resources||body.precise_health>0||!(body.effects&0x10)||
               !body.died_at||*body.died_at>now||!body.death_sequence||!body.corpse_position)return {};
            return TraversalCorpse{scope,body.death_sequence,static_cast<unsigned>(body.corpse_position->surface.plane),body.corpse_position->point.x,body.corpse_position->point.z,true,
                body.corpse_exploitable,!corpse_reservations_.available(scope)};
        }
        const auto placed=npcs_.find(scope.actor);const auto live=npc_combat_.find(scope.actor);
        if(placed==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=scope)return {};
        const auto life=live->second.lifetime();const auto* actor=live->second.actor(life);
        if(!actor||!actor->health||actor->health->current!=0||now<live->second.health_clock(life).value_or(now))return {};
        return TraversalCorpse{scope,1,placed->second.plane,placed->second.x,placed->second.y,true,
            live->second.fleshy(life),!corpse_reservations_.available(scope)};
    }
    std::optional<skills::effects::TraversalPool> prepare_necrotic_pool_locked(
            skills::effects::Scope scope,const skills::effects::NecroticTraversalPlan& plan,
            const navigation::SurfacePoint& origin,std::uint64_t now){
        std::vector<unsigned> npc_ids;for(const auto& [id,npc]:npcs_)npc_ids.push_back(id);
        for(const auto id:npc_ids){
            auto live=npc_combat_.find(id);if(live==npc_combat_.end())continue;
            if(now<live->second.health_clock(live->second.lifetime()).value_or(now))return {};
            advance_npc_conditions_locked(id,live->second,now);refresh_npc_movement_locked(id,now);
        }
        std::vector<skills::effects::TraversalCorpse> corpses;
        for(const auto& [id,p]:players_){
            if(p.resources&&p.precise_health<=0&&p.died_at&&*p.died_at<=now&&
               corpse_reservations_.available(p.effect_scope)&&!p.corpse_position)return {};
            if(const auto fact=traversal_corpse_locked(p.effect_scope,now))corpses.push_back(*fact);
        }
        for(const auto& [id,live]:npc_combat_)if(const auto fact=traversal_corpse_locked(live.hex_scope(),now))corpses.push_back(*fact);
        return skills::effects::necrotic_traversal_pool(plan,scope,origin.point.x,origin.point.z,corpses);
    }
public:
    struct NecroticTraversalReceipt {
        skills::effects::TraversalCorpseClaim corpse;
        navigation::SurfacePoint origin,destination;
        std::vector<skills::effects::Scope> condition_recipients;
        std::optional<ground::Item> dropped_bundle;
    };
    void configure_necrotic_corpse_draws(std::function<std::size_t(std::size_t)> draw){
        std::lock_guard lock(mutex_);necrotic_draw_=std::move(draw);
    }
    std::optional<skills::effects::TraversalCorpse> traversal_corpse_snapshot(skills::effects::Scope scope,std::uint64_t now)const{
        std::lock_guard lock(mutex_);return traversal_corpse_locked(scope,now);
    }
    // Cast admission observes the same settled pool as completion, but never
    // draws, reserves, consumes, or pins a corpse across the activation window.
    // nullopt means unavailable authority; zero means a known empty pool.
    std::optional<std::size_t> necrotic_traversal_available(skills::effects::Scope scope,
            const skills::effects::NecroticTraversalPlan& plan,std::uint64_t now){
        if(skills::effects::necrotic_traversal(plan.rank)!=plan||!now||now>UINT64_MAX-plan.poison)return {};
        std::lock_guard lock(mutex_);
        if(!navigation_||!layered_||!settle_timed_effects_locked(now))return {};
        const auto source=players_.find(scope.actor);
        if(source==players_.end()||source->second.effect_scope!=scope||!source->second.resources||
           !source->second.team_token||source->second.precise_health<=0||source->second.knocked_down_until>now||
           !resurrection_clock_valid(source->second,now)||!advance_resource_event_locked(source->second,now)||
           !advance_health_locked(source->second,now)||source->second.precise_health<=0)return {};
        const auto origin=shadow_position_locked(source->second,now);if(!origin)return {};
        const auto pool=prepare_necrotic_pool_locked(scope,plan,*origin,now);
        // NPC settlement may trigger death reactions against the caster.
        // Admission must not retain the pre-settlement living-source verdict.
        const auto current=players_.find(scope.actor);
        if(current==players_.end()||current->second.effect_scope!=scope||
           !current->second.resources||current->second.precise_health<=0||
           current->second.knocked_down_until>now)return {};
        return pool?std::optional(pool->corpses.size()):std::nullopt;
    }
    std::optional<NecroticTraversalReceipt> complete_necrotic_traversal(skills::effects::Scope scope,
            const skills::effects::NecroticTraversalPlan& plan,std::uint64_t attempt,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        using namespace skills::effects;using Kind=combat::conditions::Kind;
        if(publication)*publication={};
        if(necrotic_traversal(plan.rank)!=plan||!attempt||!now||now>UINT64_MAX-plan.poison)return {};
        std::lock_guard lock(mutex_);
        if(!navigation_||!layered_||!settle_timed_effects_locked(now))return {};
        const auto source=players_.find(scope.actor);
        if(source==players_.end()||source->second.effect_scope!=scope||!source->second.resources||
           !source->second.team_token||source->second.precise_health<=0||source->second.knocked_down_until>now||
           !resurrection_clock_valid(source->second,now)||!advance_resource_event_locked(source->second,now)||
           !advance_health_locked(source->second,now)||source->second.precise_health<=0)return {};
        std::erase_if(necrotic_attempts_,[&](const auto& entry){const auto p=players_.find(entry.first.actor);
            return p==players_.end()||p->second.effect_scope!=entry.first;});
        if(const auto old=necrotic_attempts_.find(scope);old!=necrotic_attempts_.end()&&attempt<=old->second)return {};
        necrotic_attempts_[scope]=attempt; // Completion attempt cannot reroll on replay.
        const auto origin=shadow_position_locked(source->second,now);if(!origin)return {};
        const auto pool=prepare_necrotic_pool_locked(scope,plan,*origin,now);
        if(!pool||pool->corpses.empty())return {};
        std::vector<unsigned> npc_ids;for(const auto& [id,npc]:npcs_)npc_ids.push_back(id);
        const auto draw=necrotic_draw_?necrotic_draw_(pool->corpses.size()):std::uniform_int_distribution<std::size_t>(0,pool->corpses.size()-1)(necrotic_rng_);
        const auto claim=select_necrotic_traversal_corpse(*pool,draw,now);if(!claim)return {};
        const navigation::Point destination{claim->corpse.x,claim->corpse.z};
        const auto cell=navigation_->locate_cell(claim->corpse.plane,destination);if(!cell)return {};
        const auto step=layered_->shadow_destination(origin->surface,origin->point,{claim->corpse.plane,*cell},destination,plan.compass);
        // Unreviewed clipped Poison-center behavior must never consume a body.
        if(!step||step->clipped||step->blocked||step->end.surface.plane>65535||
           (navigation_plane_&&step->end.surface.plane!=*navigation_plane_))return {};
        std::vector<Scope> player_targets,npc_targets;
        for(auto& [id,p]:players_){
            if(!p.resources||p.precise_health<=0||!p.team_token||p.team_token==source->second.team_token||p.surface.plane!=step->end.surface.plane)continue;
            const auto point=healing_position_locked(p,now);if(!point)return {};
            if(std::hypot(point->x-step->end.point.x,point->z-step->end.point.z)>plan.nearby)continue;
            if(!p.effect_registry||p.effect_registry->has(p.effect_scope,Category::form,now)||
               p.next_health_effect>UINT32_MAX||!advance_health_locked(p,now))return {};
            if(p.precise_health>0)player_targets.push_back(p.effect_scope);
        }
        for(const auto id:npc_ids){
            const auto body=npcs_.find(id);const auto live=npc_combat_.find(id);
            if(body==npcs_.end()||live==npc_combat_.end())continue;
            const auto life=live->second.lifetime();const auto* actor=live->second.actor(life);
            if(!actor||!actor->health||actor->health->current<=0||!live->second.hostile_to_players(life)||
               body->second.plane!=step->end.surface.plane||!live->second.allows_conditions(life)||
               std::hypot(body->second.x-step->end.point.x,body->second.y-step->end.point.z)>plan.nearby)continue;
            const auto flesh=live->second.fleshy(life);if(!flesh)return {};if(!*flesh)continue;
            npc_targets.push_back(live->second.hex_scope());
        }
        // Freeze one duration per recipient for both copied admission and commit.
        std::map<Scope,std::uint64_t> poison_durations;
        for(const auto& targets:{player_targets,npc_targets})for(const auto target:targets){
            const auto duration=poison_equipment_duration_locked(scope,target,plan.poison,now);
            if(!duration)return {};poison_durations.emplace(target,*duration);
        }
        std::map<Scope,HealingSeedReaction> facts;bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            for(const auto target:player_targets){
                const auto p=players_.find(target.actor);if(p==players_.end()||p->second.effect_scope!=target||!p->second.resources)return {};
                auto* state=admission_player_locked(batch,p->second,now);if(!state)return {};
                if(state->health.current<=0)continue;
                auto conditions=p->second.conditions;
                if(!conditions.apply(Kind::poison,scope.actor,now,poison_durations.at(target)))return {};
                if(!project_batch_condition_reactions_locked(batch,p->second,p->second.conditions,conditions,now)){valid=false;break;}
                if(state->health.current<=0&&putrid_bile_links_.lookup(target))return {}; // Shared recursive admission is not yet generalized.
            }
            if(valid){admitted=true;break;}
            if(!batch.missing_seed)return {};
            const auto p=players_.find(batch.missing_seed->actor);
            if(p==players_.end()||p->second.effect_scope!=*batch.missing_seed)return {};
            const auto seed=prepare_healing_seed_direct_locked(p->second,1.,now);if(!seed)return {};
            facts[*batch.missing_seed]=*seed;
        }
        if(!admitted)return {};
        for(const auto target:npc_targets){
            const auto live=npc_combat_.find(target.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=target)return {};
            auto preview=live->second;const auto life=preview.lifetime();
            if(!preview.apply_condition(life,Kind::poison,scope.actor,now,poison_durations.at(target)))return {};
            const auto reactions=preview.take_fragility_hits(life);
            for(const auto& event:reactions){
                if(event.reversal&&!validate_npc_reversal_receipts_locked(target,{event.reversal}))return {};
                const auto actual=event.reversal?event.reversal->damage:std::optional(event.hit);
                if(actual&&actual->applied>0){
                    for(const auto& link:bonds_of_torment_.pending())if(link.caster==target)return {};
                    if(actual->killed&&putrid_bile_links_.lookup(target))return {};
                }
            }
        }
        const auto current=traversal_corpse_locked(claim->corpse.scope,now);
        if(!current||!consume_necrotic_traversal_corpse(*claim,*current)||
           source->second.effect_scope!=scope||source->second.precise_health<=0)return {};
        const auto final_origin=shadow_position_locked(source->second,now);
        if(!final_origin||final_origin->point.x!=origin->point.x||final_origin->point.z!=origin->point.z||
           final_origin->surface.plane!=origin->surface.plane||final_origin->surface.cell!=origin->surface.cell)return {};
        auto bundle=prepare_bundle_drop_locked(scope,now,bundles::Cause::shadow_step,*origin);if(!bundle)return {};
        if(!corpse_reservations_.reserve_exploitation(claim->corpse.scope,scope,97,attempt))return {};
        DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        if(!commit_bundle_drop_locked(*bundle)){
            corpse_reservations_.release(claim->corpse.scope,{scope,0,0,97,attempt});return {};
        }
        if(!corpse_reservations_.consume_exploitation(claim->corpse.scope,scope,97,attempt))throw std::logic_error("reserved Necrotic corpse lost");
        apply_shadow_position_locked(source->second,step->end,now);
        NecroticTraversalReceipt receipt{*claim,*origin,step->end,{},bundle->returned};
        for(const auto target:player_targets){
            const auto p=players_.find(target.actor);if(p==players_.end()||p->second.effect_scope!=target||p->second.precise_health<=0)continue;
            auto conditions=p->second.conditions;if(!conditions.apply(Kind::poison,scope.actor,now,poison_durations.at(target)))throw std::logic_error("admitted Necrotic Poison failed");
            commit_conditions_locked(p->second,conditions,{Kind::poison},now);receipt.condition_recipients.push_back(target);
        }
        for(const auto target:npc_targets){
            const auto live=npc_combat_.find(target.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=target)continue;
            const auto life=live->second.lifetime();const auto* actor=live->second.actor(life);if(!actor||!actor->health||actor->health->current<=0)continue;
            if(!live->second.apply_condition(life,Kind::poison,scope.actor,now,poison_durations.at(target)))throw std::logic_error("admitted NPC Necrotic Poison failed");
            npc_condition_credit_[{target.actor,scope.actor}]={target,scope};
            for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(target.actor,live->second.actor(life)->effects)});
            publish_npc_fragility_locked(target.actor,live->second);commit_applications_locked(target,now);receipt.condition_recipients.push_back(target);
        }
        // Preserve the observed completion-frame cue: new Poison status before relocation feedback.
        for(auto& [id,peer]:players_)peer.outbox->enqueue({position_packet(scope.actor,step->end.point.x,step->end.point.z,static_cast<std::uint16_t>(step->end.surface.plane)),stop_packet(scope.actor)});
        dispatch_fevered_boundary_locked(now);
        if(publication)*publication=finish_damage_publication_locked(pending);
        return receipt;
    }
