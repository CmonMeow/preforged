// Included inside world::Instance AFTER gw_world_npc_flare.inl.
private:
    struct NpcElementalController {
        combat::ai::FlareController motion;
        combat::ai::elemental::Ledger ledger;
        struct OwnedQuest {skills::effects::Scope owner;unsigned quest=0,cycle=0;double health=0;bool triggered=false,retired=false;std::uint64_t health_loss_revision=0;};
        std::optional<OwnedQuest> owned_quest;
    };
    std::map<unsigned,NpcElementalController> npc_elemental_ai_;
    static std::uint64_t npc_elemental_player_clock(const Player& p){
        return std::max({p.health_updated,p.energy_updated,p.last_move,p.steering?p.steering_tick:0});
    }
    void resolve_npc_elemental_releases_locked(NpcElementalController& ai,std::uint64_t now){
        if(!timed_effect_boundary_){(void)settle_timed_effects_locked(now);return;}
        namespace elemental=combat::ai::elemental;
        const auto source=ai.motion.policy.source;
        const auto current=npc_combat_.find(source.actor);
        const auto current_scope=current==npc_combat_.end()?skills::effects::Scope{}:current->second.hex_scope();
        for(const auto id:ai.ledger.released_ids()){
            // Read the immutable release without consuming the no-contact probe.
            const auto probe=ai.ledger.prepare_impact(id,current_scope,{},now);
            const auto& shot=probe.release;if(now<shot.arrives)continue;
            if(current_scope!=source){ai.ledger.commit_impact(probe);continue;}
            std::vector<unsigned> candidates;bool future=false;
            for(auto&[actor,p]:players_){
                if(!p.npc_publication_ready)continue;
                const bool primary=!shot.cast.use.plan.caster_center()&&p.effect_scope==shot.cast.use.target;
                if(!primary&&!shot.cast.use.plan.radius())continue;
                // Query movement geometry before unrelated Health/Energy
                // clocks. A far-away, queryable player cannot delay this blast.
                if(now<p.last_move||(p.steering&&now<p.steering_tick)){future=true;break;}
                if(!primary&&p.surface.plane!=shot.plane)continue;
                const auto position=healing_position_locked(p,now);
                if(!position){future=true;break;}
                if(!primary&&std::hypot(position->x-shot.aim_x,position->z-shot.aim_y)>shot.cast.use.plan.radius())continue;
                if(npc_elemental_player_clock(p)>now){future=true;break;}
                candidates.push_back(actor);
            }
            if(future)continue;
            // Finish all due Health/lifetime reactions before snapshotting any
            // area recipient; ending one caster's effect can change another.
            bool ready=true;
            for(const auto actor:candidates)if(!advance_health_locked(players_.at(actor),now)){ready=false;break;}
            if(!ready)continue;
            std::vector<elemental::Recipient> contacts;
            for(const auto actor:candidates){auto& p=players_.at(actor);
                if(!p.resources){ready=false;break;}
                const auto point=healing_position_locked(p,now);if(!point){ready=false;break;}
                const auto body=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
                    combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,{},p.reviewed_fire_armor,p.reviewed_cold_armor},now);
                contacts.push_back({{p.effect_scope,point->x,point->z,unsigned(p.surface.plane),p.precise_health>0,npc_elemental_player_clock(p)},body,p.conditions,true,player_damage_projection_locked(p,now)});
            }
            if(!ready)continue;
            // Health/lifetime settlement above can retire another actor. Do
            // not authenticate a release using the pre-settlement generation.
            const auto refreshed=npc_combat_.find(source.actor);
            const auto refreshed_scope=refreshed==npc_combat_.end()?skills::effects::Scope{}:refreshed->second.hex_scope();
            const auto proposal=ai.ledger.prepare_impact(id,refreshed_scope,contacts,now);
            if(proposal.status==elemental::ImpactStatus::deferred||proposal.status==elemental::ImpactStatus::unsupported||proposal.status==elemental::ImpactStatus::invalid)continue;
            for(const auto& contact:proposal.hits){const auto found=players_.find(contact.target.actor);
                if(found==players_.end()||found->second.effect_scope!=contact.target||
                   !skills::effects::effect_instance_capacity(found->second.next_health_effect,contact.result.additions.size())||
                   contact.result.hits.size()!=1||!prepare_player_reactive_seed_locked(found->second,contact.result.hits.front(),now)){ready=false;break;}}
            if(!ready)continue;
            if(!ai.ledger.commit_impact(proposal))throw std::logic_error("preflighted NPC elemental release changed");
            // Preserve selected contacts, but refresh recipient authority after
            // earlier contacts' reactions. Reviewed NPC plans currently deliver
            // one damage slice per recipient; no aggregate Health is restored.
            for(const auto& contact:proposal.hits){auto& p=players_.at(contact.target.actor);
                if(p.effect_scope!=contact.target||p.precise_health<=0)continue;
                const auto body=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
                    combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,{},p.reviewed_fire_armor,p.reviewed_cold_armor},now);
                const auto result=combat::ai::offensive::target(shot.cast.use.plan,source,p.effect_scope,body,p.conditions,
                    ai.motion.policy.level,now,player_damage_projection_locked(p,now));
                if(!result.accepted||result.hits.size()!=1||
                   !skills::effects::effect_instance_capacity(p.next_health_effect,result.additions.size()))continue;
                const auto seed=prepare_player_reactive_seed_locked(p,result.hits.front(),now);
                if(!seed)continue;
                if(const auto visual=elemental::wire::impact_body(p.id,source.actor,shot.cast.use.plan.skill()))
                    for(auto&[peer_id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({*visual});
                if(!commit_player_reactive_hit_locked(p,result.hits.front(),source.actor,now,true,&*seed,true))
                    throw std::logic_error("preflighted NPC elemental damage changed");
                if(p.precise_health>0&&!result.additions.empty()){
                    auto merged=p.conditions;bool valid=true;
                    for(const auto kind:result.additions){
                        const auto plan=shot.cast.use.plan.elemental();
                        if(!plan||kind!=combat::conditions::Kind::burning||
                           !merged.apply(kind,source.actor,now,plan->burning_ms)){valid=false;break;}
                    }
                    if(valid)commit_conditions_locked(p,merged,result.additions,now);
                }
                if(p.passive_recovery){p.passive_recovery->record(recovery::Event::targeted_by_enemy_spell_or_signet,now);}
            }
            for(const auto& contact:proposal.hits){auto& p=players_.at(contact.target.actor);
                if(p.effect_scope!=contact.target)continue;
                if(p.precise_health<=0)kill_locked(p,now);
                publish_health_locked(p);
            }
        }
    }
    void drive_npc_elemental_locked(std::uint64_t now){
        namespace elemental=combat::ai::elemental;
        for(auto&[id,ai]:npc_elemental_ai_){
            auto& motion=ai.motion;if(now<motion.updated)continue;
            suspend_busy_npc_wander_locked(motion,now);
            resolve_npc_elemental_releases_locked(ai,now); // already released shots survive source Health loss
            const auto placed=npcs_.find(id);const auto found=npc_combat_.find(id);
            if(placed==npcs_.end()||found==npc_combat_.end()||found->second.hex_scope()!=motion.policy.source){retire_npc_ordinary_locked(motion.policy.source,now);continue;}
            auto& body=placed->second;auto& actor=found->second;const auto life=actor.lifetime();
            if(ai.owned_quest&&!gate_npc_owned_caster_locked(ai,body,actor,now)){motion.updated=now;continue;}
            if(npc_self_enchantment_casts_.contains(id)){motion.updated=now;continue;}
            const auto replace_ordinary=[&](const combat::ai::ordinary::Windup& pending){
                if(ai.ledger.pending()||pending.target!=motion.policy.target)return false;
                const combat::ai::Spellbook::Availability available=[&](skills::Slot slot,std::uint64_t at)->std::optional<std::uint64_t>{
                    if(!ai.ledger.admission_ready(at))return {};
                    const auto target=players_.find(pending.target.actor);const auto source=npc_combat_.find(id);const auto plan=ai.ledger.plan(slot);
                    if(!plan||target==players_.end()||target->second.effect_scope!=pending.target||
                       source==npc_combat_.end()||source->second.hex_scope()!=motion.policy.source||
                       (plan->kind()==combat::Kind::cold?!target->second.reviewed_cold_armor:!target->second.reviewed_fire_armor)||
                       (slot.skill==214&&!source->second.observed_overcast(source->second.lifetime(),at)))return {};
                    return skill_disabled_slot_locked(motion.policy.source,slot,at);
                };
                auto prepared=prepare_npc_ordinary_replacement_locked(motion.policy.source,pending,now,available);
                if(!prepared)return false;
                auto effect=ai.ledger;const auto token=prepared->spell.spell;
                const auto reservation=effect.reserve(*token.skill,prepared->facts,now);
                if(!reservation||!effect.accept(*reservation,token,{prepared->spell.before.current,prepared->spell.after.current,
                    prepared->spell.after.maximum,now,token.sequence}))return false;
                if(!commit_npc_ordinary_replacement_locked(motion.policy.source,*prepared))return false;
                ai.ledger=std::move(effect);observe_npc_replacement_energy_locked(motion.policy.source,*prepared);
                const auto refreshed=npc_combat_.find(id);
                if(refreshed!=npc_combat_.end()&&refreshed->second.hex_scope()==motion.policy.source&&
                   refreshed->second.owns_spell_action(refreshed->second.lifetime(),token)){
                    const auto alive=refreshed->second.actor(refreshed->second.lifetime());
                    if(alive&&alive->health&&alive->health->current>0){
                        npc_flare_event_locked(60,id,token.skill->skill,reservation->plan.caster_center()?0:pending.target.actor);return true;
                    }
                }
                ai.ledger.cancel(token,now);return true;
            };
            if(drive_npc_ordinary_locked(motion.policy.source,body,actor,now,replace_ordinary)){motion.updated=now;continue;}
            if(now<actor.health_clock(life).value_or(now)){motion.updated=now;continue;}
            const auto state=actor.actor(life);const bool alive=state&&state->health&&state->health->current>0&&actor.hostile_to_players(life);
            if(!alive){if(ai.ledger.pending())ai.ledger.cancel(ai.ledger.pending()->token,now);motion.engaged=false;
                if(motion.encounter)motion.encounter->clear();motion.updated=now;continue;}
            if(ai.ledger.pending()){
                const auto paid=*ai.ledger.pending();const auto target=players_.find(paid.use.target.actor);
                if(actor.control_epoch(life)!=paid.token.epoch){ai.ledger.cancel(paid.token,now);npc_flare_event_locked(59,id);motion.updated=now;continue;}
                if(!paid.use.plan.caster_center()&&target!=players_.end()&&target->second.effect_scope==paid.use.target&&npc_elemental_player_clock(target->second)>now){motion.updated=now;continue;}
                if(now<paid.token.completes){motion.updated=now;continue;}
                const auto failure=npc_failed_target_locked(paid.token,paid.use.target,body,actor,now);
                if(failure.status==combat::ai::failed_target::Status::deferred){motion.updated=now;continue;}
                if(failure.status==combat::ai::failed_target::Status::failed){
                    if(actor.fail_target_action(life,paid.token,now)){
                        if(!ai.ledger.cancel(paid.token,now))throw std::logic_error("preflighted NPC failed target changed");
                        npc_flare_event_locked(59,id);
                    }
                    motion.updated=now;continue;
                }
                const auto overcast=paid.use.slot.skill==214?actor.observed_overcast(life,now):std::optional<unsigned>{0};
                if(!overcast){motion.updated=now;continue;}
                elemental::Pose destination{paid.use.target,0,0,body.plane,false,0};bool sight=false;
                if(target!=players_.end()&&target->second.effect_scope==paid.use.target){
                    const auto point=healing_position_locked(target->second,now);
                    if(point){destination={paid.use.target,point->x,point->z,unsigned(target->second.surface.plane),target->second.precise_health>0,npc_elemental_player_clock(target->second)};
                        sight=ordinary_npc_snapshot_locked(target->second,body,actor,now).line_of_sight;}
                }
                const auto proposal=ai.ledger.prepare_release(paid.token,{motion.policy.source,body.x,body.y,body.plane,true,actor.health_clock(life).value_or(now)},
                    destination,paid.token.epoch,sight,now);
                if(proposal.status!=elemental::ReleaseStatus::ready&&proposal.status!=elemental::ReleaseStatus::stopped){motion.updated=now;continue;}
                const bool grant_regeneration=paid.use.slot.skill==214&&*overcast>0&&proposal.status==elemental::ReleaseStatus::ready;
                if(grant_regeneration){
                    auto preview=actor;
                    if(next_npc_hex_==UINT32_MAX||!preview.apply_ice_spear_regeneration(life,*paid.use.plan.ice_spear(),next_npc_hex_,now)){motion.updated=now;continue;}
                }
                const auto aura=prepare_npc_aura_completion_locked(paid.token,now);
                const auto terminal=complete_npc_spell_terminal_locked(paid.token,now,proposal.status==elemental::ReleaseStatus::ready);
                if(terminal==NpcSpellTerminal::unavailable){motion.updated=now;continue;}
                npc_flare_event_locked(58,id);
                const bool survived=terminal==NpcSpellTerminal::completed;
                if(survived&&grant_regeneration){
                    const auto old=actor.ice_spear_regeneration(life,now);
                    if(!actor.apply_ice_spear_regeneration(life,*paid.use.plan.ice_spear(),next_npc_hex_,now))throw std::logic_error("preflighted NPC Ice Spear regeneration changed");
                    ++next_npc_hex_;const auto entry=*actor.ice_spear_regeneration(life,now);
                    for(auto&[peer_id,peer]:players_)if(peer.npc_publication_ready){
                        if(old)peer.outbox->enqueue({health_effect_remove(id,old->instance)});
                        peer.outbox->enqueue({health_effect_add(entry,id,now),emotes::effects_2022(id,actor.actor(life)->effects)});
                    }
                }
                if(!ai.ledger.commit_release(proposal,survived))throw std::logic_error("preflighted NPC elemental completion changed");
                if(survived&&proposal.release&&paid.use.slot.skill==214){
                    const auto& shot=*proposal.release;
                    const auto packet=skills::particles::spell_projectile_launch_2022(id,shot.aim_x,shot.aim_y,
                        static_cast<std::uint16_t>(shot.plane),float(shot.arrives-shot.released)/1000.f,214);
                    for(auto&[peer_id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({packet});
                }
                if(survived&&proposal.status==elemental::ReleaseStatus::ready)
                    if(const auto visual=elemental::wire::release_body(id,paid.use.slot.skill))
                        for(auto&[peer_id,peer]:players_)if(peer.npc_publication_ready)peer.outbox->enqueue({*visual});
                if(survived&&proposal.status==elemental::ReleaseStatus::ready){
                    finish_npc_aura_completion_locked(aura,now);
                    if(!succor_npc_spell_completed_locked(paid.token,now))throw std::logic_error("NPC elemental Succor completion failed");
                    std::string reason;
                    if(!completed_skill_action_locked(motion.policy.source,paid.use.slot.skill,now,&reason))
                        std::osyncstream(std::clog)<<"WORLD npc_spell_completion actor="<<id<<" reason="<<reason<<'\n';
                }
                resolve_npc_elemental_releases_locked(ai,now);motion.updated=now;continue;
            }
            if(actor.knocked_down(life,now)||actor.active_action(life,now)){motion.updated=now;continue;}
            auto victim=players_.find(motion.policy.target.actor);
            const auto valid=[&]{return victim!=players_.end()&&victim->second.effect_scope==motion.policy.target&&victim->second.npc_publication_ready&&
                victim->second.precise_health>0&&victim->second.surface.plane==body.plane;};
            if(!motion.returning&&victim!=players_.end()&&victim->second.effect_scope==motion.policy.target&&npc_elemental_player_clock(victim->second)>now){motion.updated=now;continue;}
            if(ai.owned_quest){
                if(!select_npc_owned_caster_locked(ai,body,actor,now)){motion.updated=now;continue;}
            }else{
            bool lost_target=motion.encounter&&motion.encounter->size()!=0;
            if(motion.engaged){const auto point=valid()?healing_position_locked(victim->second,now):std::nullopt;
                if(std::hypot(body.x-motion.home_x,body.y-motion.home_y)>motion.policy.leash){motion.engaged=false;motion.returning=true;}
                else if(!point||std::hypot(point->x-body.x,point->z-body.y)>motion.policy.follow){motion.engaged=false;lost_target=true;}}
            if(motion.returning){suspend_busy_npc_wander_locked(motion,now);if(motion.encounter)motion.encounter->clear();
                if(move_npc_flare_locked(motion,body,{motion.home_x,motion.home_y},1,now))motion.returning=false;motion.updated=now;continue;}
            const auto candidates=npc_caster_candidate_locked(motion,body,now);
            if(!candidates){motion.updated=now;continue;}
            if(!motion.engaged){
                // A rejected target cannot own future chase travel. Retire
                // the old route now, including unknown remembered poses; a
                // later home/retarget decision installs its own future route.
                if(!candidates->target){if(lost_target||candidates->engaged)stop_npc_pursuit_locked(motion.policy.source,now);motion.returning=lost_target&&!candidates->engaged;suspend_busy_npc_wander_locked(motion,now);idle_npc_wander_locked(motion,body,now);motion.updated=now;continue;}
                victim=players_.find(candidates->target->actor);motion.policy.target=*candidates->target;motion.engaged=true;
            }
            }
            suspend_busy_npc_wander_locked(motion,now);
            if(!valid()){motion.updated=now;continue;}
            const auto point=healing_position_locked(victim->second,now);if(!point){motion.updated=now;continue;}
            if(ai.owned_quest&&!actor.aura_restoration_entry(life,now)){
                if(begin_npc_self_enchantment_locked(motion.policy.source,*skills::effects::aura_restoration_plan(0),now)){motion.updated=now;continue;}
            }
            const auto snapshot=ordinary_npc_snapshot_locked(victim->second,body,actor,now);
            if(!settle_mind_wrack_caster_locked(motion.policy.source,now)){motion.updated=now;continue;}
            combat::ai::SpellTarget facts{victim->second.effect_scope,float(snapshot.distance),true,true,true,snapshot.line_of_sight};
            const combat::ai::Spellbook::Availability supported=[&](skills::Slot slot,std::uint64_t at)->std::optional<std::uint64_t>{
                const auto plan=ai.ledger.plan(slot);
                if(!plan||(plan->kind()==combat::Kind::cold?!victim->second.reviewed_cold_armor:!victim->second.reviewed_fire_armor)||
                   (slot.skill==214&&!actor.observed_overcast(life,at)))return {};
                return skill_disabled_slot_locked(motion.policy.source,slot,at);
            };
            const combat::ai::Spellbook::Availability ready=[&](skills::Slot slot,std::uint64_t at)->std::optional<std::uint64_t>{
                return ai.ledger.admission_ready(at)?supported(slot,at):std::nullopt;
            };
            const auto choice=actor.choose_spell(life,facts,now,ready);
            if(!choice){
                if(begin_npc_ordinary_locked(motion,body,actor,victim->second,now)){motion.updated=now;continue;}
                // Foe_behavior oldid2656731 documents moving close for PBAoE.
                // Preserve authored order, but let an actually in-range spell
                // win before approaching. This hypothetical geometry is only
                // a readiness probe; reserve/payment always use actual facts.
                auto approach=facts;approach.distance=0;approach.line_of_sight=true;
                auto next=actor.choose_spell(life,approach,now,ready);
                if(!next){
                    // Authored pursuit policy: a known supported skill's reach
                    // remains useful geometry during recharge, empty Energy or
                    // a full projectile ledger. This does not reserve/pay/cast.
                    // Unknown armor/Overcast/disable observations still refuse.
                    for(const auto& entry:ai.ledger.reviewed_spells())
                        if(supported(entry.slot,now)){next=entry.slot;break;}
                }
                if(next){const auto plan=ai.ledger.plan(*next);
                    if(plan)move_npc_flare_locked(motion,body,*point,snapshot.line_of_sight?
                        (plan->caster_center()?plan->range():std::min(1200.f,plan->range())):0,now);
                }
                motion.updated=now;continue;
            }
            stop_npc_pursuit_locked(motion.policy.source,now);
            const auto reservation=ai.ledger.reserve(*choice,facts,now);if(!reservation){motion.updated=now;continue;}
            const auto before=actor.energy(life);const auto units=actor.precise_energy_units(life);
            // reserve already owns this admission's capacity. Revalidate the
            // external effect/disable facts without rejecting our own held
            // reservation as though this were another new selection.
            const auto token=actor.begin_spell(life,*choice,facts,now,supported);
            if(!token){ai.ledger.abort(*reservation);motion.updated=now;continue;}
            const auto after=actor.energy(life);
            if(!before||!after||!ai.ledger.accept(*reservation,*token,{before->current,after->current,after->maximum,now,token->sequence}))throw std::logic_error("preflighted NPC elemental payment changed");
            observe_npc_mind_wrack_energy_locked(motion.policy.source,{before->current,before->maximum,0,units?unsigned(*units%300000):0},now);
            const auto current=actor.actor(life);
            if(!current||!current->health||current->health->current<=0)ai.ledger.cancel(*token,now);
            else npc_flare_event_locked(60,id,choice->skill,reservation->plan.caster_center()?0:victim->first);
            motion.updated=now;
        }
        std::erase_if(npc_elemental_ai_,[&](const auto& entry){
            const auto& ai=entry.second;if(!ai.owned_quest)return false;
            const auto live=npc_combat_.find(entry.first);
            return !npcs_.contains(entry.first)||live==npc_combat_.end()||live->second.hex_scope()!=ai.motion.policy.source||
                (ai.owned_quest->retired&&ai.ledger.in_flight()==0);
        });
    }
public:
    bool observe_npc_overcast(skills::effects::Scope source,unsigned amount,std::uint64_t now,std::string evidence){
        std::lock_guard lock(mutex_);const auto found=npc_combat_.find(source.actor);
        return found!=npc_combat_.end()&&found->second.hex_scope()==source&&
            found->second.observe_overcast(found->second.lifetime(),amount,now,std::move(evidence));
    }
    struct NpcElementalAiSnapshot {
        skills::effects::Scope target;bool engaged=false,returning=false;std::size_t flights=0;
        std::optional<combat::ai::elemental::PaidCast> pending;
    };
    std::optional<NpcElementalAiSnapshot> npc_elemental_ai_snapshot(skills::effects::Scope source)const{
        std::lock_guard lock(mutex_);const auto found=npc_elemental_ai_.find(source.actor);
        if(found==npc_elemental_ai_.end()||found->second.motion.policy.source!=source)return {};
        const auto& ai=found->second;return NpcElementalAiSnapshot{ai.motion.policy.target,ai.motion.engaged,ai.motion.returning,ai.ledger.in_flight(),ai.ledger.pending()};
    }
    bool configure_npc_elemental_ai(combat::ai::FlarePolicy policy,std::vector<combat::ai::elemental::Reviewed> spells,std::size_t capacity,std::uint64_t now,
            std::optional<skills::effects::AuraRestorationPlan> aura={}){
        if(!policy.valid())return false;
        std::optional<combat::ai::elemental::Ledger> proposed;
        try{proposed.emplace(policy.source,policy.level,std::move(spells),policy.flight,capacity);}catch(const std::invalid_argument&){return false;}
        std::lock_guard lock(mutex_);const auto actor=npc_combat_.find(policy.source.actor);const auto body=npcs_.find(policy.source.actor);
        auto book=proposed->reviewed_spells();
        if(aura){const auto canonical=skills::effects::aura_restoration_plan(aura->rank);
            if(!canonical||*canonical!=*aura||actor==npc_combat_.end()||actor->second.healing_allowed(actor->second.lifetime())!=std::optional(true))return false;
            book.insert(book.begin(),{{180,0},0,policy.evidence,combat::ai::SpellTargetClass::self});}
        if(actor==npc_combat_.end()||body==npcs_.end()||actor->second.hex_scope()!=policy.source||policy.level!=body->second.definition.level||
           (npc_flare_ai_.contains(policy.source.actor)&&npc_flare_ai_.at(policy.source.actor).policy.source==policy.source)||
           (npc_elemental_ai_.contains(policy.source.actor)&&npc_elemental_ai_.at(policy.source.actor).motion.policy.source==policy.source)||
           quest_npc_owners_.contains(policy.source.actor)||quest_npc_motion_.contains(policy.source.actor)||
           now<actor->second.health_clock(actor->second.lifetime()).value_or(now)||actor->second.active_action(actor->second.lifetime(),now)||
           !actor->second.hostile_to_players(actor->second.lifetime())||!actor->second.energy(actor->second.lifetime())||
           !actor->second.configure_spellbook(actor->second.lifetime(),std::move(book)))return false;
        // Only after the new lifetime's complete preflight may its controller
        // replace retired work. Old generation flights can never own new hits.
        npc_ai_motion_.erase(policy.source.actor);
        npc_immediate_ai_.erase(policy.source.actor);
        npc_owned_immediate_.erase(policy.source.actor);
        npc_flare_ai_.erase(policy.source.actor);
        npc_elemental_ai_.insert_or_assign(policy.source.actor,NpcElementalController{{policy,body->second.x,body->second.y,body->second.plane,now},std::move(*proposed)});return true;
    }
