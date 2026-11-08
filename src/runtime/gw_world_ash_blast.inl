// Target-centered completion using shared damage authority followed by a
// current-registry hex application. The target is a scope, never a raw ID.
public:
    bool stage_ash_blast(skills::effects::Scope source,
            skills::effects::Scope target,const skills::effects::AshBlastPlan& plan,
            unsigned level,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        const auto canonical=skills::effects::ash_blast(plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255||source==target)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto caster=players_.find(source.actor),selected=players_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||!caster->second.team_token||
           caster->second.knocked_down_until>now||
           !advance_health_locked(caster->second,now)||
           caster->second.precise_health<=0)return false;
        const auto origin=healing_position_locked(caster->second,now);
        std::optional<navigation::Point> center;
        unsigned center_plane=0;
        if(selected!=players_.end()){
            if(selected->second.effect_scope!=target||!selected->second.resources||
               !selected->second.team_token||
               caster->second.team_token==selected->second.team_token||
               !advance_health_locked(selected->second,now)||
               selected->second.precise_health<=0)return false;
            center=healing_position_locked(selected->second,now);
            center_plane=selected->second.surface.plane;
        }else{
            const auto placement=npcs_.find(target.actor);
            const auto live=npc_combat_.find(target.actor);
            if(placement==npcs_.end()||live==npc_combat_.end()||
               live->second.hex_scope()!=target||
               !live->second.hostile_to_players(live->second.lifetime()))return false;
            refresh_npc_movement_locked(target.actor,now);
            const auto* actor=live->second.actor(live->second.lifetime());
            if(!actor||!actor->damageable||!actor->health||
               actor->health->current<=0)return false;
            center=navigation::Point{placement->second.x,placement->second.y};
            center_plane=placement->second.plane;
        }
        if(!origin||!center||std::hypot(origin->x-center->x,
                origin->z-center->z)>plan.range||
                caster->second.surface.plane!=center_plane)return false;
        if(navigation_){const auto line=layered_->trace(caster->second.surface,*origin,*center);
            if(!line||line->plane!=center_plane)return false;}
        const auto team=caster->second.team_token;
        const combat::Damage damage{combat::Kind::earth,double(plan.damage),level,1085};
        struct Contact {skills::effects::Scope scope;bool npc=false,burning=false;unsigned effect=0;};
        std::vector<Contact> contacts;
        // Settle all actors before taking impact-time Burning qualification.
        for(auto& [id,p]:players_){
            if(!p.resources||!p.team_token||p.team_token==team||p.surface.plane!=center_plane)continue;
            const auto point=healing_position_locked(p,now);if(!point)return false;
            if(std::hypot(point->x-center->x,point->z-center->z)>plan.radius)continue;
            if(!p.effect_registry||!p.reviewed_earth_armor||!advance_health_locked(p,now))return false;
        }
        std::vector<unsigned> ids;for(const auto& [id,live]:npc_combat_)ids.push_back(id);
        for(const auto id:ids){
            auto found=npc_combat_.find(id);auto placed=npcs_.find(id);
            if(found==npc_combat_.end()||placed==npcs_.end()||placed->second.plane!=center_plane||
               !found->second.hostile_to_players(found->second.lifetime()))continue;
            refresh_npc_movement_locked(id,now);found=npc_combat_.find(id);placed=npcs_.find(id);
            if(found==npc_combat_.end()||placed==npcs_.end())continue;
            if(std::hypot(placed->second.x-center->x,placed->second.y-center->z)>plan.radius)continue;
            advance_npc_conditions_locked(id,found->second,now);
        }
        for(auto& [id,p]:players_){
            if(!p.resources||!p.team_token||p.team_token==team||p.surface.plane!=center_plane||p.precise_health<=0)continue;
            const auto point=healing_position_locked(p,now);if(!point)return false;
            if(std::hypot(point->x-center->x,point->z-center->z)<=plan.radius)
                contacts.push_back({p.effect_scope,false,p.conditions.has(combat::conditions::Kind::burning,now)});
        }
        for(const auto& [id,live]:npc_combat_){
            const auto placed=npcs_.find(id);const auto life=live.lifetime();const auto* actor=live.actor(life);
            if(placed==npcs_.end()||placed->second.plane!=center_plane||!live.hostile_to_players(life)||
               !actor||!actor->damageable||!actor->health||actor->health->current<=0)continue;
            if(std::hypot(placed->second.x-center->x,placed->second.y-center->z)<=plan.radius)
                contacts.push_back({live.hex_scope(),true,live.has_condition(life,combat::conditions::Kind::burning,now)});
        }
        if(std::none_of(contacts.begin(),contacts.end(),[&](const auto& c){return c.scope==target;}))return false;
        const auto incoming=[&](Player& p){return incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
            combat::Health{p.precise_health,p.resources->max_health},p.reviewed_earth_armor,p.effects,true},now,true);};
        auto next_npc_effect=next_npc_hex_;
        for(auto& contact:contacts){
            if(!contact.npc){
                auto found=players_.find(contact.scope.actor);
                if(found==players_.end()||found->second.effect_scope!=contact.scope)return false;
                auto& p=found->second;auto actor=incoming(p);
                const auto step=combat::project_damage(actor,damage,player_damage_projection_locked(p,now));
                if(!step||!prepare_player_reactive_seed_locked(p,step->incoming,now))return false;
                found=players_.find(contact.scope.actor);
                if(found==players_.end()||found->second.effect_scope!=contact.scope||found->second.precise_health<=0)return false;
                auto& current=found->second;
                actor=incoming(current);
                if(!combat::project_damage(actor,damage,player_damage_projection_locked(current,now)))return false;
                if(contact.burning&&actor.health->current>0){
                    if(current.next_health_effect>UINT32_MAX||!skills::effects::prepare_ash_blast_hex(plan,source,contact.scope,
                            unsigned(current.next_health_effect),now,*current.effect_registry,true,true))return false;
                    if(contact.scope==target){
                        const auto owner=players_.find(source.actor);if(owner==players_.end()||owner->second.effect_scope!=source)return false;
                        if(!prevent_selected_hex_locked(owner->second,current,1085,now,false))return false;
                    }
                }
            }else{
                const auto found=npc_combat_.find(contact.scope.actor);
                if(found==npc_combat_.end()||found->second.hex_scope()!=contact.scope)return false;
                auto preview=found->second;
                if(!preflight_npc_reversal_or_damage_locked(preview,damage,now))return false;
                if(contact.burning&&preview.actor(preview.lifetime())->health->current>0){
                    if(next_npc_effect>=UINT32_MAX)return false;
                    if(!preview.advance_conditions(preview.lifetime(),now)||
                       !preview.apply_ash_blast_hex(preview.lifetime(),plan,source,next_npc_effect,now,true))return false;
                    contact.effect=next_npc_effect++;
                }
            }
        }
        // Reserve all NPC identities before callbacks can allocate more.
        next_npc_hex_=next_npc_effect;
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        const auto late=[&](const Contact& contact){
            std::cerr<<"Ash Blast late recipient refusal actor="<<contact.scope.actor<<" generation="<<contact.scope.generation<<" at="<<now<<std::endl;
        };
        for(auto& contact:contacts){
            if(!contact.npc){
                auto found=players_.find(contact.scope.actor);
                if(found==players_.end()||found->second.effect_scope!=contact.scope||!found->second.resources||found->second.precise_health<=0)continue;
                auto& p=found->second;
                // Reserve a current target-local identity before damage reactions.
                auto actor=incoming(p);
                const auto step=combat::project_damage(actor,damage,player_damage_projection_locked(p,now));
                if(!step){late(contact);continue;}
                if(contact.burning&&actor.health->current>0){
                    if(p.next_health_effect>UINT32_MAX){late(contact);continue;}
                    contact.effect=unsigned(p.next_health_effect++);
                }
                const auto hit=step->incoming;
                if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(p,hit,source.actor,now)){late(contact);continue;}
                found=players_.find(contact.scope.actor);
                if(found==players_.end()||found->second.effect_scope!=contact.scope||found->second.precise_health<=0||!contact.burning)continue;
                auto& current=found->second;
                if(!contact.effect){
                    if(current.next_health_effect>UINT32_MAX){late(contact);continue;}
                    contact.effect=unsigned(current.next_health_effect++);
                }
                if(contact.scope==target){
                    const auto owner=players_.find(source.actor);
                    if(owner==players_.end()||owner->second.effect_scope!=source){late(contact);continue;}
                    const auto prevention=prevent_selected_hex_locked(owner->second,current,1085,now);
                    if(!prevention){late(contact);continue;}
                    if(*prevention)continue;
                }
                const auto hex=skills::effects::prepare_ash_blast_hex(plan,source,contact.scope,contact.effect,now,
                    *current.effect_registry,true,true);
                if(!hex){late(contact);continue;}
                const auto applied=current.effect_registry->apply(contact.scope,hex->hex,skills::effects::Replace::same_skill);
                if(!applied.accepted){late(contact);continue;}
                for(const auto& old:applied.removed){
                    current.health_effects.erase(old.instance);current.movement_effects.remove(old.instance,now);
                    for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(current.id,old.instance)});
                }
                current.health_effects.emplace(hex->hex.instance,hex->hex);
                for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_add(hex->hex,current.id,now)});
                commit_applications_locked(contact.scope,now);publish_status_locked(current,now);
            }else{
                auto found=npc_combat_.find(contact.scope.actor);auto placed=npcs_.find(contact.scope.actor);
                if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=contact.scope)continue;
                const auto* actor=found->second.actor(found->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)continue;
                const auto owner=players_.find(source.actor);
                Player* cause=owner!=players_.end()&&owner->second.effect_scope==source?&owner->second:nullptr;
                if(!commit_npc_reversal_or_damage_locked(cause,placed->second,found->second,damage,now)){late(contact);continue;}
                found=npc_combat_.find(contact.scope.actor);
                if(found==npc_combat_.end()||found->second.hex_scope()!=contact.scope||!contact.burning)continue;
                const auto life=found->second.lifetime();actor=found->second.actor(life);
                if(!actor||!actor->health||actor->health->current<=0)continue;
                if(!contact.effect){if(next_npc_hex_==UINT32_MAX){late(contact);continue;}contact.effect=next_npc_hex_++;}
                // The strict followup does not settle or skip regeneration time.
                advance_npc_conditions_locked(contact.scope.actor,found->second,now);
                found=npc_combat_.find(contact.scope.actor);
                if(found==npc_combat_.end()||found->second.hex_scope()!=contact.scope)continue;
                const auto applied=found->second.apply_ash_blast_hex(life,plan,source,contact.effect,now,true);
                if(!applied){late(contact);continue;}
                npc_condition_credit_[{contact.scope.actor,source.actor}]={contact.scope,source};
                for(auto& [id,peer]:players_){
                    for(const auto& old:applied->removed)peer.outbox->enqueue({health_effect_remove(contact.scope.actor,old.instance)});
                    peer.outbox->enqueue({health_effect_add(*found->second.hex_effect(life,contact.effect),contact.scope.actor,now)});
                    peer.outbox->enqueue({emotes::effects_2022(contact.scope.actor,found->second.actor(life)->effects)});
                }
                commit_applications_locked(contact.scope,now);
            }
        }
        captures.clear();auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));return true;
    }
