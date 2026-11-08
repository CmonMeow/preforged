    bool cry_frustration(skills::effects::Scope scope,skills::effects::Scope target,const skills::effects::CryFrustrationPlan& plan,unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr,std::function<void()>* publication=nullptr){
        if(visuals)visuals->clear();if(publication)*publication={};if(publication&&!visuals)return false;
        using namespace skills::effects;
        const auto canonical=skills::effects::cry_frustration(plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255)return false;
        std::lock_guard lock(mutex_);DamagePublication pending;if(publication)begin_damage_publication_locked(pending);
        const auto perform=[&]()->bool{
        const auto caster=players_.find(scope.actor);
        if(caster==players_.end()||caster->second.effect_scope!=scope)return false;
        auto& source=caster->second;
        if(!source.resources||!source.team_token||source.precise_health<=0||source.knocked_down_until>now||now<source.health_updated)return false;
        float center_x=0,center_y=0;std::uint16_t plane=0;
        const auto selected=players_.find(target.actor);
        if(selected!=players_.end()){
            auto& victim=selected->second;
            if(victim.effect_scope!=target||!victim.resources||!victim.team_token||victim.team_token==source.team_token)return false;
            const auto point=healing_position_locked(victim,now);if(!point)return false;
            center_x=point->x;center_y=point->z;plane=victim.surface.plane;
        }else{
            const auto placement=npcs_.find(target.actor);const auto live=npc_combat_.find(target.actor);
            if(placement==npcs_.end()||live==npc_combat_.end()||target!=Scope{instance_scope_,target.actor,live->second.lifetime().actor_generation}||!live->second.hostile_to_players(live->second.lifetime()))return false;
            center_x=placement->second.x;center_y=placement->second.y;plane=placement->second.plane;
        }
        const auto from=healing_position_locked(source,now);
        if(!from||source.surface.plane!=plane||std::hypot(from->x-center_x,from->z-center_y)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(source.surface,*from,{center_x,center_y});if(!line||line->plane!=plane)return false;}
        if(!advance_health_locked(source,now)||source.precise_health<=0)return false;
        std::vector<CryContact> contacts;
        for(auto&[id,victim]:players_){
            if(!victim.resources||!victim.team_token||victim.team_token==source.team_token||victim.surface.plane!=plane)continue;
            const auto point=healing_position_locked(victim,now);if(!point)return false;
            const float distance=std::hypot(point->x-center_x,point->z-center_y);if(distance>plan.radius&&id!=target.actor)continue;
            if(now<victim.energy_updated||!advance_health_locked(victim,now)||!preflight_interrupt_protection_locked(victim,now))return false;
            if(victim.precise_health<=0){if(id==target.actor)return false;continue;}
            const bool skill=victim.casting_skill&&now>=victim.casting_begins&&now<victim.casting_ends&&victim.knocked_down_until<=now;
            const bool attack=(victim.ordinary_animation||now<victim.melee_skill_until)&&victim.knocked_down_until<=now;
            contacts.push_back({victim.effect_scope,id==target.actor?0.f:distance,true,true,skill?CryActivity::skill:attack?CryActivity::basic_attack:CryActivity::idle});
        }
        std::vector<Scope> npc_scopes;for(const auto& [id,live]:npc_combat_)npc_scopes.push_back(live.hex_scope());
        for(const auto expected:npc_scopes){
            const auto id=expected.actor;auto current=npc_combat_.find(id);
            if(current==npc_combat_.end()||current->second.hex_scope()!=expected)continue;
            const auto placement=npcs_.find(id);const auto life=current->second.lifetime();auto& initial=current->second;
            if(placement==npcs_.end()||placement->second.plane!=plane||!initial.hostile_to_players(life))continue;
            const float distance=std::hypot(placement->second.x-center_x,placement->second.y-center_y);if(distance>plan.radius&&id!=target.actor)continue;
            advance_npc_conditions_locked(id,initial,now);current=npc_combat_.find(id);
            if(current==npc_combat_.end()||current->second.hex_scope()!=expected)continue;
            auto& live=current->second;const auto* actor=live.actor(life);
            if(!actor||!actor->damageable||!actor->health){if(id==target.actor)return false;continue;}
            if(actor->health->current<=0){if(id==target.actor)return false;continue;}
            const auto active=live.active_action(life,now);
            const auto activity=!active?CryActivity::idle:(active->kind==combat::npcs::control::Action::spell||active->kind==combat::npcs::control::Action::skill)?CryActivity::skill:CryActivity::basic_attack;
            contacts.push_back({Scope{instance_scope_,id,life.actor_generation},id==target.actor?0.f:distance,true,true,activity});
        }
        const auto proposal=prepare_cry_frustration(plan,target,contacts);if(!proposal)return false;
        if(!proposal->triggered)return true;
        struct PlayerHit {Player* player;Scope scope;bool interrupt;};std::vector<PlayerHit> player_hits;
        struct NpcHit {Scope scope;std::optional<combat::npcs::control::Token> action;bool protected_action;};std::vector<NpcHit> npc_hits;
        const combat::Damage damage{combat::Kind::armor_ignoring,double(plan.damage),level,57};
        for(const auto& recipient:proposal->recipients){
            const auto player=players_.find(recipient.owner.actor);
            if(player!=players_.end()){
                auto& victim=player->second;
                if(recipient.interrupt_attempt&&(victim.control_epoch==UINT64_MAX||victim.attack_interrupt_epoch==UINT64_MAX))return false;
                auto actor=incoming_spell_actor_locked(victim,{victim.id,victim.effect_scope.generation,combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true},now);
                const auto hit=combat::apply(actor,damage,true);
                if(hit.reason!=combat::Reason::accepted||!preflight_player_reversal_locked(victim,hit,now))return false;
                player_hits.push_back({&victim,recipient.owner,recipient.interrupt_attempt});
            }else{
                auto current=npc_combat_.find(recipient.owner.actor);
                if(current==npc_combat_.end()||current->second.hex_scope()!=recipient.owner)return false;
                const auto life=current->second.lifetime();auto& initial=current->second;
                const auto action=recipient.interrupt_attempt?initial.active_action(life,now):std::nullopt;
                const auto protection=action?prevent_npc_interrupt_locked(initial,now,false):std::optional(false);
                if(!protection)return false;
                current=npc_combat_.find(recipient.owner.actor);
                if(current==npc_combat_.end()||current->second.hex_scope()!=recipient.owner)return false;
                auto preview=current->second;if(!preflight_npc_reversal_or_damage_locked(preview,damage,now))return false;
                if(action&&!*protection&&preview.actor(life)->health->current>0&&!preview.cancel_action(life,now))return false;
                npc_hits.push_back({recipient.owner,action,*protection});
            }
        }
        // Admit the complete ordered damage/Seed population before any
        // interruption, damage or protection-counter mutation.
        std::map<Scope,HealingSeedReaction> facts;bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+npc_combat_.size()+1;++pass){
            DamageAdmissionBatch batch;batch.seed_facts=&facts;bool valid=true;
            for(const auto& candidate:player_hits){auto& p=*candidate.player;
                if(p.effect_scope!=candidate.scope||!p.resources)return false;
                auto* state=admission_player_locked(batch,p,now);if(!state)return false;
                if(state->health.current<=0)continue;
                auto actor=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
                    combat::Health{state->health.current,state->health.capacity.maximum()},{},p.effects,true},now);
                if(!project_batch_player_damage_locked(batch,p,actor,damage,now)){valid=false;break;}
            }
            if(batch.missing_seed){const auto owner=*batch.missing_seed;const auto found=players_.find(owner.actor);
                if(found==players_.end()||found->second.effect_scope!=owner)return false;
                const auto fact=prepare_healing_seed_direct_locked(found->second,1.,now);if(!fact)return false;
                facts[owner]=*fact;continue;}
            if(!valid)return false;admitted=true;break;
        }
        if(!admitted||source.precise_health<=0)return false;
        // No prepared whole-actor copy is restored after another recipient's
        // callbacks. Death explosions and other reactions remain authoritative.
        std::vector<DeferredEnergyReaction> energy_reactions;
        for(const auto& candidate:player_hits){auto& victim=*candidate.player;
            if(victim.effect_scope!=candidate.scope||!victim.resources||victim.precise_health<=0)continue;
            if(candidate.interrupt&&!prevent_interrupt_locked(victim,now,&energy_reactions)){
                record_control(victim,now);++victim.attack_interrupt_epoch;victim.casting_skill=0;victim.melee_skill_until=0;
                cancel_ordinary_locked(victim,combat::weapon::Cancel::explicit_input);
            }
            if(victim.precise_health<=0)continue;
            auto actor=incoming_spell_actor_locked(victim,{victim.id,victim.effect_scope.generation,
                combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true},now);
            const auto hit=combat::apply(actor,damage,true);
            if(publication)publish_target_body_locked(victim.id,57,scope.actor);
            if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(victim,hit,scope.actor,now,false,nullptr,false,true,nullptr,scope))
                throw std::logic_error("preflighted Cry player damage failed");
            if(!publication)publish_target_body_locked(victim.id,57,scope.actor);publish_health_locked(victim);
        }
        for(const auto& candidate:npc_hits){
            auto found=npc_combat_.find(candidate.scope.actor);const auto placed=npcs_.find(candidate.scope.actor);
            if(found==npc_combat_.end()||placed==npcs_.end()||found->second.hex_scope()!=candidate.scope)continue;
            auto& live=found->second;const auto life=live.lifetime();
            if(!live.actor(life)->health||live.actor(life)->health->current<=0)continue;
            const bool same_action=candidate.action&&live.active_action(life,now)==candidate.action;
            if(same_action&&!candidate.protected_action){
                if(!live.cancel_action(life,now))throw std::logic_error("preflighted Cry NPC cancellation failed");
                Packet stopped;const auto* row=candidate.action->skill?skills::find_skill(candidate.action->skill->skill):nullptr;
                u16(stopped,158);u32(stopped,candidate.action->kind==combat::npcs::control::Action::attack||(row&&row->type==14)?49:59);u32(stopped,candidate.scope.actor);u32(stopped,0);
                for(auto&[id,peer]:players_)peer.outbox->enqueue({stopped});
            }
            if(publication)publish_target_body_locked(candidate.scope.actor,57,scope.actor);
            if(!commit_npc_reversal_or_damage_locked(source,placed->second,live,damage,now))
                throw std::logic_error("preflighted Cry NPC damage failed");
            found=npc_combat_.find(candidate.scope.actor);
            if(found==npc_combat_.end()||found->second.hex_scope()!=candidate.scope)continue;
            if(!publication)publish_target_body_locked(candidate.scope.actor,57,scope.actor);
            auto& current=found->second;const auto* alive=current.actor(life);
            if(same_action&&candidate.protected_action&&alive&&alive->health&&alive->health->current>0&&current.active_action(life,now)==candidate.action){
                const auto prevented=prevent_npc_interrupt_locked(current,now,true);
                if(!prevented||!*prevented)throw std::logic_error("preflighted Cry NPC protection failed");
            }
        }
        commit_energy_reactions_locked(energy_reactions);return true;
        };
        const bool accepted=perform();dispatch_fevered_boundary_locked(now);
        auto release=finish_damage_publication_locked(pending);
        if(publication&&accepted)*publication=std::move(release);else if(release)release();return accepted;
    }
