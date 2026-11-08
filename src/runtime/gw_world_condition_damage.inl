public:
    bool condition_damage(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::ConditionDamagePlan& plan,unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr,std::function<void()>* publication=nullptr,std::optional<std::uint64_t> completion_event={}){
        if(visuals)visuals->clear();if(publication)*publication={};if(publication&&!visuals)return false;
        if(!skills::effects::valid_condition_damage(plan)||!level||level>255||now>UINT64_MAX-plan.duration)return false;
        std::lock_guard lock(mutex_);
        const bool accumulated=bool(skills::effects::accumulated_pain_damage(plan.skill,plan.rank));
        unsigned admitted_skill=plan.skill;
        if(completion_event){
            if(!accumulated)return false;
            const auto activation=spell_activation_cursors_.find(scope.actor);
            // The retained1052slot produces its reviewed3184PvP plan.
            if(plan.skill==3184&&activation!=spell_activation_cursors_.end()&&activation->second.actor==scope&&activation->second.skill==1052)admitted_skill=1052;
            if(!complete_ether_renewal_locked(scope,admitted_skill,*completion_event,now,true,false))return false;
        }
        const std::vector<combat::conditions::Kind> followups=accumulated?
            std::vector{combat::conditions::Kind::deep_wound}:
            std::vector{combat::conditions::Kind::poison,combat::conditions::Kind::weakness};
        DamagePublication pending;if(publication&&!completion_event)begin_damage_publication_locked(pending);
        auto perform_condition_batch=[&]()->bool{
            const auto a=players_.find(scope.actor),b=players_.find(target.actor);
            if(a==players_.end()||a->second.effect_scope!=scope||!a->second.team_token||!a->second.resources||
               a->second.precise_health<=0||a->second.knocked_down_until>now||now<a->second.health_updated)return false;
            std::optional<navigation::Point> center;unsigned plane=0;
            if(b!=players_.end()){
                if(b->second.effect_scope!=target||!b->second.team_token||a->second.team_token==b->second.team_token||
                   !b->second.resources||b->second.precise_health<=0||now<b->second.health_updated)return false;
                center=healing_position_locked(b->second,now);plane=b->second.surface.plane;
            }else{
                const auto placed=npcs_.find(target.actor);const auto live=npc_combat_.find(target.actor);
                if(placed==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=target||
                   !live->second.hostile_to_players(live->second.lifetime()))return false;
                refresh_npc_movement_locked(target.actor,now);
                const auto current=npc_combat_.find(target.actor);const auto body=npcs_.find(target.actor);
                if(current==npc_combat_.end()||body==npcs_.end()||current->second.hex_scope()!=target)return false;
                center=navigation::Point{body->second.x,body->second.y};plane=body->second.plane;
            }
            const auto origin=healing_position_locked(a->second,now);
            if(!origin||!center||a->second.surface.plane!=plane||std::hypot(origin->x-center->x,origin->z-center->z)>plan.range)return false;
            if(accumulated&&navigation_){const auto line=layered_->trace(a->second.surface,*origin,*center);
                if(!line||line->plane!=plane)return false;}
            std::vector<Player*> recipients;
            for(auto&[id,p]:players_){
                if(!p.resources||p.precise_health<=0||!p.team_token||p.team_token==a->second.team_token||p.surface.plane!=plane)continue;
                if(!plan.radius&&id!=target.actor)continue;const auto point=healing_position_locked(p,now);if(!point)return false;
                if(id!=target.actor&&std::hypot(point->x-center->x,point->z-center->z)>plan.radius)continue;
                if(now<p.health_updated||p.next_health_effect>UINT32_MAX-2||(plan.kind==combat::Kind::cold&&!p.reviewed_cold_armor))return false;recipients.push_back(&p);
            }
            if(!advance_health_locked(a->second,now)||a->second.precise_health<=0)return false;
            struct Candidate{Player* p;skills::effects::Scope scope;bool conditioned;};
            std::vector<Candidate> candidates;
            const combat::Damage damage{plan.kind,double(plan.damage),level,plan.skill};
            std::map<skills::effects::Scope,skills::effects::HealingSeedReaction> facts;bool admitted=false;
            for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
                DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;candidates.clear();
                for(auto* ptr:recipients){auto& p=*ptr;if(!advance_health_locked(p,now))return false;if(p.precise_health<=0)continue;
                    auto* state=admission_player_locked(batch,p,now);if(!state)return false;if(state->health.current<=0)continue;
                    const bool conditioned=accumulated?state->registry.count(p.effect_scope,skills::effects::Category::hex,now)>=2:p.conditions.count(now)!=0;
                    auto actor=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
                        combat::Health{state->health.current,state->health.capacity.maximum()},{},p.effects,true,{},{},p.reviewed_cold_armor},now,true);
                    const auto step=project_batch_player_damage_locked(batch,p,actor,damage,now);
                    if(!step){valid=false;break;}
                    if(conditioned&&state->health.current>0){
                        if(plan.skill==828){
                            // Hex Breaker includes its caster damage/Seed admission
                            // before this spell's initial victim damage can commit.
                            const auto prevented=p.id==target.actor?prevent_selected_hex_locked(a->second,p,plan.skill,now,false):std::optional(false);
                            if(!prevented)return false;
                            if(!*prevented){auto registry=state->registry;auto regeneration=*p.health_regeneration;
                                const skills::effects::Entry hex{static_cast<std::uint32_t>(p.next_health_effect),scope.actor,plan.skill,plan.rank,skills::effects::Category::hex,now,now+plan.duration};
                                if(!registry.apply(p.effect_scope,hex,skills::effects::Replace::same_skill).accepted||
                                   !regeneration.apply(p.effect_scope,{hex.instance,scope.actor,plan.skill,-static_cast<int>(plan.pips),now,now+plan.duration}))return false;
                            }
                        }else{auto conditions=p.conditions;
                            for(const auto kind:followups)if(!conditions.apply(kind,scope.actor,now,plan.duration))return false;
                            if(accumulated)state->health.capacity.capacity(state->health.current,state->health.capacity.base,true);
                            if(!project_batch_condition_reactions_locked(batch,p,p.conditions,conditions,now)){valid=false;break;}
                        }
                    }
                    candidates.push_back({&p,p.effect_scope,conditioned});
                }
                if(batch.missing_seed){const auto owner=*batch.missing_seed;const auto player=players_.find(owner.actor);
                    if(player==players_.end()||player->second.effect_scope!=owner)return false;
                    const auto fact=prepare_healing_seed_direct_locked(player->second,1.,now);if(!fact)return false;facts[owner]=*fact;continue;}
                if(!valid)return false;admitted=true;break;
            }
            if(!admitted)return false;
            struct NpcCandidate{skills::effects::Scope scope;bool conditioned;};std::vector<NpcCandidate> npc_candidates;
            std::vector<skills::effects::Scope> npc_scopes;for(const auto& [id,live]:npc_combat_)npc_scopes.push_back(live.hex_scope());
            for(const auto expected:npc_scopes){
                const auto id=expected.actor;auto current=npc_combat_.find(id);
                if(current==npc_combat_.end()||current->second.hex_scope()!=expected)continue;
                const auto placed=npcs_.find(id);const auto life=current->second.lifetime();auto& initial=current->second;
                if(placed==npcs_.end()||placed->second.plane!=plane||!initial.hostile_to_players(life)||(!plan.radius&&id!=target.actor))continue;
                refresh_npc_movement_locked(id,now);
                if(id!=target.actor&&std::hypot(placed->second.x-center->x,placed->second.y-center->z)>plan.radius)continue;
                if(now<initial.health_clock(life).value_or(now))return false;
                advance_npc_conditions_locked(id,initial,now);current=npc_combat_.find(id);
                if(current==npc_combat_.end()||current->second.hex_scope()!=expected)continue;
                auto& live=current->second;const auto* actor=live.actor(life);
                if(!actor||!actor->health)return false;
                if(!actor->damageable){if(id==target.actor)return false;continue;}
                if(actor->health->current<=0)continue;
                const auto conditions=live.condition_snapshot(life);if(!conditions)return false;
                const auto registry=live.hex_registry_snapshot(life,now);if(!registry)return false;
                const bool conditioned=accumulated?registry->count(expected,skills::effects::Category::hex,now)>=2:conditions->count(now)>0;auto preview=live;
                if(!preflight_npc_reversal_or_damage_locked(preview,damage,now))return false;
                if(conditioned&&preview.actor(life)->health->current>0){
                    if(plan.skill==828){if(next_npc_hex_>=UINT32_MAX||!preview.apply_vile_miasma(life,plan,scope.actor,next_npc_hex_,now))return false;}
                    else if(preview.allows_conditions(life))for(const auto kind:followups)
                        if(!preview.apply_condition(life,kind,scope.actor,now,plan.duration))return false;
                }
                for(const auto& event:preview.take_fragility_hits(life))if(event.reversal&&!validate_npc_reversal_receipts_locked(expected,{event.reversal}))return false;
                npc_candidates.push_back({expected,conditioned});
            }
            if(std::none_of(candidates.begin(),candidates.end(),[&](const auto& c){return c.scope==target;})&&
               std::none_of(npc_candidates.begin(),npc_candidates.end(),[&](const auto& c){return c.scope==target;}))return false;
            if(completion_event){
                if(!complete_ether_renewal_locked(scope,admitted_skill,*completion_event,now,true))return false;
                if(a->second.precise_health<=0)return true;
                // Source rewards precede the native finish. Only the ensuing
                // recipient damage/conditions are deferred behind that prefix.
                if(publication)begin_damage_publication_locked(pending);
            }
            if(a->second.passive_recovery)a->second.passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
            for(const auto& c:candidates){auto& p=*c.p;
                if(p.effect_scope!=c.scope||!p.resources||p.precise_health<=0)continue;
                const bool conditioned=accumulated?p.effect_registry->count(p.effect_scope,skills::effects::Category::hex,now)>=2:c.conditioned;
                if(publication)publish_target_body_locked(p.id,plan.skill,accumulated?scope.actor:0);
                auto actor=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
                    combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,{},{},p.reviewed_cold_armor},now,true);
                const auto hit=combat::apply(actor,damage,true);
                if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(p,hit,scope.actor,now,false,nullptr,false,true,nullptr,scope))
                    throw std::logic_error("preflighted conditional damage failed");
                if(!publication)publish_target_body_locked(p.id,plan.skill,accumulated?scope.actor:0);
                if(p.passive_recovery)p.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
                if(p.precise_health<=0||!conditioned)continue;
                if(plan.skill==828){
                    const auto prevention=prevent_selected_hex_locked(a->second,p,plan.skill,now);
                    if(!prevention)throw std::logic_error("validated damaging hex prevention failed");
                    if(*prevention)continue;
                    if(p.next_health_effect>UINT32_MAX)throw std::logic_error("conditional hex effect space exhausted after damage reactions");
                    const skills::effects::Entry hex{static_cast<std::uint32_t>(p.next_health_effect),scope.actor,plan.skill,plan.rank,skills::effects::Category::hex,now,now+plan.duration};
                    // Copy CURRENT authorities only after damage/end callbacks.
                    // Never resurrect an enchantment consumed by conversion.
                    auto registry=*p.effect_registry;auto regeneration=*p.health_regeneration;
                    const auto replaced=registry.apply(p.effect_scope,hex,skills::effects::Replace::same_skill);
                    if(!replaced.accepted||!regeneration.apply(p.effect_scope,{hex.instance,scope.actor,plan.skill,-static_cast<int>(plan.pips),now,now+plan.duration}))
                        throw std::logic_error("preflighted conditional hex application failed");
                    *p.effect_registry=std::move(registry);*p.health_regeneration=std::move(regeneration);++p.next_health_effect;
                    std::vector<Packet> packets;for(const auto& old:replaced.removed){p.health_effects.erase(old.instance);packets.push_back(health_effect_remove(p.id,old.instance));}
                    p.health_effects.emplace(hex.instance,hex);packets.push_back(health_effect_add(hex,p.id,now));
                    for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);commit_applications_locked(p.effect_scope,now);publish_status_locked(p,now);
                }else{auto conditions=p.conditions;
                    if(p.next_health_effect>UINT32_MAX-2)throw std::logic_error("conditional condition identity exhausted");
                    for(const auto kind:followups)if(!conditions.apply(kind,scope.actor,now,plan.duration))
                        throw std::logic_error("preflighted conditional conditions failed");
                    commit_conditions_locked(p,conditions,followups,now);
                }
            }
            for(const auto& c:npc_candidates){
                auto found=npc_combat_.find(c.scope.actor);const auto placed=npcs_.find(c.scope.actor);
                if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=c.scope)continue;
                const auto life=found->second.lifetime();const auto actor=found->second.actor(life);
                if(!actor||!actor->health||actor->health->current<=0)continue;
                const auto registry=found->second.hex_registry_snapshot(life,now);if(!registry)continue;
                const bool conditioned=accumulated?registry->count(c.scope,skills::effects::Category::hex,now)>=2:c.conditioned;
                if(publication)publish_target_body_locked(c.scope.actor,plan.skill,scope.actor);
                if(!commit_npc_reversal_or_damage_locked(a->second,placed->second,found->second,damage,now))
                    throw std::logic_error("preflighted NPC conditional damage failed");
                if(!publication)publish_target_body_locked(c.scope.actor,plan.skill,scope.actor);
                found=npc_combat_.find(c.scope.actor);if(found==npc_combat_.end()||found->second.hex_scope()!=c.scope)continue;
                auto& live=found->second;
                if(!live.actor(life)->health||live.actor(life)->health->current<=0||!conditioned)continue;
                if(plan.skill==828){
                    if(next_npc_hex_>=UINT32_MAX)throw std::logic_error("NPC conditional hex effect space exhausted");
                    const auto effect=next_npc_hex_;const auto applied=live.apply_vile_miasma(life,plan,scope.actor,effect,now);
                    if(!applied)throw std::logic_error("preflighted NPC Vile Miasma failed");++next_npc_hex_;
                    npc_condition_credit_[{c.scope.actor,scope.actor}]={c.scope,scope};
                    std::vector<Packet> packets;for(const auto& old:applied->removed)packets.push_back(health_effect_remove(c.scope.actor,old.instance));
                    const auto entry=live.hex_effect(life,effect);if(!entry)throw std::logic_error("NPC Vile Miasma entry missing");
                    packets.push_back(health_effect_add(*entry,c.scope.actor,now));packets.push_back(emotes::effects_2022(c.scope.actor,live.actor(life)->effects));
                    for(auto&[id,peer]:players_)peer.outbox->enqueue(packets);commit_applications_locked(c.scope,now);
                }else if(live.allows_conditions(life)){
                    auto updated=live;
                    for(const auto kind:followups)if(!updated.apply_condition(life,kind,scope.actor,now,plan.duration))
                        throw std::logic_error("preflighted NPC conditional damage condition failed");
                    live=std::move(updated);npc_condition_credit_[{c.scope.actor,scope.actor}]={c.scope,scope};
                    for(auto&[id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(c.scope.actor,live.actor(life)->effects)});
                    const bool killed=publish_npc_fragility_locked(c.scope.actor,live);
                    found=npc_combat_.find(c.scope.actor);
                    if(!killed&&found!=npc_combat_.end()&&found->second.hex_scope()==c.scope)commit_applications_locked(c.scope,now);
                }
            }
            return true;
        };
        const bool accepted=perform_condition_batch();
        dispatch_fevered_boundary_locked(now);
        auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();return accepted;
    }
