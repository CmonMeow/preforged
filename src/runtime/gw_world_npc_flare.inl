// Included inside world::Instance; dedicated opt-in hostile caster authority.
private:
    std::map<unsigned,combat::ai::FlareController> npc_flare_ai_;
    auto npc_caster_candidate_locked(combat::ai::FlareController& ai,const Npc& body,std::uint64_t now){
        namespace aggro=combat::ai::aggro;
        if(!ai.encounter)ai.encounter.emplace(ai.policy.source);
        std::vector<aggro::Observation> observations;observations.reserve(players_.size());
        for(auto&[id,p]:players_){
            aggro::Observation observation{p.effect_scope,true,true,true,{}};
            // Future observations cannot establish a target's earlier death,
            // plane or pose. Keep prior memory but never acquire/select it.
            if(std::max({p.health_updated,p.energy_updated,p.last_move,p.steering?p.steering_tick:0})<=now){
                observation.alive=p.precise_health>0;observation.published=p.npc_publication_ready;
                observation.same_plane=p.surface.plane==body.plane;
                if(observation.alive&&observation.published&&observation.same_plane){
                    const auto point=healing_position_locked(p,now);
                    if(point)observation.distance=std::hypot(point->x-body.x,point->z-body.y);
                }
            }
            observations.push_back(observation);
        }
        return ai.encounter->observe(observations,ai.policy.acquire,ai.policy.follow,ai.policy.proximity_acquisition);
    }
    combat::ai::failed_target::Decision npc_failed_target_locked(const combat::npcs::control::Token& token,
        skills::effects::Scope selected,const Npc& body,combat::npcs::LiveActor& actor,std::uint64_t now){
        namespace failure=combat::ai::failed_target;
        if(!token.skill||(token.skill->skill!=191&&token.skill->skill!=194&&token.skill->skill!=214))return {};
        failure::Observation facts;facts.selected=selected;
        const auto found=players_.find(selected.actor);
        if(found!=players_.end()){
            auto& p=found->second;facts.current=p.effect_scope;facts.published=p.npc_publication_ready;
            // A reused numeric ID does not lend its resource/pose clocks to
            // the retired target lifetime. Completion can fail the old cast
            // without observing any facts about the replacement actor.
            if(p.effect_scope!=selected)return failure::at_completion(token,facts,now);
            facts.clock=std::max({p.health_updated,p.energy_updated,p.last_move,p.steering?p.steering_tick:0});
            if(facts.clock>now)return {failure::Status::deferred};
            if(p.effect_scope==selected&&p.npc_publication_ready){
                if(!advance_health_locked(p,now))return {failure::Status::deferred};
                facts.alive=p.precise_health>0;facts.same_plane=p.surface.plane==body.plane;
                if(healing_position_locked(p,now))facts.line_of_sight=ordinary_npc_snapshot_locked(p,body,actor,now).line_of_sight;
            }
        }
        return failure::at_completion(token,facts,now);
    }
    void npc_flare_event_locked(unsigned kind,unsigned actor,unsigned value=0,unsigned target=0){
        Packet p;u16(p,target?159:158);u32(p,kind);u32(p,actor);if(target)u32(p,target);u32(p,value);
        for(auto&[id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({p});
    }
    void resolve_npc_flare_releases_locked(combat::ai::FlareController& ai,std::uint64_t now){
        if(!timed_effect_boundary_){(void)settle_timed_effects_locked(now);return;}
        const auto source=npc_combat_.find(ai.policy.source.actor);
        if(!npcs_.contains(ai.policy.source.actor)||source==npc_combat_.end()||source->second.hex_scope()!=ai.policy.source){ai.shots.clear();return;}
        for(std::size_t index=0;index<ai.shots.size();){
            const auto shot=ai.shots[index];
            const auto discard=[&]{ai.shots.erase(ai.shots.begin()+index);};
            if(now<shot.arrives){++index;continue;}
            const auto target=players_.find(shot.target.actor);
            if(target==players_.end()||target->second.effect_scope!=shot.target){discard();continue;}
            auto& p=target->second;
            if(std::max({p.health_updated,p.energy_updated,p.last_move,p.steering?p.steering_tick:0})>now){++index;continue;}
            if(!advance_health_locked(p,now)){++index;continue;}
            if(!p.npc_publication_ready||p.precise_health<=0||p.surface.plane!=shot.plane){discard();continue;}
            const auto point=healing_position_locked(p,now);
            if(!point||!p.resources||!p.reviewed_fire_armor){++index;continue;}
            if(std::hypot(point->x-shot.x,point->z-shot.y)>ai.policy.flight.hit_radius){discard();continue;}
            auto recipient=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,{},p.reviewed_fire_armor},now);
            const auto result=skills::effects::elemental_target(*skills::effects::elemental(194,ai.policy.fire_magic),recipient,p.conditions,ai.policy.level,ai.policy.source.actor,now);
            if(!result.accepted||result.hits.size()!=1){++index;continue;}
            const auto seed=prepare_player_reactive_seed_locked(p,result.hits.front(),now);
            if(!seed){++index;continue;}
            // Consume exactly this immutable delivery before hit/death callbacks.
            // A future or unknown observation for another target cannot stall it.
            discard();
            if(!commit_player_reactive_hit_locked(p,result.hits.front(),ai.policy.source.actor,now,true,&*seed,true))
                throw std::logic_error("preflighted NPC Flare damage changed");
            if(p.passive_recovery){p.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);}
            if(const auto particle=skills::particles::target_body_2022(p.id,194))for(auto&[peer_id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({*particle});
            publish_health_locked(p);if(p.precise_health<=0)kill_locked(p,now);
        }
    }
    void stop_npc_quest_melee_locked(combat::ai::FlareController& ai,combat::npcs::LiveActor& actor,std::uint64_t now){
        const auto ordinary=npc_ordinary_ai_.find(ai.policy.source.actor);
        if(ordinary!=npc_ordinary_ai_.end()&&ordinary->second.source==ai.policy.source&&ordinary->second.ledger.pending()){
            const auto pending=*ordinary->second.ledger.pending();
            if(actor.control_epoch(actor.lifetime())==pending.token.epoch&&actor.cancel_action(actor.lifetime(),now))
                npc_ordinary_packet_locked(combat::weapon::stop_2022(ai.policy.source.actor));
        }
        retire_npc_ordinary_locked(ai.policy.source,now);retire_npc_patrol_locked(ai.policy.source);ai.engaged=false;ai.returning=false;
        ai.melee_quest->retired=true;ai.updated=now;
    }
    void drive_npc_quest_melee_locked(combat::ai::FlareController& ai,Npc& body,combat::npcs::LiveActor& actor,std::uint64_t now){
        auto& binding=*ai.melee_quest;if(binding.retired)return;
        const auto life=actor.lifetime();if(now<actor.health_clock(life).value_or(now)){ai.updated=now;return;}
        const auto owner=players_.find(binding.owner.actor);
        const auto assignment=quest_npc_owners_.find(body.id);
        if(owner==players_.end()||owner->second.effect_scope!=binding.owner||assignment==quest_npc_owners_.end()||
           assignment->second!=std::pair{binding.owner,binding.quest}){stop_npc_quest_melee_locked(ai,actor,now);return;}
        const auto objective=owner->second.quest_kills.find(binding.quest);
        if(objective==owner->second.quest_kills.end()||objective->second.cycle!=binding.cycle||
           !objective->second.remaining.contains(ai.policy.source)){stop_npc_quest_melee_locked(ai,actor,now);return;}
        // Settlement can complete an any-death objective through another
        // assigned actor. Retain identities, then revalidate the live binding
        // before starting/continuing combat against the former quest owner.
        const auto source=ai.policy.source,owner_scope=binding.owner;
        const auto quest=binding.quest,cycle=binding.cycle;
        const auto* motion_address=&ai;const auto* body_address=&body;const auto* actor_address=&actor;
        advance_npc_conditions_locked(source.actor,actor,now);
        const auto current_motion=npc_flare_ai_.find(source.actor);
        const auto current_body=npcs_.find(source.actor);const auto current_actor=npc_combat_.find(source.actor);
        if(current_motion==npc_flare_ai_.end()||&current_motion->second!=motion_address||
           current_motion->second.policy.source!=source||current_body==npcs_.end()||&current_body->second!=body_address||
           current_actor==npc_combat_.end()||&current_actor->second!=actor_address||current_actor->second.hex_scope()!=source)return;
        const auto& current_binding=current_motion->second.melee_quest;
        if(!current_binding||current_binding->retired||current_binding->owner!=owner_scope||
           current_binding->quest!=quest||current_binding->cycle!=cycle)return;
        const auto current_owner=players_.find(owner_scope.actor);
        const auto current_assignment=quest_npc_owners_.find(source.actor);
        if(current_owner==players_.end()||current_owner->second.effect_scope!=owner_scope||
           current_assignment==quest_npc_owners_.end()||current_assignment->second!=std::pair{owner_scope,quest}){
            stop_npc_quest_melee_locked(ai,actor,now);return;
        }
        const auto current_objective=current_owner->second.quest_kills.find(quest);
        if(current_objective==current_owner->second.quest_kills.end()||current_objective->second.cycle!=cycle||
           !current_objective->second.remaining.contains(source)){
            stop_npc_quest_melee_locked(ai,actor,now);return;
        }
        const auto state=actor.actor(life);
        if(!state||!state->health||state->health->current<=0){stop_npc_quest_melee_locked(ai,actor,now);return;}
        auto& victim=current_owner->second;
        if(!ai.returning&&std::max({victim.health_updated,victim.energy_updated,victim.last_move,victim.steering?victim.steering_tick:0})>now){ai.updated=now;return;}
        // Committed Health loss activates owner retaliation. Proximity is a
        // separate explicit enrollment choice: Grawl remains damage-only;
        // the authored Bull policy also acquires its owner within acquire.
        const auto loss=*actor.health_loss_revision(life);
        if(loss!=binding.health_loss_revision||state->health->current<binding.health)binding.triggered=true;
        binding.health_loss_revision=loss;
        binding.health=state->health->current;
        if(drive_npc_ordinary_locked(ai.policy.source,body,actor,now)){ai.updated=now;return;}
        if(actor.knocked_down(life,now)||actor.active_action(life,now)){ai.updated=now;return;}
        if(ai.returning){
            if(move_npc_flare_locked(ai,body,{ai.home_x,ai.home_y},1,now)){ai.returning=false;binding.triggered=false;}
            ai.updated=now;return;
        }
        const auto point=healing_position_locked(victim,now);
        if(ai.engaged&&(!point||victim.precise_health<=0||!victim.npc_publication_ready||victim.surface.plane!=body.plane||
           std::hypot(point->x-body.x,point->z-body.y)>ai.policy.follow)){
            ai.engaged=false;ai.returning=true;suspend_busy_npc_wander_locked(ai,now);ai.updated=now;return;
        }
        const bool proximity=binding.acquisition==combat::ai::FlareController::OwnedAcquisition::owner_proximity&&point&&
            std::hypot(point->x-body.x,point->z-body.y)<=ai.policy.acquire;
        if((binding.triggered||proximity)&&!ai.engaged&&point&&victim.precise_health>0&&victim.npc_publication_ready&&victim.surface.plane==body.plane&&
           std::hypot(point->x-body.x,point->z-body.y)<=ai.policy.follow)ai.engaged=true;
        if(ai.engaged){suspend_busy_npc_wander_locked(ai,now);begin_npc_ordinary_locked(ai,body,actor,victim,now);}
        else idle_npc_wander_locked(ai,body,now);
        ai.updated=now;
    }
    void drive_npc_flare_locked(std::uint64_t now){
        for(auto&[id,ai]:npc_flare_ai_){
            if(now<ai.updated)continue;
            suspend_busy_npc_wander_locked(ai,now);
            const auto placed=npcs_.find(id);const auto live=npc_combat_.find(id);
            if(placed==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=ai.policy.source){
                retire_npc_ordinary_locked(ai.policy.source,now);retire_npc_patrol_locked(ai.policy.source);ai.cast.reset();ai.shots.clear();continue;}
            auto& body=placed->second;auto& actor=live->second;const auto life=actor.lifetime();
            if(ai.melee_quest){drive_npc_quest_melee_locked(ai,body,actor,now);continue;}
            if(drive_npc_owned_immediate_locked(ai,now))continue;
            if(finish_npc_immediate_locked(ai,now)){ai.updated=now;continue;}
            resolve_npc_flare_releases_locked(ai,now);
            const auto replace_ordinary=[&](const combat::ai::ordinary::Windup& pending){
                if(ai.cast||pending.target!=ai.policy.target)return false;
                const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at)->std::optional<std::uint64_t>{
                    const auto target=players_.find(pending.target.actor);
                    if(ai.shots.size()>=ai.policy.max_flights||target==players_.end()||
                       target->second.effect_scope!=pending.target||!target->second.reviewed_fire_armor)return {};
                    return skill_disabled_slot_locked(ai.policy.source,slot,at);
                };
                auto prepared=prepare_npc_ordinary_replacement_locked(ai.policy.source,pending,now,available);
                if(!prepared||prepared->spell.spell.skill!=skills::Slot{194,0})return false;
                const auto token=prepared->spell.spell;
                if(!commit_npc_ordinary_replacement_locked(ai.policy.source,*prepared))return false;
                ai.cast=token;observe_npc_replacement_energy_locked(ai.policy.source,*prepared);
                const auto refreshed=npc_combat_.find(id);
                if(refreshed!=npc_combat_.end()&&refreshed->second.hex_scope()==ai.policy.source&&
                   refreshed->second.owns_spell_action(refreshed->second.lifetime(),token)){
                    const auto alive=refreshed->second.actor(refreshed->second.lifetime());
                    if(alive&&alive->health&&alive->health->current>0){npc_flare_event_locked(60,id,194,pending.target.actor);return true;}
                }
                ai.cast.reset();return true;
            };
            if(drive_npc_ordinary_locked(ai.policy.source,body,actor,now,replace_ordinary)){ai.updated=now;continue;}
            const auto state=actor.actor(life);
            const bool alive=state&&state->health&&state->health->current>0&&actor.hostile_to_players(life);
            auto victim=players_.find(ai.policy.target.actor);
            const auto target_valid=[&]{return victim!=players_.end()&&victim->second.effect_scope==ai.policy.target&&
                victim->second.precise_health>0&&victim->second.npc_publication_ready&&victim->second.surface.plane==body.plane;};
            const auto target_ahead=[&]{return victim!=players_.end()&&victim->second.effect_scope==ai.policy.target&&
                (now<victim->second.health_updated||now<victim->second.energy_updated||now<victim->second.last_move||
                 (victim->second.steering&&now<victim->second.steering_tick));};
            // Once released, the projectile is independent of caster Health;
            // actor retirement/generation mismatch still invalidates its owner.
            if(now<actor.health_clock(life).value_or(now)){ai.updated=now;continue;}
            if(!alive){ai.cast.reset();ai.engaged=false;if(ai.encounter)ai.encounter->clear();ai.updated=now;continue;}
            if(ai.cast){
                const auto token=*ai.cast;
                if(now<actor.health_clock(life).value_or(now)){ai.updated=now;continue;}
                if(actor.control_epoch(life)!=token.epoch){ai.cast.reset();npc_flare_event_locked(59,id);ai.updated=now;continue;}
                if(target_ahead()){ai.updated=now;continue;}
                if(now<token.completes){ai.updated=now;continue;}
                const auto failure=npc_failed_target_locked(token,ai.policy.target,body,actor,now);
                if(failure.status==combat::ai::failed_target::Status::deferred){ai.updated=now;continue;}
                if(failure.status==combat::ai::failed_target::Status::failed){
                    if(actor.fail_target_action(life,token,now)){ai.cast.reset();npc_flare_event_locked(59,id);}
                    ai.updated=now;continue;
                }
                // Completion can fail while an authoritative action clock is
                // ahead. Only a confirmed completion/epoch change retires it.
                const auto aura=prepare_npc_aura_completion_locked(token,now);
                const auto terminal=complete_npc_spell_terminal_locked(token,now);
                if(terminal==NpcSpellTerminal::unavailable){ai.updated=now;continue;}
                ai.cast.reset();npc_flare_event_locked(58,id);
                bool released_this_cast=false;
                if(terminal==NpcSpellTerminal::completed&&target_valid()){
                    const auto point=healing_position_locked(victim->second,now);
                    const auto snapshot=ordinary_npc_snapshot_locked(victim->second,body,actor,now);
                    if(point&&victim->second.surface.plane==body.plane&&snapshot.line_of_sight){const auto aim=projectiles::intercept(ai.policy.flight,body.x,body.y,point->x,point->z,0,0);
                        if(aim&&now<=UINT64_MAX-aim->flight_ms){
                            if(ai.shots.size()>=ai.policy.max_flights)throw std::logic_error("reserved NPC Flare flight capacity lost");
                            ai.shots.push_back({ai.policy.target,aim->x,aim->y,body.plane,now,now+aim->flight_ms,token.sequence,body.x,body.y});
                            released_this_cast=true;
                        }}
                }
                if(released_this_cast){
                    finish_npc_aura_completion_locked(aura,now);
                    if(!succor_npc_spell_completed_locked(token,now))throw std::logic_error("NPC Flare Succor completion failed");
                    std::string reason;
                    if(!completed_skill_action_locked(ai.policy.source,194,now,&reason))
                        std::osyncstream(std::clog)<<"WORLD npc_spell_completion actor="<<id<<" reason="<<reason<<'\n';
                }
                ai.updated=now;continue;
            }
            if(actor.knocked_down(life,now)||actor.active_action(life,now)){ai.updated=now;continue;}
            // An established homeward leg uses the source's clocks and known
            // home point, not observations of its discarded combat target.
            if(!ai.returning&&target_ahead()){ai.updated=now;continue;}
            bool lost_target=ai.encounter&&ai.encounter->size()!=0;
            if(ai.engaged){
                const auto point=target_valid()?healing_position_locked(victim->second,now):std::nullopt;
                if(std::hypot(body.x-ai.home_x,body.y-ai.home_y)>ai.policy.leash){ai.engaged=false;ai.returning=true;}
                else if(!point||std::hypot(point->x-body.x,point->z-body.y)>ai.policy.follow){ai.engaged=false;lost_target=true;}
            }
            if(ai.returning){
                suspend_busy_npc_wander_locked(ai,now);
                if(ai.encounter)ai.encounter->clear();
                if(move_npc_flare_locked(ai,body,{ai.home_x,ai.home_y},1,now))ai.returning=false;
                ai.updated=now;continue;
            }
            // Observe every acquisition entrant while the selected foe is
            // retained. Otherwise a later switch would forget already-engaged
            // foes that have since moved beyond the acquisition radius.
            const auto candidates=npc_caster_candidate_locked(ai,body,now);
            if(!candidates){ai.updated=now;continue;}
            if(!ai.engaged){
                // Aggro oldid2655578: losing one foe need not end combat with
                // remembered foes. Radius and nearest-first remain authored.
                // Unknown remembered poses defer return without fabricating a
                // selectable position or admitting outside-radius newcomers.
                // A rejected target cannot own future chase travel. Retire
                // the old route now, including unknown remembered poses; a
                // later home/retarget decision installs its own future route.
                if(!candidates->target){if(lost_target||candidates->engaged)stop_npc_pursuit_locked(ai.policy.source,now);ai.returning=lost_target&&!candidates->engaged;suspend_busy_npc_wander_locked(ai,now);idle_npc_wander_locked(ai,body,now);ai.updated=now;continue;}
                victim=players_.find(candidates->target->actor);ai.policy.target=*candidates->target;ai.engaged=true;
            }
            suspend_busy_npc_wander_locked(ai,now);
            if(!target_valid()){ai.updated=now;continue;}
            const auto point=healing_position_locked(victim->second,now);if(!point){ai.updated=now;continue;}
            if(begin_npc_immediate_locked(ai,body,victim->second,now)){ai.updated=now;continue;}
            if(npc_hostile_ordinary_.contains(ai.policy.source)){
                begin_npc_ordinary_locked(ai,body,actor,victim->second,now);ai.updated=now;continue;
            }
            const auto snapshot=ordinary_npc_snapshot_locked(victim->second,body,actor,now);
            if(!settle_mind_wrack_caster_locked(ai.policy.source,now)){ai.updated=now;continue;}
            combat::ai::SpellTarget facts{victim->second.effect_scope,float(snapshot.distance),true,true,true,snapshot.line_of_sight};
            const combat::ai::Spellbook::Availability ready=[&](skills::Slot slot,std::uint64_t at)->std::optional<std::uint64_t>{
                if(ai.shots.size()>=ai.policy.max_flights)return {};
                return skill_disabled_slot_locked(ai.policy.source,slot,at);
            };
            if((!victim->second.reviewed_fire_armor||!actor.choose_spell(life,facts,now,ready))&&
                begin_npc_ordinary_locked(ai,body,actor,victim->second,now)){ai.updated=now;continue;}
            if(snapshot.distance>1248||!snapshot.line_of_sight){move_npc_flare_locked(ai,body,*point,snapshot.line_of_sight?1200:0,now);ai.updated=now;continue;}
            stop_npc_pursuit_locked(ai.policy.source,now);
            if(!victim->second.reviewed_fire_armor||!settle_mind_wrack_caster_locked(ai.policy.source,now)){ai.updated=now;continue;}
            const auto before=actor.energy(life);const auto units=actor.precise_energy_units(life);
            const auto token=actor.begin_spell(life,{194,0},facts,now,ready);
            if(token){
                if(before)observe_npc_mind_wrack_energy_locked(ai.policy.source,{before->current,before->maximum,0,units?unsigned(*units%300000):0},now);
                const auto alive=actor.actor(life);if(alive&&alive->health&&alive->health->current>0){ai.cast=*token;npc_flare_event_locked(60,id,194,victim->first);}
            }
            ai.updated=now;
        }
        // Quest lifetimes are frequently reset/restored. Retire only these
        // melee bindings after iteration, leaving caster lifecycle unchanged.
        std::erase_if(npc_flare_ai_,[&](const auto& entry){
            const auto& ai=entry.second;if(!ai.melee_quest)return false;
            const auto live=npc_combat_.find(entry.first);
            return ai.melee_quest->retired||!npcs_.contains(entry.first)||live==npc_combat_.end()||
                live->second.hex_scope()!=ai.policy.source;
        });
    }
    // Caller supplies reviewed combat and an explicit acquisition policy. No
    // appearance/level-based stat defaults and no new NPC lifetime are created.
    bool configure_npc_owned_ordinary_locked(skills::effects::Scope owner,unsigned quest,unsigned cycle,
            combat::ai::FlarePolicy policy,combat::ai::ordinary::Reviewed attack,
            combat::ai::FlareController::OwnedAcquisition acquisition,std::uint64_t now){
        using Acquisition=combat::ai::FlareController::OwnedAcquisition;
        if(!quest||!cycle||!policy.valid(true)||policy.target!=owner||!attack.valid()||
           (acquisition!=Acquisition::damage_only&&acquisition!=Acquisition::owner_proximity))return false;
        const auto player=players_.find(owner.actor);const auto live=npc_combat_.find(policy.source.actor);
        const auto placed=npcs_.find(policy.source.actor);const auto assignment=quest_npc_owners_.find(policy.source.actor);
        if(player==players_.end()||player->second.effect_scope!=owner||live==npc_combat_.end()||placed==npcs_.end()||
           live->second.hex_scope()!=policy.source||placed->second.definition.level!=policy.level||
           assignment==quest_npc_owners_.end()||assignment->second!=std::pair{owner,quest})return false;
        const auto objective=player->second.quest_kills.find(quest);
        if(objective==player->second.quest_kills.end()||objective->second.cycle!=cycle||
           !objective->second.remaining.contains(policy.source))return false;
        const auto life=live->second.lifetime();const auto state=live->second.actor(life);
        if(!state||!state->health||state->health->current<=0||!live->second.hostile_to_players(life)||
           now<live->second.health_clock(life).value_or(now))return false;
        const auto old=npc_flare_ai_.find(policy.source.actor);
        if(old!=npc_flare_ai_.end()){
            const auto ordinary=npc_ordinary_ai_.find(policy.source.actor);const auto& binding=old->second.melee_quest;
            return old->second.policy.source==policy.source&&now>=old->second.updated&&binding&&!binding->retired&&
                binding->owner==owner&&binding->quest==quest&&binding->cycle==cycle&&binding->acquisition==acquisition&&
                ordinary!=npc_ordinary_ai_.end()&&ordinary->second.source==policy.source;
        }
        if(npc_elemental_ai_.contains(policy.source.actor)||npc_ordinary_ai_.contains(policy.source.actor)||
           quest_npc_motion_.contains(policy.source.actor)||live->second.active_action(life,now)||
           !live->second.ordinary_action_ready(life,now))return false;
        combat::ai::FlareController motion{policy,placed->second.x,placed->second.y,placed->second.plane,now};
        const auto loss=live->second.health_loss_revision(life).value_or(0);
        motion.melee_quest=combat::ai::FlareController::MeleeQuest{owner,quest,cycle,state->health->current,loss!=0,false,loss,acquisition};
        npc_flare_ai_.emplace(policy.source.actor,std::move(motion));
        npc_ordinary_ai_.emplace(policy.source.actor,NpcOrdinaryController{policy.source,combat::ai::ordinary::Ledger(policy.source,std::move(attack)),{}});
        return true;
    }
public:
    bool configure_npc_owned_ordinary(skills::effects::Scope owner,unsigned quest,unsigned cycle,
            combat::ai::FlarePolicy policy,combat::ai::ordinary::Reviewed attack,
            combat::ai::FlareController::OwnedAcquisition acquisition,std::uint64_t now){
        std::lock_guard lock(mutex_);
        return configure_npc_owned_ordinary_locked(owner,quest,cycle,std::move(policy),std::move(attack),acquisition,now);
    }
    // Existing quest71 content only. Weapon/motion values are explicitly
    // authored development policy; Health80 is captured and armor120 wiki-backed.
    bool arm_rogue_bull_melee(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        if(!cycle)return false;std::lock_guard lock(mutex_);
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return false;
        const auto objective=player->second.quest_kills.find(71);
        if(objective==player->second.quest_kills.end()||objective->second.cycle!=cycle||objective->second.remaining.size()!=1)return false;
        const auto source=*objective->second.remaining.begin();const auto body=npcs_.find(source.actor);
        const auto live=npc_combat_.find(source.actor);
        if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source||live->second.lifetime().map!=146)return false;
        const auto& d=body->second.combat_definition;
        if(!d||d->binding.definition!=source.actor||d->binding.model_file!=116295||d->binding.level!=5||
           !d->spawn_health||d->spawn_health->maximum!=80||d->physical_armor!=120||d->earth_armor!=120||
           d->fire_armor!=120||d->cold_armor!=120||d->lightning_armor!=120||d->general_armor!=120)return false;
        combat::ai::FlarePolicy policy;policy.source=source;policy.target=owner;policy.level=5;
        policy.speed=220;policy.acquire=1000;policy.follow=1500;policy.leash=2000;policy.flight={100,32};
        policy.evidence="data/gw/quests/rogue-bull-melee-development-policy.txt";
        combat::ai::ordinary::Reviewed attack{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{100,32},policy.evidence};
        return configure_npc_owned_ordinary_locked(owner,71,cycle,std::move(policy),std::move(attack),
            combat::ai::FlareController::OwnedAcquisition::owner_proximity,now);
    }
    // Bounded quest75 host activation. All motion/attack numbers below are
    // authored playability tuning; only content identity and Health80 are
    // independently reviewed. No caster or resource pool is manufactured.
    bool arm_grawl_melee(skills::effects::Scope owner,unsigned cycle,std::uint64_t now){
        if(!cycle)return false;std::lock_guard lock(mutex_);
        const auto player=players_.find(owner.actor);
        if(player==players_.end()||player->second.effect_scope!=owner)return false;
        const auto objective=player->second.quest_kills.find(75);
        if(objective==player->second.quest_kills.end()||objective->second.cycle!=cycle)return false;
        std::vector<skills::effects::Scope> additions;
        for(const auto source:objective->second.remaining){
            const auto live=npc_combat_.find(source.actor);
            const auto placement=npcs_.find(source.actor);
            const auto assignment=quest_npc_owners_.find(source.actor);
            if(live==npc_combat_.end()||placement==npcs_.end()||live->second.hex_scope()!=source||
               assignment==quest_npc_owners_.end()||assignment->second!=std::pair{owner,75u}||
               live->second.lifetime().map!=160||!live->second.hostile_to_players(live->second.lifetime()))return false;
            const auto state=live->second.actor(live->second.lifetime());
            const auto& definition=placement->second.combat_definition;
            if(!definition||!definition->spawn_health||definition->spawn_health->maximum!=80)return false;
            if(!state||!state->health||state->health->current<=0)continue;
            if(const auto old=npc_flare_ai_.find(source.actor);old!=npc_flare_ai_.end()){
                if(old->second.policy.source!=source||!old->second.melee_quest||old->second.melee_quest->owner!=owner||
                   old->second.melee_quest->quest!=75||old->second.melee_quest->cycle!=cycle)return false;
                continue;
            }
            if(npc_elemental_ai_.contains(source.actor)||npc_ordinary_ai_.contains(source.actor)||quest_npc_motion_.contains(source.actor))return false;
            additions.push_back(source);
        }
        // The production poll normally finds an already-bound encounter.
        // Validate exact identities above without copying every AI ledger.
        if(additions.empty())return true;
        auto controllers=npc_flare_ai_;auto ordinary=npc_ordinary_ai_;
        for(const auto source:additions){
            const auto placement=npcs_.find(source.actor);
            combat::ai::FlarePolicy policy;policy.source=source;policy.target=owner;policy.level=placement->second.definition.level;
            policy.speed=220;policy.acquire=1000;policy.follow=1500;policy.leash=2000;policy.flight={100,32};
            policy.evidence="data/gw/quests/grawl-melee-development-policy.txt";
            combat::ai::ordinary::Reviewed profile{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{100,32},policy.evidence};
            if(!policy.valid()||!profile.valid())return false;
            combat::ai::FlareController controller{policy,placement->second.x,placement->second.y,placement->second.plane,now};
            controller.melee_quest=combat::ai::FlareController::MeleeQuest{owner,75,cycle,80};
            controllers.emplace(source.actor,std::move(controller));
            ordinary.emplace(source.actor,NpcOrdinaryController{source,combat::ai::ordinary::Ledger(source,std::move(profile)),{}});
        }
        npc_flare_ai_.swap(controllers);npc_ordinary_ai_.swap(ordinary);return true;
    }
    std::optional<combat::ai::FlareController> npc_flare_ai_snapshot(skills::effects::Scope source)const{
        std::lock_guard lock(mutex_);const auto found=npc_flare_ai_.find(source.actor);
        return found!=npc_flare_ai_.end()&&found->second.policy.source==source?std::optional(found->second):std::nullopt;
    }
    std::optional<combat::ai::FlareController::Movement> npc_pursuit_movement_snapshot(skills::effects::Scope source)const{
        std::lock_guard lock(mutex_);const auto live=npc_combat_.find(source.actor);
        if(live==npc_combat_.end()||live->second.hex_scope()!=source)return {};
        const auto flare=npc_flare_ai_.find(source.actor);
        if(flare!=npc_flare_ai_.end()&&flare->second.policy.source==source)return flare->second.last_movement;
        const auto elemental=npc_elemental_ai_.find(source.actor);
        return elemental!=npc_elemental_ai_.end()&&elemental->second.motion.policy.source==source?
            elemental->second.motion.last_movement:std::nullopt;
    }
    bool configure_npc_flare_ai(combat::ai::FlarePolicy policy,std::uint64_t now,std::optional<skills::effects::AuraRestorationPlan> aura={}){
        if(!policy.valid())return false;std::lock_guard lock(mutex_);
        const auto actor=npc_combat_.find(policy.source.actor);const auto body=npcs_.find(policy.source.actor);
        std::vector<combat::ai::ReviewedSpell> book{{{194,0},1248,policy.evidence}};
        if(aura){const auto canonical=skills::effects::aura_restoration_plan(aura->rank);
            if(!canonical||*canonical!=*aura||actor==npc_combat_.end()||actor->second.healing_allowed(actor->second.lifetime())!=std::optional(true))return false;
            book.insert(book.begin(),{{180,0},0,policy.evidence,combat::ai::SpellTargetClass::self});}
        if(actor==npc_combat_.end()||body==npcs_.end()||actor->second.hex_scope()!=policy.source||
           policy.level!=body->second.definition.level||
           quest_npc_owners_.contains(policy.source.actor)||quest_npc_motion_.contains(policy.source.actor)||
           now<actor->second.health_clock(actor->second.lifetime()).value_or(now)||
           actor->second.active_action(actor->second.lifetime(),now)||
           (npc_flare_ai_.contains(policy.source.actor)&&npc_flare_ai_.at(policy.source.actor).policy.source==policy.source&&
            (npc_flare_ai_.at(policy.source.actor).cast||!npc_flare_ai_.at(policy.source.actor).shots.empty()))||
           !actor->second.hostile_to_players(actor->second.lifetime())||!actor->second.energy(actor->second.lifetime())||
           !actor->second.configure_spellbook(actor->second.lifetime(),std::move(book)))return false;
        npc_flare_ai_.insert_or_assign(policy.source.actor,combat::ai::FlareController{policy,body->second.x,body->second.y,body->second.plane,now});return true;
    }
