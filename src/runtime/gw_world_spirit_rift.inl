// Included inside Instance. Fixed centers are stored independently of victims.
private:
    std::optional<std::uint64_t> next_spirit_rift_locked()const{
        std::optional<std::uint64_t> next;for(const auto& r:spirit_rifts_)if(r.armed&&(!next||r.pulse.at<*next))next=r.pulse.at;return next;
    }
    struct RiftRecipients {std::vector<skills::effects::Scope> players,npcs;};
    std::optional<RiftRecipients> admit_spirit_rift_locked(const skills::effects::SpiritRiftPulse& pulse){
        const auto source=players_.find(pulse.caster.actor);
        if(source==players_.end()||source->second.effect_scope!=pulse.caster)return {};
        const auto source_team=source->second.team_token;
        RiftRecipients recipients;
        for(auto& [id,p]:players_){
            if(!p.resources||p.precise_health<=0||!p.team_token||!source_team||p.team_token==source_team||p.surface.plane!=pulse.plane)continue;
            const auto pos=healing_position_locked(p,pulse.at);
            if(!pos)return {};
            if(std::hypot(pos->x-pulse.x,pos->z-pulse.y)>pulse.plan.radius)continue;
            if(!p.reviewed_lightning_armor||!advance_health_locked(p,pulse.at))return {};
            if(p.precise_health>0)recipients.players.push_back(p.effect_scope);
        }
        std::vector<unsigned> ids;for(const auto& [id,npc]:npcs_)ids.push_back(id);
        for(const auto id:ids){
            auto live=npc_combat_.find(id);if(live==npc_combat_.end()||!live->second.hostile_to_players(live->second.lifetime()))continue;
            refresh_npc_movement_locked(id,pulse.at);advance_npc_conditions_locked(id,live->second,pulse.at);
            live=npc_combat_.find(id);const auto body=npcs_.find(id);if(live==npc_combat_.end()||body==npcs_.end())continue;
            const auto actor=live->second.actor(live->second.lifetime());
            if(!actor||!actor->health||actor->health->current<=0||!actor->damageable||body->second.plane!=pulse.plane||std::hypot(body->second.x-pulse.x,body->second.y-pulse.y)>pulse.plan.radius)continue;
            if(!actor->lightning_armor)return {};
            recipients.npcs.push_back(live->second.hex_scope());
        }
        std::map<skills::effects::Scope,skills::effects::HealingSeedReaction> facts;
        const combat::Damage damage{combat::Kind::lightning,pulse.plan.damage,pulse.level,910,0};
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            for(const auto scope:recipients.players){
                const auto found=players_.find(scope.actor);if(found==players_.end()||found->second.effect_scope!=scope)continue;
                const auto& p=found->second;auto state=admission_player_locked(batch,p,pulse.at);if(!state){valid=false;break;}
                if(state->health.current<=0)continue;
                auto actor=incoming_spell_actor_locked(p,{p.id,scope.generation,combat::Health{state->health.current,state->health.capacity.maximum()},{},p.effects,true,p.reviewed_lightning_armor},pulse.at,true);
                if(!project_batch_player_damage_locked(batch,p,actor,damage,pulse.at)){valid=false;break;}
                if(state->health.current<=0)continue;
                auto conditions=p.conditions;
                if(!conditions.apply(combat::conditions::Kind::cracked_armor,pulse.caster.actor,pulse.at,pulse.plan.cracked_ms)){valid=false;break;}
                const auto before=p.conditions.lookup(combat::conditions::Kind::cracked_armor),after=conditions.lookup(combat::conditions::Kind::cracked_armor);
                if((!before||before->expires!=after->expires||!p.condition_effects.contains(combat::conditions::Kind::cracked_armor))&&p.next_health_effect>UINT32_MAX){valid=false;break;}
                if(!project_batch_condition_reactions_locked(batch,p,p.conditions,conditions,pulse.at)){valid=false;break;}
            }
            if(batch.missing_seed){
                const auto scope=*batch.missing_seed;const auto found=players_.find(scope.actor);
                if(found==players_.end()||found->second.effect_scope!=scope)return {};
                const auto fact=prepare_healing_seed_direct_locked(found->second,1.,pulse.at);if(!fact)return {};
                facts[scope]=*fact;continue;
            }
            if(!valid)return {};
            for(const auto scope:recipients.npcs){
                const auto live=npc_combat_.find(scope.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=scope)continue;
                auto preview=batch.npcs.contains(scope)?batch.npcs.at(scope):live->second;const auto life=preview.lifetime();
                if(!preflight_npc_reversal_or_damage_locked(preview,damage,pulse.at))return {};
                const auto actor=preview.actor(life);if(!actor||!actor->health)return {};
                if(actor->health->current>0&&preview.allows_conditions(life)&&!preview.apply_condition(life,combat::conditions::Kind::cracked_armor,pulse.caster.actor,pulse.at,pulse.plan.cracked_ms))return {};
                for(const auto& event:preview.take_fragility_hits(life))if(event.reversal&&!validate_npc_reversal_receipts_locked(scope,{event.reversal}))return {};
            }
            return recipients;
        }
        return {};
    }
    void dispatch_spirit_rift_locked(std::uint64_t now){
        std::vector<PendingRift> due;
        for(auto it=spirit_rifts_.begin();it!=spirit_rifts_.end();){
            if(!it->armed||it->pulse.at>now){++it;continue;}
            due.push_back(std::move(*it));it=spirit_rifts_.erase(it);
        }
        for(const auto& rift:due){const auto& pulse=rift.pulse;
            const auto source=players_.find(pulse.caster.actor);if(source==players_.end()||source->second.effect_scope!=pulse.caster)continue;
            const auto admitted=admit_spirit_rift_locked(pulse);
            if(!admitted){std::cerr<<"Spirit Rift pulse refused before damage: recipient admission"<<std::endl;continue;}
            for(const auto scope:admitted->players){
                auto found=players_.find(scope.actor);if(found==players_.end()||found->second.effect_scope!=scope)continue;
                auto& p=found->second;if(!p.resources||p.precise_health<=0)continue;
                auto actor=incoming_spell_actor_locked(p,{p.id,scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,p.reviewed_lightning_armor},pulse.at,true);
                const auto hit=combat::apply(actor,{combat::Kind::lightning,pulse.plan.damage,pulse.level,910,0},true);
                if(!commit_player_reactive_hit_locked(p,hit,pulse.caster.actor,pulse.at,false,nullptr,false,true,nullptr,pulse.caster))throw std::logic_error("admitted Rift player hit failed");
                found=players_.find(scope.actor);if(found==players_.end()||found->second.effect_scope!=scope||found->second.precise_health<=0)continue;
                auto conditions=found->second.conditions;
                if(!conditions.apply(combat::conditions::Kind::cracked_armor,pulse.caster.actor,pulse.at,pulse.plan.cracked_ms))throw std::logic_error("admitted Rift player condition failed");
                const auto old=found->second.conditions.lookup(combat::conditions::Kind::cracked_armor),added=conditions.lookup(combat::conditions::Kind::cracked_armor);
                if((!old||old->expires!=added->expires||!found->second.condition_effects.contains(combat::conditions::Kind::cracked_armor))&&found->second.next_health_effect>UINT32_MAX)
                    throw std::logic_error("Rift condition capacity changed during damage reactions");
                commit_conditions_locked(found->second,conditions,{combat::conditions::Kind::cracked_armor},pulse.at);
            }
            for(const auto scope:admitted->npcs){const auto id=scope.actor;
                auto found=npc_combat_.find(id);if(found==npc_combat_.end()||found->second.hex_scope()!=scope)continue;
                refresh_npc_movement_locked(id,pulse.at);
                found=npc_combat_.find(id);if(found==npc_combat_.end()||found->second.hex_scope()!=scope)continue;
                advance_npc_conditions_locked(id,found->second,pulse.at);
                found=npc_combat_.find(id);const auto body=npcs_.find(id);
                if(found==npc_combat_.end()||body==npcs_.end()||found->second.hex_scope()!=scope)continue;
                const auto life=found->second.lifetime();const auto actor=found->second.actor(life);
                if(!actor||!actor->health||actor->health->current<=0)continue;
                const auto owner=players_.find(pulse.caster.actor);
                Player* credit=owner!=players_.end()&&owner->second.effect_scope==pulse.caster?&owner->second:nullptr;
                const combat::Damage damage{combat::Kind::lightning,pulse.plan.damage,pulse.level,910,0};
                if(!commit_npc_reversal_or_damage_locked(credit,body->second,found->second,damage,pulse.at,nullptr,nullptr,nullptr,false,pulse.caster))
                    throw std::logic_error("admitted Rift NPC damage failed");
                found=npc_combat_.find(id);if(found==npc_combat_.end()||found->second.hex_scope()!=scope)continue;
                const auto current_life=found->second.lifetime();const auto current=found->second.actor(current_life);
                if(!current||!current->health||current->health->current<=0||!found->second.allows_conditions(current_life))continue;
                if(!found->second.apply_condition(current_life,combat::conditions::Kind::cracked_armor,pulse.caster.actor,pulse.at,pulse.plan.cracked_ms))
                    throw std::logic_error("admitted Rift NPC condition failed");
                npc_condition_credit_[{id,pulse.caster.actor}]={scope,pulse.caster};
                const auto presentation=found->second.actor(current_life)->effects;
                for(auto&[peer,p]:players_)p.outbox->enqueue({emotes::effects_2022(id,presentation)});
                const bool reacted=publish_npc_fragility_locked(id,found->second);
                found=npc_combat_.find(id);if(found==npc_combat_.end()||found->second.hex_scope()!=scope)continue;
                if(!reacted)commit_applications_locked(scope,pulse.at);
            }
            dispatch_fevered_boundary_locked(pulse.at);
        }
    }
public:
    bool spirit_rift(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::SpiritRift& plan,unsigned level,
                     std::uint64_t now,std::vector<Packet>* visuals,std::function<void()>* publication){
        if(visuals)visuals->clear();if(publication)*publication={};const auto canonical=skills::effects::spirit_rift(plan.rank);
        if(!visuals||!publication||!canonical||*canonical!=plan||!level||level>255||now>UINT64_MAX-plan.delay)return false;
        std::lock_guard lock(mutex_);settle_damage_fields_locked(now);
        const auto a=players_.find(scope.actor);if(a==players_.end()||a->second.effect_scope!=scope||!a->second.resources||
            !resurrection_clock_valid(a->second,now)||!advance_health_locked(a->second,now)||a->second.precise_health<=0||a->second.knocked_down_until>now||spirit_rifts_.size()>=4096||next_rift_==UINT64_MAX)return false;
        const auto origin=healing_position_locked(a->second,now);if(!origin)return false;navigation::Point center{};std::uint16_t plane=0;
        if(const auto b=players_.find(target.actor);b!=players_.end()){
            if(b->second.effect_scope!=target||!b->second.resources||!b->second.team_token||!a->second.team_token||b->second.team_token==a->second.team_token||!resurrection_clock_valid(b->second,now)||!advance_health_locked(b->second,now)||b->second.precise_health<=0)return false;
            const auto p=healing_position_locked(b->second,now);if(!p)return false;center=*p;plane=static_cast<std::uint16_t>(b->second.surface.plane);
        }else{
            const auto npc=npcs_.find(target.actor);const auto live=npc_combat_.find(target.actor);
            if(npc==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=target||!live->second.hostile_to_players(live->second.lifetime())||live->second.health_clock(live->second.lifetime()).value_or(0)>now)return false;
            advance_npc_conditions_locked(target.actor,live->second,now);refresh_npc_movement_locked(target.actor,now);
            const auto* actor=live->second.actor(live->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)return false;center={npc->second.x,npc->second.y};plane=npc->second.plane;
        }
        if(plane!=a->second.surface.plane||std::hypot(center.x-origin->x,center.z-origin->z)>1248)return false;
        if(navigation_){const auto trace=layered_->trace(a->second.surface,*origin,center);if(!trace||trace->plane!=plane)return false;}
        const auto key=next_rift_++;spirit_rifts_.push_back({{scope,level,center.x,center.z,plane,plan,now},key});
        visuals->push_back(skills::particles::spirit_rift_ground_2022(scope.actor,center.x,center.z,plane));
        *publication=[this,scope,key]{std::lock_guard guard(mutex_);for(auto& r:spirit_rifts_)if(r.key==key&&r.pulse.caster==scope&&r.visual_seen){r.armed=true;break;}};
        return true;
    }
