// A read-only admission gate for Lightbringer's Gaze. Unknown NPC taxonomy
// remains unknown; neither its model nor its generic creature type is proof.
public:
    bool valid_lightbringers_gaze_primary(
            skills::effects::Scope source,skills::effects::Scope target,
            const skills::effects::LightbringersGazePlan& plan,
            std::uint64_t now)const{
        if(skills::effects::lightbringers_gaze(plan.rank)!=plan)return false;
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        const auto placed=npcs_.find(target.actor);
        const auto live=npc_combat_.find(target.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||caster->second.precise_health<=0||
           placed==npcs_.end()||live==npc_combat_.end()||
           live->second.hex_scope()!=target||
           !live->second.hostile_to_players(live->second.lifetime())||
           skills::effects::lightbringer_servant_trait(
               live->second.lifetime().map,
               live->second.servant_of_abaddon(live->second.lifetime()))!=
               std::optional(true))return false;
        const auto* victim=live->second.actor(live->second.lifetime());
        if(!victim||!victim->damageable||!victim->health||
           victim->health->current<=0||
           placed->second.plane!=caster->second.surface.plane)return false;
        const auto from=healing_position_locked(caster->second,now);
        if(!from||std::hypot(from->x-placed->second.x,
                              from->z-placed->second.y)>plan.range)return false;
        if(navigation_){
            const auto line=layered_->trace(caster->second.surface,*from,
                {placed->second.x,placed->second.y});
            if(!line||line->plane!=placed->second.plane)return false;
        }
        return true;
    }

    // World effect portion of Gaze. Runtime owns payment/cancellation and
    // recharge disabling; this stage retains deferred effect publication.
    std::optional<std::vector<skills::effects::Scope>>
    stage_lightbringers_gaze_world(
            skills::effects::Scope source,skills::effects::Scope primary,
            const skills::effects::LightbringersGazePlan& plan,
            unsigned level,std::uint64_t now,
            std::function<void()>* publication=nullptr){
        if(publication)*publication={};
        if(skills::effects::lightbringers_gaze(plan.rank)!=plan||
           !level||level>255)return {};
        std::lock_guard lock(mutex_);
        const auto caster=players_.find(source.actor);
        if(caster==players_.end()||caster->second.effect_scope!=source||
           !caster->second.resources||caster->second.precise_health<=0||
           caster->second.knocked_down_until>now)return {};
        if(!settle_timed_effects_locked(now))return {};
        const auto selected=npc_combat_.find(primary.actor);
        const auto placed=npcs_.find(primary.actor);
        if(selected==npc_combat_.end()||placed==npcs_.end()||
           selected->second.hex_scope()!=primary||
           !selected->second.hostile_to_players(selected->second.lifetime())||
           skills::effects::lightbringer_servant_trait(
               selected->second.lifetime().map,
               selected->second.servant_of_abaddon(
                   selected->second.lifetime()))!=std::optional(true)||
           placed->second.plane!=caster->second.surface.plane)return {};
        const auto from=healing_position_locked(caster->second,now);
        if(!from||std::hypot(from->x-placed->second.x,
                              from->z-placed->second.y)>plan.range)return {};
        if(navigation_){
            const auto line=layered_->trace(caster->second.surface,*from,
                {placed->second.x,placed->second.y});
            if(!line||line->plane!=placed->second.plane)return {};
        }
        if(!advance_resource_event_locked(caster->second,now)||
           !advance_health_locked(caster->second,now)||
           caster->second.precise_health<=0)return {};
        std::vector<skills::effects::GazeCandidate> candidates;
        for(auto& [id,live]:npc_combat_){
            const auto placement=npcs_.find(id);
            const auto life=live.lifetime();
            if(placement==npcs_.end()||
               life.map!=selected->second.lifetime().map||
               placement->second.plane!=placed->second.plane||
               !live.hostile_to_players(life))continue;
            const auto* actor=live.actor(life);
            if(!actor||!actor->damageable||!actor->health||
               actor->health->current<=0)continue;
            candidates.push_back({live.hex_scope(),
                std::hypot(placement->second.x-placed->second.x,
                           placement->second.y-placed->second.y),
                true,true,skills::effects::lightbringer_servant_trait(
                    life.map,live.servant_of_abaddon(life))});
        }
        for(auto& [id,peer]:players_){
            if(!peer.resources||!caster->second.team_token||
               !peer.team_token||peer.team_token==caster->second.team_token||
               peer.surface.plane!=placed->second.plane||
               peer.precise_health<=0)continue;
            const auto point=healing_position_locked(peer,now);
            if(!point)return {};
            candidates.push_back({peer.effect_scope,
                std::hypot(point->x-placed->second.x,
                           point->z-placed->second.y),
                true,true,std::nullopt});
        }
        const auto impacts=skills::effects::lightbringers_gaze_impacts(
            plan,primary,std::move(candidates));
        if(!impacts)return {};
        // Settle selected recipients before copying any batch authority.
        // An end reaction from a later recipient must not stale an earlier copy.
        for(const auto& impact:*impacts){
            if(const auto found=players_.find(impact.target.actor);found!=players_.end()){
                if(found->second.effect_scope!=impact.target||!advance_health_locked(found->second,now)||found->second.precise_health<=0)return {};
            }else{
                const auto npc_settle=npc_combat_.find(impact.target.actor);if(npc_settle==npc_combat_.end()||npc_settle->second.hex_scope()!=impact.target)return {};
                const auto life=npc_settle->second.lifetime();if(now<npc_settle->second.health_clock(life).value_or(now))return {};
                advance_npc_conditions_locked(impact.target.actor,npc_settle->second,now);
                if(impact.interrupt&&npc_settle->second.active_action(life,now)){
                    const auto stance=npc_settle->second.newest_stance(life,now);
                    if((!stance||stance->skill!=17||npc_settle->second.energy(life))&&!prevent_npc_interrupt_locked(npc_settle->second,now,false))return {};
                }
            }
        }
        if(caster->second.precise_health<=0)return {};
        struct NpcHit {
            skills::effects::Scope target;combat::npcs::LiveActor proposed;combat::Hit hit;
            std::optional<combat::npcs::LiveActor::ReversalDamage> reversal;
            std::optional<combat::npcs::control::Token> interrupted_action;bool protected_action=false;
        };
        struct PlayerHit {skills::effects::Scope target;combat::Damage damage;std::optional<int> holy_armor;};
        std::vector<NpcHit> hits;
        std::vector<PlayerHit> player_hits;
        for(const auto& impact:*impacts){
            if(const auto found=players_.find(impact.target.actor);
               found!=players_.end()){
                auto& victim=found->second;
                if(victim.effect_scope!=impact.target||
                   !victim.resources||!advance_health_locked(victim,now)||
                   victim.precise_health<=0)return {};
                combat::Actor base{impact.target.actor,impact.target.generation,
                    combat::Health{victim.precise_health,
                                   victim.resources->max_health},
                    std::nullopt,victim.effects,true};
                if(victim.reviewed_holy_locations){
                    const auto bucket=armor_location_draw_?armor_location_draw_():
                        std::uniform_int_distribution<unsigned>(0,7)(armor_location_rng_);
                    base.holy_armor=energy::armor_hit(*victim.reviewed_holy_locations,bucket);
                }else base.holy_armor=victim.reviewed_holy_armor;
                const combat::Damage damage{combat::Kind::holy,
                    double(impact.holy_damage),level,1814};
                player_hits.push_back({impact.target,damage,base.holy_armor});
            }
        }
        // Admit all additional player foes as one ordered damage cohort.
        // A first Seed reaction can keep a later foe alive; missing recipient
        // facts must reject before NPC damage, interruption or publication.
        std::map<skills::effects::Scope,skills::effects::HealingSeedReaction> seed_facts;
        bool admitted=false;
        for(std::size_t pass=0;pass<=players_.size()+1;++pass){
            DamageAdmissionBatch batch;batch.seed_facts=&seed_facts;bool valid=true;
            for(const auto& planned:player_hits){
                const auto found=players_.find(planned.target.actor);
                if(found==players_.end()||found->second.effect_scope!=planned.target||!found->second.resources)return {};
                auto& victim=found->second;
                auto* state=admission_player_locked(batch,victim,now);if(!state)return {};
                if(state->health.current<=0)continue;
                combat::Actor base{victim.id,planned.target.generation,
                    combat::Health{state->health.current,state->health.capacity.maximum()},{},victim.effects,true};
                base.holy_armor=planned.holy_armor;
                auto actor=incoming_spell_actor_locked(victim,base,now,true);
                if(!project_batch_player_damage_locked(batch,victim,actor,planned.damage,now)){
                    valid=false;break;
                }
            }
            if(valid){admitted=true;break;}
            if(!batch.missing_seed)return {};
            const auto found=players_.find(batch.missing_seed->actor);
            if(found==players_.end()||found->second.effect_scope!=*batch.missing_seed)return {};
            const auto facts=prepare_healing_seed_direct_locked(found->second,1.,now);
            if(!facts)return {};
            seed_facts[*batch.missing_seed]=*facts;
            // Real resource settlement may run callbacks. Discard the copies
            // and rebuild admission from fresh authority on every retry.
        }
        if(!admitted||caster->second.effect_scope!=source||caster->second.precise_health<=0)return {};
        for(const auto& impact:*impacts){
            if(players_.contains(impact.target.actor))continue;
            auto current=npc_combat_.find(impact.target.actor);
            const auto location=npcs_.find(impact.target.actor);
            if(current==npc_combat_.end()||location==npcs_.end()||
               current->second.hex_scope()!=impact.target)return {};
            const auto life=current->second.lifetime();
            if(const auto clock=current->second.health_clock(life);
               clock&&now<*clock)return {};
            advance_npc_conditions_locked(impact.target.actor,current->second,now);
            const auto action=impact.interrupt?
                current->second.active_action(life,now):
                std::optional<combat::npcs::control::Token>{};
            auto proposed=current->second;
            const combat::Damage damage{combat::Kind::holy,
                double(impact.holy_damage),level,1814};
            std::optional<combat::Hit> actual;std::optional<combat::npcs::LiveActor::ReversalDamage> reversal;
            if(!preflight_npc_reversal_or_damage_locked(proposed,damage,now,nullptr,&actual,&reversal))return {};
            const bool survives=proposed.actor(life)->health->current>0;
            const auto protected_action=action&&survives?prevent_npc_interrupt_locked(current->second,now,false):std::optional(false);
            if(!protected_action)return {};
            if(action&&survives&&!*protected_action&&!proposed.cancel_action(life,now))return {};
            hits.push_back({impact.target,std::move(proposed),actual.value_or(combat::Hit{}),reversal,
                            action&&survives?action:std::nullopt,*protected_action});
        }
        std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> queues;
        std::set<const Outbox*> seen;
        for(auto& [id,peer]:players_)
            if(seen.insert(peer.outbox.get()).second)
                queues.emplace_back(peer.outbox,Outbox::DeferredQueue{});
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        for(auto& [box,queue]:queues)
            captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        std::vector<skills::effects::Scope> struck;
        for(const auto& impact:*impacts)struck.push_back(impact.target);
        // All NPC state is assigned before any downstream World reaction.
        // No prepared copy may be restored after this boundary.
        for(auto& hit:hits){
            auto found=npc_combat_.find(hit.target.actor);
            if(found==npc_combat_.end()||found->second.hex_scope()!=hit.target)throw std::logic_error("Gaze lost prepared NPC scope");
            found->second=std::move(hit.proposed);
            if(hit.reversal)npc_simple_enchantment_credit_.erase({hit.target.actor,hit.reversal->removed.instance});
        }
        // Queue base feedback before any player/death/Bonds reactions, so a
        // later heal is not followed by a stale absolute damage fraction.
        for(const auto& hit:hits){
            const auto packets=hit.reversal?combat::npcs::reversal_damage_packets_2022(hit.target.actor,source.actor,*hit.reversal,reversal_healing_source_locked(hit.target,hit.reversal->removed)):combat::damage_packets_2022(source.actor,hit.hit);
            for(auto& [id,peer]:players_)peer.outbox->enqueue(packets);
            const auto found=npc_combat_.find(hit.target.actor);
            if(found!=npc_combat_.end()&&found->second.hex_scope()==hit.target){
                if(!hit.reversal||hit.reversal->damage)consume_watch_yourself_npc_hit_locked(found->second,hit.hit,true,now);
                if(hit.reversal&&(!hit.reversal->damage||!hit.reversal->damage->killed))for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(hit.target.actor,found->second.actor(found->second.lifetime())->effects)});
            }
        }
        for(const auto& planned:player_hits){
            const auto found=players_.find(planned.target.actor);if(found==players_.end()||found->second.effect_scope!=planned.target||found->second.precise_health<=0)continue;
            auto& victim=found->second;combat::Actor base{victim.id,planned.target.generation,combat::Health{victim.precise_health,victim.resources->max_health},{},victim.effects,true};base.holy_armor=planned.holy_armor;
            auto actor=incoming_spell_actor_locked(victim,base,now,true);const auto hit=combat::apply(actor,planned.damage,true);
            if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(victim,hit,source.actor,now,false,nullptr,false,true,nullptr,source))throw std::logic_error("preflighted Gaze player damage failed");
        }
        for(const auto& hit:hits)if(hit.interrupted_action){
            auto found=npc_combat_.find(hit.target.actor);if(found==npc_combat_.end()||found->second.hex_scope()!=hit.target)continue;
            auto& live=found->second;const auto life=live.lifetime();if(!live.actor(life)->health||live.actor(life)->health->current<=0)continue;
            const auto token=*hit.interrupted_action;
            if(hit.protected_action){
                if(live.active_action(life,now)!=hit.interrupted_action)continue;
                const auto prevented=prevent_npc_interrupt_locked(live,now,true);
                if(!prevented||!*prevented)throw std::logic_error("preflighted Gaze NPC protection failed");
            }else{
                Packet stopped;const auto* row=token.skill?skills::find_skill(token.skill->skill):nullptr;
                if(token.kind==combat::npcs::control::Action::attack&&!token.skill)stopped=combat::weapon::stop_2022(hit.target.actor);
                else{u16(stopped,158);u32(stopped,row&&row->type==14?49:59);u32(stopped,hit.target.actor);u32(stopped,0);}
                for(auto& [id,peer]:players_)peer.outbox->enqueue({stopped});
            }
        }
        for(const auto& hit:hits){
            auto found=npc_combat_.find(hit.target.actor);auto placed_now=npcs_.find(hit.target.actor);
            if(found==npc_combat_.end()||placed_now==npcs_.end()||found->second.hex_scope()!=hit.target)continue;
            // Let the final landed damage trigger Bonds before base-hit death
            // cleanup retires that link. Downstream reactions own their deaths;
            // do not report a surviving Gaze target merely because it died later.
            if(hit.hit.applied>0)bonds_npc_caster_damage_locked(hit.target,hit.hit.applied,now,nullptr);
            const bool killed=hit.reversal?(hit.reversal->damage?hit.reversal->damage->killed:hit.reversal->after_healing.current<=0):hit.hit.killed;
            found=npc_combat_.find(hit.target.actor);placed_now=npcs_.find(hit.target.actor);
            if(killed&&found!=npc_combat_.end()&&placed_now!=npcs_.end()&&found->second.hex_scope()==hit.target&&found->second.actor(found->second.lifetime())->health->current<=0)
                record_npc_death_locked(caster->second,placed_now->second,found->second,now);
        }
        captures.clear();
        auto publish=[queues=std::move(queues)]()mutable{
            for(auto& [box,queue]:queues)box->enqueue(std::move(queue.packets));
            queues.clear();
        };
        defer_or_publish_completion_locked(publication,std::move(publish));
        return struck;
    }
