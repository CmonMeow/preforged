// Included inside world::Instance before the two caster drivers.
private:
    struct NpcOrdinaryController {
        skills::effects::Scope source;
        combat::ai::ordinary::Ledger ledger;
        std::map<std::uint64_t,std::uint64_t> events;
    };
    std::map<unsigned,NpcOrdinaryController> npc_ordinary_ai_;
    struct NpcOrdinaryReplacement {
        combat::ai::replacement::Prepared spell;
        combat::ai::ordinary::Ledger ordinary;
        combat::ai::SpellTarget facts;
        std::optional<combat::npcs::Energy> observed_before;
        std::optional<std::int64_t> observed_units;
    };
    std::optional<NpcOrdinaryReplacement> prepare_npc_ordinary_replacement_locked(skills::effects::Scope source,
        const combat::ai::ordinary::Windup& pending,std::uint64_t now,const combat::ai::Spellbook::Availability& available){
        namespace replacement=combat::ai::replacement;
        const auto facts=[&]()->std::optional<combat::ai::SpellTarget>{
            const auto target=npc_ordinary_target_locked(pending.target,now);
            const auto body=npcs_.find(source.actor);const auto live=npc_combat_.find(source.actor);
            const auto player=players_.find(pending.target.actor);
            if(body==npcs_.end()||live==npc_combat_.end()||live->second.hex_scope()!=source||
               player==players_.end()||player->second.effect_scope!=pending.target||!target.observed||!target.alive||target.clock>now)return {};
            const auto snapshot=ordinary_npc_snapshot_locked(player->second,body->second,live->second,now);
            return combat::ai::SpellTarget{pending.target,float(snapshot.distance),true,true,target.plane==body->second.plane,snapshot.line_of_sight};
        };
        auto target=facts();auto live=npc_combat_.find(source.actor);
        if(!target||live==npc_combat_.end()||live->second.hex_scope()!=source)return {};
        const auto life=live->second.lifetime();
        // The preview never cancels/pays/publishes against the real source.
        if(!replacement::prepare(live->second,life,pending.token,pending.target,*target,now,available))return {};
        if(!settle_mind_wrack_caster_locked(source,now))return {};
        live=npc_combat_.find(source.actor);if(live==npc_combat_.end()||!live->second.matches(life))return {};
        advance_npc_conditions_locked(source.actor,live->second,now);
        target=facts();live=npc_combat_.find(source.actor);
        const auto ordinary=npc_ordinary_ai_.find(source.actor);
        if(!target||live==npc_combat_.end()||!live->second.matches(life)||ordinary==npc_ordinary_ai_.end()||
           ordinary->second.source!=source||!ordinary->second.ledger.pending()||*ordinary->second.ledger.pending()!=pending)return {};
        const auto before=live->second.energy(life);const auto units=live->second.precise_energy_units(life);
        auto prepared=replacement::prepare(live->second,life,pending.token,pending.target,*target,now,available);
        if(!prepared)return {};
        auto ledger=ordinary->second.ledger;if(!ledger.cancel(now))return {};
        return NpcOrdinaryReplacement{std::move(*prepared),std::move(ledger),*target,before,units};
    }
    bool commit_npc_ordinary_replacement_locked(skills::effects::Scope source,NpcOrdinaryReplacement& prepared){
        const auto live=npc_combat_.find(source.actor);const auto ordinary=npc_ordinary_ai_.find(source.actor);
        if(live==npc_combat_.end()||ordinary==npc_ordinary_ai_.end()||ordinary->second.source!=source||
           !ordinary->second.ledger.pending()||ordinary->second.ledger.pending()->token!=prepared.spell.ordinary||
           ordinary->second.ledger.pending()->target!=prepared.spell.target||
           !combat::ai::replacement::current(prepared.spell,live->second,live->second.lifetime(),prepared.facts.scope,prepared.spell.at))return false;
        // No callbacks between fresh preparation and these paired commits.
        live->second=std::move(prepared.spell.actor);ordinary->second.ledger=std::move(prepared.ordinary);
        // Outcomes are created at release: this cancelled windup owns none.
        // Existing flights and their outcome records remain in the copied ledger.
        npc_ordinary_packet_locked(combat::weapon::stop_2022(source.actor));
        return true;
    }
    void observe_npc_replacement_energy_locked(skills::effects::Scope source,const NpcOrdinaryReplacement& prepared){
        if(prepared.observed_before)observe_npc_mind_wrack_energy_locked(source,
            {prepared.observed_before->current,prepared.observed_before->maximum,0,
                prepared.observed_units?unsigned(*prepared.observed_units%300000):0},prepared.spell.at);
    }
    void retire_npc_ordinary_locked(skills::effects::Scope source,std::uint64_t now){
        const auto found=npc_ordinary_ai_.find(source.actor);
        if(found==npc_ordinary_ai_.end()||found->second.source!=source)return;
        for(const auto& [id,event]:found->second.events){const auto probe=found->second.ledger.prepare_impact(id,{},true,now);
            if(probe.release.attack.id)finish_attack_outcome_locked(source,probe.release.attack.target,event,AttackOutcome::cancelled,now);}
        // Retired identity cannot own future hits. Do not stop the animation
        // of a replacement actor that happens to reuse the same numeric ID.
        npc_ordinary_ai_.erase(found);
    }
    void npc_ordinary_packet_locked(const Packet& packet){
        for(auto& [id,p]:players_)if(p.npc_publication_ready)p.outbox->enqueue({packet});
    }
    combat::ai::ordinary::Pose npc_ordinary_target_locked(skills::effects::Scope scope,std::uint64_t now){
        combat::ai::ordinary::Pose result{scope};const auto it=players_.find(scope.actor);
        if(it==players_.end()){
            auto live=npc_combat_.find(scope.actor);auto body=npcs_.find(scope.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=scope)return result;
            const auto life=live->second.lifetime();result.clock=std::max(live->second.health_clock(life).value_or(0),live->second.energy_clock(life).value_or(0));
            if(result.clock>now){result.observed=false;return result;}
            advance_npc_conditions_locked(scope.actor,live->second,now);refresh_npc_movement_locked(scope.actor,now);
            live=npc_combat_.find(scope.actor);body=npcs_.find(scope.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||!live->second.matches(life))return result;
            const auto* actor=live->second.actor(life);result.x=body->second.x;result.y=body->second.y;result.plane=body->second.plane;
            result.clock=live->second.health_clock(life).value_or(now);result.alive=actor&&actor->health&&actor->health->current>0;return result;
        }
        if(it==players_.end()||it->second.effect_scope!=scope)return result;
        auto& p=it->second;result.clock=std::max({p.health_updated,p.energy_updated,p.last_move,p.steering?p.steering_tick:0});
        if(result.clock>now)return result;
        if(!advance_health_locked(p,now)){result.observed=false;return result;}
        const auto point=healing_position_locked(p,now);if(!point){result.observed=false;return result;}
        result.x=point->x;result.y=point->z;result.plane=p.surface.plane;result.alive=p.npc_publication_ready&&p.precise_health>0;
        return result; // Authored stationary aim; no inferred client velocity.
    }
    std::optional<int> npc_ordinary_armor_locked(const Player& p,unsigned type,bool sample){
        const auto* locations=type==3?(p.reviewed_cold_locations?&*p.reviewed_cold_locations:nullptr):
            type==4?(p.reviewed_lightning_locations?&*p.reviewed_lightning_locations:nullptr):
            type==5?(p.reviewed_fire_locations?&*p.reviewed_fire_locations:nullptr):
            type==11?(p.reviewed_earth_locations?&*p.reviewed_earth_locations:nullptr):
            (p.reviewed_physical_locations?&*p.reviewed_physical_locations:nullptr);
        if(locations){if(!sample)return 0;return energy::armor_hit(*locations,armor_location_draw_?armor_location_draw_():std::uniform_int_distribution<unsigned>(0,7)(armor_location_rng_));}
        return type==3?p.reviewed_cold_armor:type==4?p.reviewed_lightning_armor:type==5?p.reviewed_fire_armor:
            type==11?p.reviewed_earth_armor:p.reviewed_physical_armor;
    }
    void resolve_npc_ordinary_locked(NpcOrdinaryController& ai,Npc& body,combat::npcs::LiveActor& live,std::uint64_t now){
        if(!timed_effect_boundary_){(void)settle_timed_effects_locked(now);return;}
        namespace ordinary=combat::ai::ordinary;
        for(const auto id:ai.ledger.released_ids()){
            const auto released=ai.ledger.released(id);
            if(!released||npc_ordinary_observer_frontier_locked(ai,*released)>now)continue;
            const auto probe=ai.ledger.prepare_impact(id,{},true,now);
            if(probe.status==ordinary::Contact::invalid||now<probe.release.arrives)continue;
            const auto target=probe.release.attack.target;auto pose=npc_ordinary_target_locked(target,now);
            const auto player=players_.find(target.actor);
            bool sight=true;
            // Projectile obstruction is tested along the released aim segment,
            // not the source's potentially changed position at impact.
            if(navigation_){const auto& shot=probe.release;
                const auto start=navigation_->locate_cell(shot.plane,{shot.origin_x,shot.origin_y});
                if(!start)sight=false;
                else {const auto line=layered_->trace({shot.plane,*start},{shot.origin_x,shot.origin_y},{shot.aim_x,shot.aim_y});
                    sight=line&&line->plane==shot.plane;}
            }
            auto impact=ai.ledger.prepare_impact(id,pose,sight,now);
            if(impact.status==ordinary::Contact::deferred||impact.status==ordinary::Contact::invalid)continue;
            const auto event=ai.events.find(id);if(event==ai.events.end())throw std::logic_error("NPC ordinary release lacks outcome");
            const auto event_id=event->second;const auto& profile=ai.ledger.profile();
            if(npc_combat_.contains(target.actor)){
                const bool authorized=valid_friendly_ordinary_locked(ai.source,target,false);
                if(impact.status==ordinary::Contact::contact&&authorized){
                    const auto live_target=npc_combat_.find(target.actor);const auto target_body=npcs_.find(target.actor);
                    if(live_target==npc_combat_.end()||target_body==npcs_.end()||live_target->second.hex_scope()!=target)continue;
                    const auto facts=ordinary::opposed_npc_recipient(live,live.lifetime(),live_target->second,live_target->second.lifetime(),
                        target_body->second.x,target_body->second.y,target_body->second.plane,profile.damage_type,now,profile.delivery==ordinary::Delivery::ranged,true);
                    if(!facts||!facts->pose.observed||!facts->armor)continue;
                }
                if(!ai.ledger.commit_impact(impact))continue;ai.events.erase(event);
                const auto outcome=!authorized?AttackOutcome::target_retired:
                    impact.status==ordinary::Contact::retired?AttackOutcome::target_retired:
                    impact.status==ordinary::Contact::obstruction?AttackOutcome::obstruction:
                    impact.status==ordinary::Contact::miss?AttackOutcome::miss:
                    npc_ordinary_npc_contact_locked(ai,impact,event_id,now).value_or(AttackOutcome::cancelled);
                finish_attack_outcome_locked(ai.source,target,event_id,outcome,now);continue;
            }
            if(impact.status==ordinary::Contact::contact&&
               (player==players_.end()||!player->second.resources||!npc_ordinary_armor_locked(player->second,profile.damage_type,false)))continue;
            if(!ai.ledger.commit_impact(impact))continue;ai.events.erase(event);
            auto outcome=impact.status==ordinary::Contact::retired?AttackOutcome::target_retired:
                impact.status==ordinary::Contact::obstruction?AttackOutcome::obstruction:AttackOutcome::miss;
            if(impact.status==ordinary::Contact::contact){auto& p=player->second;
                combat::weapon::Strike strike;strike.level=body.definition.level;strike.minimum=profile.minimum;strike.maximum=profile.maximum;
                strike.mastery=profile.mastery;strike.caster_weapon=profile.caster_weapon;
                // Same explicit private midpoint/no-critical policy as player
                // ordinary_hit_locked; this is not a retail RNG claim.
                strike.base_roll=.5;strike.miss_roll=.5;
                strike.location_armor=conditioned_armor_locked(p,*npc_ordinary_armor_locked(p,profile.damage_type,true),now);
                strike.weakness=live.has_condition(live.lifetime(),combat::conditions::Kind::weakness,now);
                strike.blind=live.has_condition(live.lifetime(),combat::conditions::Kind::blind,now);
                strike.critical_immune=critical_immune_locked(p,now);sample_attack_miss_locked(strike,ai.source,now);
                const auto resolved=combat::weapon::resolve(strike,true); // Preserve actual reviewed NPC level, including zero.
                if(resolved&&!resolved->missed){
                    const auto contact=player_block_contact_locked(p,now,false,ai.source,profile.delivery==ordinary::Delivery::melee,event_id);
                    outcome=contact.value_or(AttackOutcome::cancelled);
                    if(outcome==AttackOutcome::hit){
                        const auto reduced=live.empathy_attack_damage(live.lifetime(),resolved->damage,now);
                        const auto element=combat::weapon::element(profile.damage_type);
                        const auto incoming=reduced?incoming_strike_locked(p,*reduced,now,element):std::nullopt;
                        std::optional<skills::Slot> dazed;
                        const auto qualified=incoming&&preflight_player_dazed_hit_locked(p,now,dazed)?qualify_attack_hit_locked(ai.source,target,event_id,now):std::optional<combat::weapon::QualifiedHit>{};
                        if(qualified){
                            combat::Actor recipient{p.id,p.effect_scope.generation,combat::Health{p.precise_health,p.resources->max_health},p.reviewed_earth_armor,p.effects,true};
                            auto hit=combat::weapon::apply_resolved(recipient,element?*incoming:std::max(0.0,*incoming-p.absorption),true);
                            if(hit){
                                if(element)hit->kind=*element;
                                const auto seed=prepare_player_reactive_seed_locked(p,*hit,now,true);
                                std::optional<combat::Hit> actual;
                                if(!seed||!commit_player_weapon_hit_locked(p,*hit,ai.source.actor,{false,bool(element),true,true},now,actual,*seed,&*qualified))outcome=AttackOutcome::cancelled;
                                else {
                                    const auto loss=actual?actual->applied:0.;
                                    mark_of_pain_reaction_locked(target,profile.damage_type,loss,now);
                                    if(p.precise_health>0)barbs_reaction_locked(target,profile.damage_type,loss,now);
                                    if(p.precise_health<=0)kill_locked(p,now);else commit_dazed_interrupt_locked(p,dazed,now);
                                }
                            }else outcome=AttackOutcome::cancelled;
                        }else outcome=AttackOutcome::cancelled;
                    }
                }
            }
            finish_attack_outcome_locked(ai.source,target,event_id,outcome,now);
        }
    }
    // Returns true when an ordinary windup consumed this controller tick.
    bool drive_npc_ordinary_locked(skills::effects::Scope source,Npc& body,combat::npcs::LiveActor& live,std::uint64_t now,
        const std::function<bool(const combat::ai::ordinary::Windup&)>& replacement={}){
        namespace ordinary=combat::ai::ordinary;
        const auto found=npc_ordinary_ai_.find(source.actor);if(found==npc_ordinary_ai_.end()||found->second.source!=source)return false;
        auto& ai=found->second;
        if(now<live.health_clock(live.lifetime()).value_or(now))return ai.ledger.pending().has_value();
        resolve_npc_ordinary_locked(ai,body,live,now);
        if(!ai.ledger.pending())return false;
        const auto pending=*ai.ledger.pending();const auto life=live.lifetime();
        if(now<live.health_clock(life).value_or(now))return true;
        const auto actor=live.actor(life);const bool alive=actor&&actor->health&&actor->health->current>0&&
            (live.hostile_to_players(life)||valid_friendly_ordinary_locked(source,pending.target));
        const auto epoch=live.control_epoch(life);
        if(!alive||!epoch||*epoch!=pending.token.epoch){
            if(epoch&&*epoch==pending.token.epoch)live.cancel_action(life,now);
            ai.ledger.cancel(now);npc_ordinary_packet_locked(combat::weapon::stop_2022(source.actor));return true;}
        // Host preference may replace an unreleased ordinary windup with an
        // admissible reviewed spell. It never revokes an already released shot.
        if(now<pending.token.completes&&replacement&&replacement(pending))return true;
        auto target=npc_ordinary_target_locked(pending.target,now);const auto victim=players_.find(pending.target.actor);
        const auto refreshed=live.actor(life);const auto refreshed_epoch=live.control_epoch(life);
        if(!refreshed||!refreshed->health||refreshed->health->current<=0||!refreshed_epoch||*refreshed_epoch!=pending.token.epoch){
            ai.ledger.cancel(now);npc_ordinary_packet_locked(combat::weapon::stop_2022(source.actor));return true;}
        bool clear=victim!=players_.end()&&victim->second.effect_scope==pending.target&&ordinary_npc_snapshot_locked(victim->second,body,live,now).line_of_sight;
        if(victim==players_.end()&&valid_friendly_ordinary_locked(source,pending.target)){
            clear=target.observed&&target.plane==body.plane;
            if(clear&&navigation_){const navigation::Point from{body.x,body.y};const auto cell=navigation_->locate_cell(body.plane,from);
                if(!cell)clear=false;else{const auto trace=layered_->trace({body.plane,*cell},from,{target.x,target.y});clear=trace&&trace->plane==body.plane;}}
        }
        const auto proposal=ai.ledger.prepare_release(pending.token,{source,body.x,body.y,body.plane,alive,live.health_clock(life).value_or(now)},target,*epoch,clear,now);
        if(proposal.status!=ordinary::ReleaseStatus::ready&&proposal.status!=ordinary::ReleaseStatus::stopped)return true;
        if(proposal.status==ordinary::ReleaseStatus::stopped){
            if(live.cancel_action(life,now)){ai.ledger.commit_release(proposal,false);npc_ordinary_packet_locked(combat::weapon::stop_2022(source.actor));}return true;
        }
        const auto delivery=ai.ledger.profile().delivery==ordinary::Delivery::ranged?combat::weapon::AttackDelivery::ranged:combat::weapon::AttackDelivery::melee;
        if(!live.complete_action(life,pending.token,now))return true;
        const auto event=begin_attack_outcome_locked(source,pending.target,now,delivery);
        if(!event){ai.ledger.commit_release(proposal,false);npc_ordinary_packet_locked(combat::weapon::stop_2022(source.actor));return true;}
        ai.ledger.commit_release(proposal);ai.events.emplace(pending.id,event);
        npc_ordinary_packet_locked(combat::weapon::finish_2022(source.actor,delivery==combat::weapon::AttackDelivery::ranged));
        if(!vigorous_release_locked(source,now,skills::effects::VigorousActionKind::attack))std::osyncstream(std::clog)<<"WORLD vigorous_npc_release actor="<<source.actor<<" failed\n";
        ordinary_spiteful_release_locked(source,now);resolve_npc_ordinary_locked(ai,body,live,now);return true;
    }
    bool begin_npc_ordinary_locked(combat::ai::FlareController& motion,Npc& body,combat::npcs::LiveActor& live,Player& target,std::uint64_t now){
        const auto found=npc_ordinary_ai_.find(body.id);if(found==npc_ordinary_ai_.end()||found->second.source!=motion.policy.source)return false;
        auto& ai=found->second;const auto life=live.lifetime();const auto& profile=ai.ledger.profile();
        if(ai.ledger.pending()||now<ai.ledger.clock()||!live.ordinary_action_ready(life,now)||live.active_action(life,now)||live.knocked_down(life,now))return false;
        const auto pose=npc_ordinary_target_locked(target.effect_scope,now);if(!pose.alive||pose.clock>now||pose.plane!=body.plane)return false;
        const auto snapshot=ordinary_npc_snapshot_locked(target,body,live,now);const auto point=healing_position_locked(target,now);if(!point)return false;
        if(snapshot.distance>profile.range||!snapshot.line_of_sight){move_npc_flare_locked(motion,body,*point,snapshot.line_of_sight?float(profile.range):0,now);return true;}
        stop_npc_pursuit_locked(motion.policy.source,now);
        // Weapon recovery gates the next swing, not pursuit after a completed
        // or stopped windup. Keep the original cadence and capacity checks at
        // attack admission while allowing movement toward a fleeing target.
        if(!ai.ledger.ready(now))return false;
        if(!npc_ordinary_armor_locked(target,profile.damage_type,false))return false;
        const auto duration=live.attack_duration(life,profile.windup_ms,now),period=live.attack_duration(life,profile.period_ms,now);
        if(!duration||!period||*duration>60000||*period>60000||now>UINT64_MAX-*period)return false;
        const auto token=live.begin_action(life,combat::npcs::control::Action::attack,now,now+*duration);if(!token)return false;
        npc_ordinary_packet_locked(combat::weapon::timing_2022(body.id,float(*period)/1000.f,1.f));
        npc_ordinary_packet_locked(combat::weapon::start_2022(body.id,target.id));
        const bool killed=publish_npc_attack_hexes_locked(body.id,live);
        const bool survives=!killed&&npc_empathy_reaction_locked(ai.source,live,body,now);
        if(!survives||live.control_epoch(life)!=token->epoch){live.cancel_action(life,now);npc_ordinary_packet_locked(combat::weapon::stop_2022(body.id));return true;}
        if(!ai.ledger.accept(target.effect_scope,*token,*period))throw std::logic_error("preflighted NPC ordinary admission changed");
        return true;
    }
public:
    // Explicit opt-in. Mixed actions are captured for hostile casters, but no
    // attack numbers are derived from profession, model, level or skill bar.
    bool configure_npc_ordinary_ai(skills::effects::Scope source,combat::ai::ordinary::Reviewed profile,std::uint64_t now){
        if(!profile.valid())return false;std::lock_guard lock(mutex_);
        const auto live=npc_combat_.find(source.actor);if(live==npc_combat_.end()||live->second.hex_scope()!=source)return false;
        const bool caster=(npc_flare_ai_.contains(source.actor)&&npc_flare_ai_.at(source.actor).policy.source==source)||
            (npc_elemental_ai_.contains(source.actor)&&npc_elemental_ai_.at(source.actor).motion.policy.source==source);
        if(!caster||quest_npc_owners_.contains(source.actor)||quest_npc_motion_.contains(source.actor)||
           !live->second.hostile_to_players(live->second.lifetime())||!live->second.ordinary_action_ready(live->second.lifetime(),now)||
           live->second.active_action(live->second.lifetime(),now))return false;
        const auto old=npc_ordinary_ai_.find(source.actor);
        if(old!=npc_ordinary_ai_.end()&&old->second.source==source)return false;
        npc_ordinary_ai_.insert_or_assign(source.actor,NpcOrdinaryController{source,combat::ai::ordinary::Ledger(source,std::move(profile)),{}});return true;
    }
    std::optional<combat::ai::ordinary::Windup> npc_ordinary_pending(skills::effects::Scope source)const{
        std::lock_guard lock(mutex_);const auto it=npc_ordinary_ai_.find(source.actor);
        return it!=npc_ordinary_ai_.end()&&it->second.source==source?it->second.ledger.pending():std::nullopt;
    }

