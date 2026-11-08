// Whole-cohort elemental admission; actual hits never replay preflight Health.
private:
    bool elemental_impact_locked(Player& caster,std::uint32_t target,const skills::effects::ElementalPlan& plan,unsigned level,
            float center_x,float center_y,std::uint16_t plane,std::uint64_t now,std::vector<Packet>* visuals=nullptr,
            std::optional<skills::sacrifice::Loss>* paid_sacrifice=nullptr,skills::effects::ElementalOutcome* outcome=nullptr,
            std::optional<std::uint64_t> completion_event={},std::optional<skills::effects::SpiritProximity> essence_spirits={}){
        using namespace skills::effects;
        if(plan.skill==1068&&!outcome)return false;
        const auto source=caster.effect_scope;const auto team=caster.team_token;
        const auto caster_at=[&]()->Player*{const auto p=players_.find(source.actor);return p!=players_.end()&&p->second.effect_scope==source?&p->second:nullptr;};
        const auto player_at=[&](Scope owner)->Player*{const auto p=players_.find(owner.actor);return p!=players_.end()&&p->second.effect_scope==owner?&p->second:nullptr;};
        float radius=plan.radius;
        if(plan.area_if_target_active){
            std::optional<bool> spreads;
            if(const auto p=players_.find(target);p!=players_.end()){
                auto& victim=p->second;if(!victim.resources||!advance_health_locked(victim,now)||victim.precise_health<=0)return false;
                std::optional<skills::Slot> skill;
                if(victim.casting_skill&&victim.casting_begins<=now&&now<victim.casting_ends)skill=skills::Slot{victim.casting_skill,victim.casting_copy};
                spreads=liquid_flame_spreads(skill,victim.ordinary_animation||now<victim.melee_skill_until);
            }else{
                auto live=npc_combat_.find(target);if(live==npc_combat_.end())return false;const auto expected=live->second.hex_scope();
                advance_npc_conditions_locked(target,live->second,now);live=npc_combat_.find(target);
                if(live==npc_combat_.end()||live->second.hex_scope()!=expected)return false;
                const auto life=live->second.lifetime();const auto actor=live->second.actor(life);
                if(!actor||!actor->health||actor->health->current<=0)return false;
                const auto action=live->second.active_action(life,now);
                spreads=action?liquid_flame_spreads(action->skill.value_or(skills::Slot{}),action->kind==combat::npcs::control::Action::attack):std::optional(false);
            }
            if(!spreads)return false;if(!*spreads)radius=0;
        }
        std::vector<Scope> recipients,npc_recipients;
        for(auto& [id,p]:players_){
            if(!p.team_token||p.team_token==team||!p.resources||p.precise_health<=0||p.surface.plane!=plane)continue;
            if(!radius&&id!=target)continue;const auto position=healing_position_locked(p,now);if(!position)return false;
            if(std::hypot(position->x-center_x,position->z-center_y)>radius&&id!=target)continue;
            if(now<p.health_updated||p.next_health_effect>UINT32_MAX-2||!advance_health_locked(p,now))return false;
            if(p.precise_health>0)recipients.push_back(p.effect_scope);
        }
        std::vector<Scope> npc_scopes;for(const auto& [id,live]:npc_combat_)npc_scopes.push_back(live.hex_scope());
        for(const auto owner:npc_scopes){
            auto live=npc_combat_.find(owner.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=owner||!live->second.hostile_to_players(live->second.lifetime()))continue;
            refresh_npc_movement_locked(owner.actor,now);auto placed=npcs_.find(owner.actor);
            if(placed==npcs_.end()||placed->second.plane!=plane||(!radius&&owner.actor!=target))continue;
            if(std::hypot(placed->second.x-center_x,placed->second.y-center_y)>radius&&owner.actor!=target)continue;
            if(now<live->second.health_clock(live->second.lifetime()).value_or(now))return false;
            advance_npc_conditions_locked(owner.actor,live->second,now);live=npc_combat_.find(owner.actor);
            if(live==npc_combat_.end()||live->second.hex_scope()!=owner)continue;
            const auto actor=live->second.actor(live->second.lifetime());
            if(actor&&actor->health&&actor->health->current>0&&actor->damageable)npc_recipients.push_back(owner);
        }
        if(plan.skill==1227||plan.skill==919){
            if(!essence_spirits||*essence_spirits==SpiritProximity::unknown)return false;
            // Recipient settlement may retire a witness after the entry census.
            essence_spirits=spirit_light_proximity_locked(source,now);
            if(*essence_spirits==SpiritProximity::unknown)return false;
        }
        const auto conditions_at=[&](Player& p,const std::vector<combat::conditions::Kind>& additions){
            auto conditions=p.conditions;std::vector<combat::conditions::Kind> applied;
            for(const auto kind:additions){
                if(kind==combat::conditions::Kind::burning&&burning_immune_locked(p,now))continue;
                const auto duration=kind==combat::conditions::Kind::burning?plan.burning_ms:plan.blind_if_burning_ms;
                if(!duration||!conditions.apply(kind,source.actor,now,duration))return std::optional<std::pair<combat::conditions::State,std::vector<combat::conditions::Kind>>>{};
                applied.push_back(kind);
            }
            return std::optional(std::pair{std::move(conditions),std::move(applied)});
        };
        // Interrupt protection can settle Energy reactions. Do this before the
        // discardable cohort projection; never quote it on a copied Player.
        for(const auto owner:recipients)if(auto* p=player_at(owner);p&&plan.interrupt_if_burning&&p->conditions.has(combat::conditions::Kind::burning,now)&&
            (p->casting_skill&&p->casting_begins<=now&&now<p->casting_ends||p->ordinary_animation||now<p->melee_skill_until)){
            if(p->control_epoch==UINT64_MAX||p->attack_interrupt_epoch==UINT64_MAX||!preflight_interrupt_protection_locked(*p,now))return false;
        }
        for(const auto owner:npc_recipients){
            auto n=npc_combat_.find(owner.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=owner)continue;
            auto preview=n->second;auto result=preview.apply_elemental(preview.lifetime(),plan,level,source.actor,now,true,essence_spirits.value_or(SpiritProximity::unknown));
            if(!result||!validate_npc_reversal_receipts_locked(owner,result->reversals))return false;
            if(result->interrupt&&result->actor.health->current>0&&n->second.active_action(n->second.lifetime(),now)){
                const auto prevented=prevent_npc_interrupt_locked(n->second,now,false);if(!prevented)return false;
                if(!*prevented&&!preview.cancel_action(preview.lifetime(),now))return false;
            }
        }
        std::map<Scope,HealingSeedReaction> facts;bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            auto* own=caster_at();if(!own||!own->resources)return false;
            if(plan.self_bleeding_ms){
                if(!paid_sacrifice||!resurrection_clock_valid(*own,now)||own->next_health_effect>UINT32_MAX-2)return false;
                const auto loss=skills::sacrifice::resolve(plan.skill,own->precise_health,own->resources->max_health);if(!loss)return false;
                auto* state=admission_player_locked(batch,*own,now);if(!state)return false;
                state->health.capacity.event(state->health.current,loss->remaining-state->health.current,false);
                if(state->health.current>0){auto condition=own->conditions;
                    if(!condition.apply(combat::conditions::Kind::bleeding,source.actor,now,plan.self_bleeding_ms))return false;
                    valid=project_batch_condition_reactions_locked(batch,*own,own->conditions,condition,now);
                }
            }
            for(const auto owner:recipients){
                if(!valid)break;auto* p=player_at(owner);if(!p||!p->resources||p->precise_health<=0)continue;
                auto* state=admission_player_locked(batch,*p,now);if(!state)return false;if(state->health.current<=0)continue;
                auto actor=incoming_spell_actor_locked(*p,{p->id,owner.generation,combat::Health{state->health.current,state->health.capacity.maximum()},{},p->effects,true,
                    p->reviewed_lightning_armor,p->reviewed_fire_armor,p->reviewed_cold_armor},now);
                const combat::DamageProjection projection=[&](combat::Actor& current,const combat::Damage& damage){return project_batch_player_damage_locked(batch,*p,current,damage,now);};
                const auto result=elemental_target(plan,actor,p->conditions,level,source.actor,now,
                    water_hex(state->registry,owner,now),1,true,state->registry.has(owner,Category::enchantment,now),projection,essence_spirits.value_or(SpiritProximity::unknown));
                if(!result.accepted){valid=false;break;}
                if(state->health.current>0){const auto condition=conditions_at(*p,result.additions);if(!condition)return false;
                    if(!project_batch_condition_reactions_locked(batch,*p,p->conditions,condition->first,now)){valid=false;break;}}
            }
            if(batch.missing_seed){const auto owner=*batch.missing_seed;auto* p=player_at(owner);if(!p)return false;
                const auto fact=prepare_healing_seed_direct_locked(*p,1.,now);if(!fact)return false;facts[owner]=*fact;continue;}
            if(!valid)return false;
            for(const auto owner:npc_recipients){
                const auto n=npc_combat_.find(owner.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=owner)continue;
                auto preview=batch.npcs.contains(owner)?batch.npcs.at(owner):n->second;
                const auto result=preview.apply_elemental(preview.lifetime(),plan,level,source.actor,now,true,essence_spirits.value_or(SpiritProximity::unknown));
                if(!result||!validate_npc_reversal_receipts_locked(owner,result->reversals))return false;
                for(const auto& event:preview.take_fragility_hits(preview.lifetime()))if(event.reversal&&!validate_npc_reversal_receipts_locked(owner,{event.reversal}))return false;
            }
            admitted=true;break;
        }
        if(!admitted)return false;
        auto* own=caster_at();if(!own)return false;
        if(completion_event&&!complete_ether_renewal_locked(source,plan.skill,*completion_event,now,true))return false;
        own=caster_at();if(!own||own->precise_health<=0)return true;
        if(plan.self_bleeding_ms){
            auto loss=skills::sacrifice::resolve(plan.skill,own->precise_health,own->resources->max_health);if(!loss)return false;
            health_event_locked(*own,loss->remaining-own->precise_health,now);loss->remaining=own->precise_health;loss->killed=own->precise_health<=0;*paid_sacrifice=loss;
            if(own->passive_recovery)own->passive_recovery->record(recovery::Event::health_loss,now);
            if(loss->killed)kill_locked(*own,now);else{
                publish_health_locked(*own);own->outbox->enqueue(owner_vitals(*own));auto condition=own->conditions;
                if(!condition.apply(combat::conditions::Kind::bleeding,source.actor,now,plan.self_bleeding_ms))throw std::logic_error("admitted Agony Bleeding failed");
                commit_conditions_locked(*own,condition,{combat::conditions::Kind::bleeding},now);
            }
            dispatch_fevered_boundary_locked(now);
        }
        unsigned energy=0;ElementalOutcome settled_outcome;
        const auto body=[&](Scope owner){
            if(plan.self_bleeding_ms||plan.skill==866||plan.skill==1068||plan.skill==1081||plan.skill==845||plan.skill==1227||plan.skill==919||plan.skill==189)
                publish_target_body_locked(owner.actor,plan.skill,source.actor,visuals);
            else if(const auto particle=skills::particles::target_body_2022(owner.actor,plan.skill))for(auto& [id,peer]:players_)peer.outbox->enqueue({*particle});
        };
        for(const auto owner:recipients){
            auto* p=player_at(owner);if(!p||!p->resources||p->precise_health<=0)continue;
            if(!visuals)body(owner);
            auto actor=incoming_spell_actor_locked(*p,{p->id,owner.generation,combat::Health{p->precise_health,p->resources->max_health},{},p->effects,true,
                p->reviewed_lightning_armor,p->reviewed_fire_armor,p->reviewed_cold_armor},now);
            const combat::DamageProjection apply=[&](combat::Actor& current,const combat::Damage& damage)->std::optional<combat::DamageStep>{
                auto* live=player_at(owner);if(!live||!live->resources||live->precise_health<=0)return {};
                auto fresh=incoming_spell_actor_locked(*live,{live->id,owner.generation,combat::Health{live->precise_health,live->resources->max_health},{},live->effects,true,
                    live->reviewed_lightning_armor,live->reviewed_fire_armor,live->reviewed_cold_armor},now,true);
                const auto incoming=combat::apply(fresh,damage,true);if(incoming.reason!=combat::Reason::accepted)return {};
                if(plan.skill==1068&&owner.actor==target){const auto caster=caster_at();if(!caster)return {};settled_outcome.instant_recharge=incoming.before.current>caster->precise_health;}
                std::optional<combat::Hit> actual;
                if(!commit_player_reactive_hit_locked(*live,incoming,source.actor,now,false,nullptr,false,true,&actual,source))return {};
                live=player_at(owner);if(!live||!live->resources)return {};
                current.health=combat::Health{live->precise_health,live->resources->max_health};current.effects=live->effects;
                return combat::DamageStep{incoming,actual};
            };
            const auto result=elemental_target(plan,actor,p->conditions,level,source.actor,now,water_hex(*p->effect_registry,owner,now),1,true,
                p->effect_registry->has(owner,Category::enchantment,now),apply,essence_spirits.value_or(SpiritProximity::unknown));
            if(!result.accepted)throw std::logic_error("admitted elemental player failed");energy+=result.energy;
            if(visuals)body(owner);
            p=player_at(owner);if(!p||!p->resources||p->precise_health<=0)continue;
            if(p->passive_recovery)p->passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            const auto condition=conditions_at(*p,result.additions);if(!condition)throw std::logic_error("admitted elemental conditions failed");
            if(p->next_health_effect>UINT32_MAX-condition->second.size())throw std::logic_error("elemental condition identity exhausted");
            if(!condition->second.empty())commit_conditions_locked(*p,condition->first,condition->second,now);
            p=player_at(owner);if(p&&p->precise_health>0&&result.interrupt&&
                (p->casting_skill&&p->casting_begins<=now&&now<p->casting_ends||p->ordinary_animation||now<p->melee_skill_until)&&!prevent_interrupt_locked(*p,now)){
                record_control(*p,now);++p->attack_interrupt_epoch;p->casting_skill=0;p->melee_skill_until=0;cancel_ordinary_locked(*p,combat::weapon::Cancel::explicit_input);
            }
        }
        for(const auto owner:npc_recipients){
            auto n=npc_combat_.find(owner.actor);auto placed=npcs_.find(owner.actor);
            if(n==npc_combat_.end()||placed==npcs_.end()||n->second.hex_scope()!=owner)continue;
            const auto initial=n->second.actor(n->second.lifetime());if(!initial||!initial->health||initial->health->current<=0)continue;
            if(!visuals)publish_target_body_locked(owner.actor,plan.skill,source.actor);
            own=caster_at();if(!own)continue;auto proposed=n->second;
            const auto result=proposed.apply_elemental(proposed.lifetime(),plan,level,source.actor,now,true,essence_spirits.value_or(SpiritProximity::unknown));
            if(!result||!validate_npc_reversal_receipts_locked(owner,result->reversals))throw std::logic_error("admitted elemental NPC failed");
            if(plan.skill==1068&&owner.actor==target&&!result->hits.empty())settled_outcome.instant_recharge=result->hits.front().before.current>own->precise_health;
            n->second=std::move(proposed);energy+=result->energy;publish_npc_elemental_hits_locked(*own,n->second,*result,now);
            n=npc_combat_.find(owner.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=owner)continue;
            if(result->interrupt){const auto life=n->second.lifetime();const auto actor=n->second.actor(life);
                if(actor&&actor->health&&actor->health->current>0)if(const auto action=n->second.active_action(life,now)){
                    const auto prevented=prevent_npc_interrupt_locked(n->second,now,true);
                    if(!prevented)throw std::logic_error("admitted NPC interrupt failed");
                    n=npc_combat_.find(owner.actor);
                    if(n==npc_combat_.end()||n->second.hex_scope()!=owner)continue;
                    const auto recipient=n->second.actor(life);
                    if(!recipient||!recipient->health||recipient->health->current<=0)continue;
                    if(!*prevented&&n->second.active_action(life,now)==action){
                        if(!n->second.cancel_action(life,now))throw std::logic_error("admitted NPC interrupt cancel failed");
                        const auto packet=action->kind==combat::npcs::control::Action::attack?combat::weapon::stop_2022(owner.actor):combat::detail::integer(59,owner.actor,0);
                        for(auto& [id,peer]:players_)peer.outbox->enqueue({packet});
                    }
                }
            }
            n=npc_combat_.find(owner.actor);if(n==npc_combat_.end()||n->second.hex_scope()!=owner)continue;
            auto& live=n->second;const auto life=live.lifetime();const auto actor=live.actor(life);if(!actor||!actor->health)continue;
            if(visuals)publish_target_body_locked(owner.actor,plan.skill,source.actor,visuals);
            if(!result->additions.empty()){npc_condition_credit_[{owner.actor,source.actor}]={owner,source};for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(owner.actor,live.actor(life)->effects)});}
            if(live.actor(life)->health->current<=0){own=caster_at();placed=npcs_.find(owner.actor);if(own&&placed!=npcs_.end())record_npc_death_locked(*own,placed->second,live,now);}
            else{const bool reacted=publish_npc_fragility_locked(owner.actor,live);n=npc_combat_.find(owner.actor);
                if(!reacted&&n!=npc_combat_.end()&&n->second.hex_scope()==owner&&!result->additions.empty())commit_applications_locked(owner,now);}
        }
        if(essence_spirits==SpiritProximity::present)energy+=plan.energy_if_spirit;
        own=caster_at();if(energy&&own&&own->resources&&own->precise_health>0){gain_energy_locked(*own,energy,now);own->outbox->enqueue(owner_vitals(*own));}
        if(outcome)*outcome=settled_outcome;return true;
    }
