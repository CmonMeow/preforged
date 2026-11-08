// Ordered mixed-recipient Mind Burn admission and actual feedback.
public:
    bool mind_burn(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::MindBurnPlan& plan,
            unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr,std::function<void()>* publication=nullptr){
        using namespace skills::effects;
        if(visuals)visuals->clear();if(publication)*publication={};if(publication&&!visuals)return false;
        const auto canonical=skills::effects::mind_burn(plan.skill,plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255||now>UINT64_MAX-plan.burning_ms)return false;
        std::lock_guard lock(mutex_);
        DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        const auto execute=[&]()->bool{
        const auto caster_at=[&]()->Player*{const auto p=players_.find(scope.actor);return p!=players_.end()&&p->second.effect_scope==scope?&p->second:nullptr;};
        auto* caster=caster_at();if(!caster||!caster->resources||!advance_resource_event_locked(*caster,now)||caster->precise_health<=0||caster->knocked_down_until>now)return false;
        const auto energy=[&](const Player& p)->std::optional<std::int64_t>{const auto e=mind_wrack_energy_locked(p);
            if(!valid_mind_wrack_energy(e))return {};return (std::int64_t(e.current)-e.debt)*300000+e.fractional_units;};
        const auto team=caster->team_token;navigation::Point center{};std::uint16_t plane=0;
        if(const auto p=players_.find(target.actor);p!=players_.end()){
            if(p->second.effect_scope!=target||!p->second.resources||p->second.precise_health<=0||!team||!p->second.team_token||team==p->second.team_token)return false;
            const auto pos=healing_position_locked(p->second,now);if(!pos)return false;center=*pos;plane=p->second.surface.plane;
        }else{
            auto live=npc_combat_.find(target.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=target||!live->second.hostile_to_players(live->second.lifetime()))return false;
            refresh_npc_movement_locked(target.actor,now);const auto body=npcs_.find(target.actor);if(body==npcs_.end())return false;center={body->second.x,body->second.y};plane=body->second.plane;
        }
        const auto origin=healing_position_locked(*caster,now);
        if(!origin||caster->surface.plane!=plane||std::hypot(origin->x-center.x,origin->z-center.z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster->surface,*origin,center);if(!line||line->plane!=plane)return false;}
        struct Recipient{Scope scope;bool bonus=false;};std::vector<Recipient> players,npcs;bool selected=false;
        for(auto& [id,p]:players_){
            if(!p.resources||p.precise_health<=0||!team||!p.team_token||p.team_token==team||p.surface.plane!=plane)continue;
            const auto pos=healing_position_locked(p,now);if(!pos)return false;if(std::hypot(pos->x-center.x,pos->z-center.z)>plan.radius)continue;
            if(!advance_resource_event_locked(p,now))return false;if(p.precise_health<=0)continue;
            players.push_back({p.effect_scope});selected|=p.effect_scope==target;
        }
        std::vector<unsigned> ids;for(const auto& [id,n]:npcs_)ids.push_back(id);
        for(const auto id:ids){
            auto live=npc_combat_.find(id);if(live==npc_combat_.end()||!live->second.hostile_to_players(live->second.lifetime()))continue;
            refresh_npc_movement_locked(id,now);auto body=npcs_.find(id);if(body==npcs_.end()||body->second.plane!=plane||std::hypot(body->second.x-center.x,body->second.y-center.z)>plan.radius)continue;
            live=npc_combat_.find(id);if(live==npc_combat_.end())continue;const auto owner=live->second.hex_scope();
            if(now<live->second.health_clock(live->second.lifetime()).value_or(now))return false;
            advance_npc_conditions_locked(id,live->second,now);live=npc_combat_.find(id);if(live==npc_combat_.end()||live->second.hex_scope()!=owner)continue;
            const auto actor=live->second.actor(live->second.lifetime());if(!actor||!actor->damageable||!actor->health||actor->health->current<=0)continue;
            if(!live->second.advance_energy(live->second.lifetime(),now))return false;npcs.push_back({owner});selected|=owner==target;
        }
        if(!selected)return false;
        const combat::Damage damage{combat::Kind::fire,plan.damage,level,plan.skill};
        std::map<Scope,HealingSeedReaction> facts;bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            // Settle every participant before quoting either side. Seed fact
            // discovery may have advanced real timelines on the preceding pass.
            for(const auto& recipient:players){const auto p=players_.find(recipient.scope.actor);
                if(p!=players_.end()&&p->second.effect_scope==recipient.scope&&!advance_resource_event_locked(p->second,now))return false;}
            for(const auto& recipient:npcs){const auto n=npc_combat_.find(recipient.scope.actor);
                if(n!=npc_combat_.end()&&n->second.hex_scope()==recipient.scope&&!n->second.advance_energy(n->second.lifetime(),now))return false;}
            caster=caster_at();if(!caster||!advance_resource_event_locked(*caster,now)||caster->precise_health<=0)return false;
            const auto own=energy(*caster);if(!own)return false;
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            for(auto& recipient:players){
                const auto found=players_.find(recipient.scope.actor);if(found==players_.end()||found->second.effect_scope!=recipient.scope)continue;
                auto& p=found->second;const auto other=energy(p);if(!other)return false;recipient.bonus=*own>*other;
                auto state=admission_player_locked(batch,p,now);if(!state)return false;
                auto armor=watch_yourself_armor_sequence_locked(p,combat::Kind::fire,now);std::optional<combat::Hit> previous;
                for(unsigned hit=0;hit<(recipient.bonus?2u:1u)&&state->health.current>0;++hit){
                    auto actor=incoming_spell_actor_locked(p,{p.id,recipient.scope.generation,combat::Health{state->health.current,state->health.capacity.maximum()},{},p.effects,true,{},p.reviewed_fire_armor},now,true);
                    if(armor){const auto rating=armor(previous?&*previous:nullptr);if(!rating)return false;actor.fire_armor=p.conditions.armor(*rating,now);}
                    const auto step=project_batch_player_damage_locked(batch,p,actor,damage,now);if(!step){valid=false;break;}previous=step->projected;
                }
                if(!valid)break;
                if(state->health.current>0&&recipient.bonus&&!burning_immune_locked(p,now)){
                    auto conditions=p.conditions;if(!conditions.apply(combat::conditions::Kind::burning,scope.actor,now,plan.burning_ms))return false;
                    const auto old=p.conditions.lookup(combat::conditions::Kind::burning),added=conditions.lookup(combat::conditions::Kind::burning);
                    if((!old||old->expires!=added->expires||!p.condition_effects.contains(combat::conditions::Kind::burning))&&p.next_health_effect>UINT32_MAX)return false;
                    if(!project_batch_condition_reactions_locked(batch,p,p.conditions,conditions,now)){valid=false;break;}
                }
            }
            if(batch.missing_seed){const auto owner=*batch.missing_seed;const auto p=players_.find(owner.actor);
                if(p==players_.end()||p->second.effect_scope!=owner)return false;const auto fact=prepare_healing_seed_direct_locked(p->second,1.,now);if(!fact)return false;facts[owner]=*fact;continue;}
            if(!valid)return false;
            for(auto& recipient:npcs){
                auto found=npc_combat_.find(recipient.scope.actor);if(found==npc_combat_.end()||found->second.hex_scope()!=recipient.scope)continue;
                const auto other=found->second.precise_energy_units(found->second.lifetime());if(!other)return false;recipient.bonus=*own>*other;
                auto preview=batch.npcs.contains(recipient.scope)?batch.npcs.at(recipient.scope):found->second;const auto life=preview.lifetime();
                for(unsigned hit=0;hit<(recipient.bonus?2u:1u)&&preview.actor(life)->health->current>0;++hit){std::optional<combat::Hit> actual;
                    if(!preflight_npc_reversal_or_damage_locked(preview,damage,now,nullptr,&actual))return false;if(actual)preview.record_watch_yourself_hit(life,*actual,true,now);}
                const auto registry=preview.owned_effect_snapshot(life);if(!registry)return false;
                if(preview.actor(life)->health->current>0&&recipient.bonus&&preview.allows_conditions(life)&&!active_frigid_armor(*registry,recipient.scope,now)&&
                   !preview.apply_condition(life,combat::conditions::Kind::burning,scope.actor,now,plan.burning_ms))return false;
                for(const auto& event:preview.take_fragility_hits(life))if(event.reversal&&!validate_npc_reversal_receipts_locked(recipient.scope,{event.reversal}))return false;
            }
            admitted=true;break;
        }
        if(!admitted)return false;
        const auto body=[&](Scope owner){publish_target_body_locked(owner.actor,plan.skill,scope.actor,publication?nullptr:visuals);};
        for(const auto recipient:players){
            auto found=players_.find(recipient.scope.actor);if(found==players_.end()||found->second.effect_scope!=recipient.scope||!found->second.resources||found->second.precise_health<=0)continue;
            body(recipient.scope);
            for(unsigned hit=0;hit<(recipient.bonus?2u:1u);++hit){
                found=players_.find(recipient.scope.actor);if(found==players_.end()||found->second.effect_scope!=recipient.scope||!found->second.resources||found->second.precise_health<=0)break;
                auto& p=found->second;auto actor=incoming_spell_actor_locked(p,{p.id,recipient.scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,{},p.reviewed_fire_armor},now,true);
                const auto incoming=combat::apply(actor,damage,true);
                if(!commit_player_reactive_hit_locked(p,incoming,scope.actor,now,false,nullptr,false,true,nullptr,scope))throw std::logic_error("admitted Mind Burn player damage failed");
            }
            found=players_.find(recipient.scope.actor);if(found==players_.end()||found->second.effect_scope!=recipient.scope||found->second.precise_health<=0)continue;
            auto& p=found->second;if(p.passive_recovery)p.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);
            if(recipient.bonus&&!burning_immune_locked(p,now)){
                auto conditions=p.conditions;if(!conditions.apply(combat::conditions::Kind::burning,scope.actor,now,plan.burning_ms))throw std::logic_error("admitted Mind Burn Burning failed");
                const auto old=p.conditions.lookup(combat::conditions::Kind::burning),added=conditions.lookup(combat::conditions::Kind::burning);
                if((!old||old->expires!=added->expires||!p.condition_effects.contains(combat::conditions::Kind::burning))&&p.next_health_effect>UINT32_MAX)throw std::logic_error("Mind Burn condition identity exhausted");
                commit_conditions_locked(p,conditions,{combat::conditions::Kind::burning},now);
            }
        }
        for(const auto recipient:npcs){
            auto found=npc_combat_.find(recipient.scope.actor);if(found==npc_combat_.end()||found->second.hex_scope()!=recipient.scope)continue;
            const auto initial=found->second.actor(found->second.lifetime());if(!initial||!initial->health||initial->health->current<=0)continue;
            body(recipient.scope);
            for(unsigned hit=0;hit<(recipient.bonus?2u:1u);++hit){
                found=npc_combat_.find(recipient.scope.actor);const auto placed=npcs_.find(recipient.scope.actor);
                if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=recipient.scope)break;
                const auto actor=found->second.actor(found->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)break;
                caster=caster_at();
                if(!commit_npc_reversal_or_damage_locked(caster,placed->second,found->second,damage,now,nullptr,nullptr,nullptr,false,scope))throw std::logic_error("admitted Mind Burn NPC damage failed");
            }
            found=npc_combat_.find(recipient.scope.actor);if(found==npc_combat_.end()||found->second.hex_scope()!=recipient.scope)continue;
            auto& live=found->second;const auto life=live.lifetime();const auto actor=live.actor(life);const auto registry=live.owned_effect_snapshot(life);
            if(!actor||!actor->health||actor->health->current<=0||!recipient.bonus||!live.allows_conditions(life)||!registry||active_frigid_armor(*registry,recipient.scope,now))continue;
            if(!live.apply_condition(life,combat::conditions::Kind::burning,scope.actor,now,plan.burning_ms))throw std::logic_error("admitted Mind Burn NPC Burning failed");
            npc_condition_credit_[{recipient.scope.actor,scope.actor}]={recipient.scope,scope};
            const auto presentation=live.actor(life)->effects;for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(recipient.scope.actor,presentation)});
            const bool reacted=publish_npc_fragility_locked(recipient.scope.actor,live);
            found=npc_combat_.find(recipient.scope.actor);if(found==npc_combat_.end()||found->second.hex_scope()!=recipient.scope)continue;
            if(!reacted)commit_applications_locked(recipient.scope,now);
        }
        caster=caster_at();if(caster&&caster->passive_recovery)caster->passive_recovery->record(recovery::Event::target_enemy_spell_or_signet,now);
        dispatch_fevered_boundary_locked(now);return true;
        };
        const bool accepted=execute();auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();
        return accepted;
    }
