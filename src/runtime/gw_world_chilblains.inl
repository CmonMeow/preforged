// Mixed-area transaction with an atomic per-recipient production cast batch.
public:
    bool complete_chilblains(skills::effects::Scope source,skills::effects::Scope selected,
            const skills::effects::ChilblainsPlan& plan,unsigned level,std::uint64_t now){
        return stage_chilblains(source,selected,plan,skills::effects::captured_chilblains_order,level,now,nullptr,nullptr,true);
    }
    bool stage_chilblains(skills::effects::Scope source,skills::effects::Scope selected,
            const skills::effects::ChilblainsPlan& plan,unsigned level,std::uint64_t now,
            std::vector<Packet>* visuals=nullptr,std::function<void()>* publication=nullptr){
        return stage_chilblains(source,selected,plan,skills::effects::captured_chilblains_order,level,now,visuals,publication);
    }
    bool stage_chilblains(skills::effects::Scope source,skills::effects::Scope selected,
            const skills::effects::ChilblainsPlan& plan,skills::effects::ChilblainsOrder order,
            unsigned level,std::uint64_t now,std::vector<Packet>* visuals=nullptr,
            std::function<void()>* publication=nullptr,bool publish_completion=false){
        if(publish_completion&&(visuals||publication))return false;
        if(publication)*publication={};
        if(visuals)visuals->clear();const auto canonical=skills::effects::chilblains(plan.rank);
        if(!canonical||*canonical!=plan||!level||level>255||
           (order!=skills::effects::ChilblainsOrder::damage_then_remove&&order!=skills::effects::ChilblainsOrder::remove_then_damage))return false;
        std::lock_guard lock(mutex_);const auto a=players_.find(source.actor),b=players_.find(selected.actor);
        if(a==players_.end()||a->second.effect_scope!=source)return false;
        auto& caster=a->second;
        if(!caster.resources||!caster.team_token||now<caster.health_updated||
           !advance_health_locked(caster,now)||caster.precise_health<=0||caster.knocked_down_until>now)return false;
        const auto origin=healing_position_locked(caster,now);if(!origin)return false;
        double center_x=0,center_z=0;unsigned plane=0;
        if(b!=players_.end()){
            auto& target=b->second;
            if(target.effect_scope!=selected||!target.resources||!target.team_token||caster.team_token==target.team_token||
               now<target.health_updated||!advance_health_locked(target,now)||target.precise_health<=0)return false;
            const auto point=healing_position_locked(target,now);if(!point)return false;
            center_x=point->x;center_z=point->z;plane=target.surface.plane;
        }else{
            auto live=npc_combat_.find(selected.actor);auto body=npcs_.find(selected.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=selected||!live->second.hostile_to_players(live->second.lifetime()))return false;
            advance_npc_conditions_locked(selected.actor,live->second,now);refresh_npc_movement_locked(selected.actor,now);
            live=npc_combat_.find(selected.actor);body=npcs_.find(selected.actor);
            if(live==npc_combat_.end()||body==npcs_.end()||live->second.hex_scope()!=selected)return false;
            const auto* actor=live->second.actor(live->second.lifetime());if(!actor||!actor->health||actor->health->current<=0)return false;
            center_x=body->second.x;center_z=body->second.y;plane=body->second.plane;
        }
        if(plane!=caster.surface.plane||std::hypot(origin->x-center_x,origin->z-center_z)>plan.range)return false;
        if(navigation_){const auto line=layered_->trace(caster.surface,*origin,{float(center_x),float(center_z)});if(!line||line->plane!=plane)return false;}
        std::vector<skills::effects::ChilblainsNpcRecipient> npc_inputs;
        std::vector<unsigned> npc_ids;for(const auto& [id,npc]:npcs_)npc_ids.push_back(id);
        for(auto id:npc_ids){
            auto live=npc_combat_.find(id);auto body=npcs_.find(id);
            if(live==npc_combat_.end()||body==npcs_.end()||!live->second.hostile_to_players(live->second.lifetime()))continue;
            advance_npc_conditions_locked(id,live->second,now);refresh_npc_movement_locked(id,now);
            live=npc_combat_.find(id);body=npcs_.find(id);if(live==npc_combat_.end()||body==npcs_.end())continue;
            if(body->second.plane!=plane)continue;
            const auto distance=std::hypot(body->second.x-center_x,body->second.y-center_z);if(distance>plan.radius)continue;
            const auto* actor=live->second.actor(live->second.lifetime());
            if(actor&&actor->health&&actor->health->current<=0)continue;
            npc_inputs.push_back({live->second,live->second.lifetime(),distance});
        }
        struct NpcCandidate {combat::npcs::Lifetime expected;};
        std::vector<NpcCandidate> npc_candidates;
        const combat::Damage npc_damage{combat::Kind::cold,plan.damage,level,144};
        for(const auto& input:npc_inputs){
            auto projected=input.actor;NpcCandidate candidate{input.expected};
            const auto strip=[&](){
                for(unsigned i=0;i<plan.enchantments;++i){
                    const auto entry=projected.newest_enchantment(input.expected,now);if(!entry)break;
                    if(entry->skill==1392&&order!=skills::effects::captured_chilblains_order)return false;
                    const auto removal=projected.prepare_enchantment_removal(input.expected,entry->instance,now);
                    if(!removal||!projected.commit_enchantment_removal(*removal,now))return false;
                }
                return true;
            };
            if(order==skills::effects::ChilblainsOrder::remove_then_damage&&!strip())return false;
            if(!preflight_npc_reversal_or_damage_locked(projected,npc_damage,now))return false;
            if(order==skills::effects::ChilblainsOrder::damage_then_remove&&!strip())return false;
            npc_candidates.push_back(std::move(candidate));
        }
        const auto poison=skills::effects::chilblains_poison(plan,caster.conditions,source.actor,now);
        if(!poison||caster.next_health_effect>UINT32_MAX-2)return false;
        struct PlayerRemoval {skills::effects::Entry entry;std::uint64_t sequence;};
        const auto removals=[&](const Player& p,skills::effects::Registry registry)
                ->std::optional<std::vector<PlayerRemoval>>{
            std::vector<PlayerRemoval> result;
            for(unsigned i=0;i<plan.enchantments;++i){
                const auto entry=registry.newest(p.effect_scope,skills::effects::Category::enchantment,now);if(!entry)break;
                // Reviewed ends only. The comparison order excludes Watchful
                // and all temporary-Health/end-heal effects: its projection
                // must not invent a Health state from registry erasure alone.
                if(entry->skill!=113&&entry->skill!=214&&entry->skill!=119&&entry->skill!=157&&entry->skill!=180&&entry->skill!=38&&entry->skill!=1392&&entry->skill!=245&&entry->skill!=290&&entry->skill!=310&&entry->skill!=1952&&entry->skill!=2095&&entry->skill!=1399&&entry->skill!=307&&entry->skill!=274)return {};
                if((entry->skill==310||entry->skill==1952||entry->skill==2095)&&!p.health_effects.contains(entry->instance))return {};
                if(entry->skill==290){const auto owner=mending_ownership_.maintainer(p.effect_scope,entry->instance,now);
                    if(!owner||!p.health_regeneration||!p.health_effects.contains(entry->instance)||!skills::effects::prepare_mending_end(*owner,p.effect_scope,entry->instance,now,*p.effect_registry,*p.health_regeneration,mending_ownership_))return {};}
                if((entry->skill==119||entry->skill==157||entry->skill==1392)&&!p.health_effects.contains(entry->instance))return {};
                if(entry->skill==1392){
                    if(order!=skills::effects::captured_chilblains_order)return {};
                    const auto owner=watchful_credit_.find({p.id,entry->instance});
                    if(owner==watchful_credit_.end()||owner->second.target!=p.effect_scope||
                       owner->second.caster.actor!=entry->cause||owner->second.applied!=entry->applied)return {};
                }
                const auto sequence=registry.entry_sequence(p.effect_scope,entry->instance);
                if(!sequence||!registry.remove(p.effect_scope,entry->instance))return {};
                result.push_back({*entry,*sequence});
            }
            return result;
        };
        const auto incoming=[&](const Player& p){auto actor=incoming_spell_actor_locked(p,{p.id,p.effect_scope.generation,
            combat::Health{p.precise_health,p.resources->max_health},{},p.effects,true,{},p.reviewed_fire_armor,p.reviewed_cold_armor},now,true);return combat::apply(actor,npc_damage,true);};
        const auto admit=[&](Player& p){
            const auto selection=[&](const Player& state,const combat::Hit& hit){
                auto registry=*state.effect_registry;
                if(hit.damage>0&&registry.newest_skill(state.effect_scope,307,now)){
                    const auto quote=skills::effects::prepare_reversal_fortune(registry,state.effect_scope,skills::effects::ReversalInput::damage,hit.damage,now);
                    if(!quote||!skills::effects::consume_reversal_fortune(registry,*quote,now))return false;
                }
                return bool(removals(state,std::move(registry)));
            };
            if(order==skills::effects::ChilblainsOrder::damage_then_remove){
                auto hit=incoming(p);
                if(hit.reason!=combat::Reason::accepted||!prepare_player_reactive_seed_locked(p,hit,now))return false;
                // Seed admission can settle other actors and their end reactions.
                // Quote again from current authority before accepting removal.
                hit=incoming(p);
                return hit.reason==combat::Reason::accepted&&preflight_player_reversal_locked(p,hit,now)&&selection(p,hit);
            }
            // The comparison order has only reviewed non-Health-changing ends.
            // Its discarded copy is a read-only protection/Seed admission view.
            const auto project=[&]()->std::optional<Player>{
                auto preview=p;const auto selected=removals(p,*p.effect_registry);if(!selected)return {};
                for(const auto& removal:*selected){
                    preview.effect_registry->remove(preview.effect_scope,removal.entry.instance);
                    preview.health_effects.erase(removal.entry.instance);
                    if(preview.health_regeneration)preview.health_regeneration->remove(preview.effect_scope,removal.entry.instance);
                }
                return preview;
            };
            auto preview=project();if(!preview)return false;
            auto hit=incoming(*preview);
            if(hit.reason!=combat::Reason::accepted||!prepare_player_reactive_seed_locked(*preview,hit,now))return false;
            preview=project();if(!preview)return false;hit=incoming(*preview);
            return hit.reason==combat::Reason::accepted&&preflight_player_reversal_locked(*preview,hit,now);
        };
        std::vector<skills::effects::Scope> candidates;
        for(auto& [id,p]:players_){
            if(!p.resources||!p.team_token||p.team_token==caster.team_token||p.surface.plane!=plane)continue;
            const auto point=healing_position_locked(p,now);
            if(!point)return false;if(std::hypot(point->x-center_x,point->z-center_z)>plan.radius)continue;
            if(now<p.health_updated||!advance_health_locked(p,now)||!p.effect_registry)return false;
            if(p.precise_health<=0)continue;
            if(!admit(p))return false;
            candidates.push_back(p.effect_scope);
        }
        if(candidates.empty()&&npc_candidates.empty())return false;
        struct Publication {
            std::vector<std::pair<std::shared_ptr<Outbox>,Outbox::DeferredQueue>> recipients;
            std::vector<std::size_t> poison_starts;
            std::vector<Packet> poison_announcements;
            std::mutex mutex;
            bool published=false;
            void publish(const std::vector<Packet>& prefix={}){
                std::lock_guard lock(mutex);
                if(published)return;published=true;
                for(std::size_t i=0;i<recipients.size();++i){auto& [box,queue]=recipients[i];
                    // Preserve replacement's old-remove/new-add dependency.
                    // All Health/resource/end-reaction data keeps its order.
                    if(i<poison_starts.size()){
                        std::size_t inserted=0;
                        for(const auto& packet:poison_announcements){
                            const auto found=std::find(queue.packets.begin()+poison_starts[i]+inserted,queue.packets.end(),packet);
                            if(found!=queue.packets.end()){
                                std::rotate(queue.packets.begin()+inserted,found,found+1);++inserted;
                            }
                        }
                    }
                    auto batch=prefix;batch.insert(batch.end(),std::make_move_iterator(queue.packets.begin()),std::make_move_iterator(queue.packets.end()));
                    box->enqueue(std::move(batch));
                }
            }
        };
        std::shared_ptr<Publication> pending;
        std::vector<std::unique_ptr<Outbox::Capture>> captures;
        if(publication||publish_completion){
            pending=std::make_shared<Publication>();pending->recipients.reserve(players_.size());
            std::set<const Outbox*> seen;
            for(auto& [id,peer]:players_)if(seen.insert(peer.outbox.get()).second)pending->recipients.emplace_back(peer.outbox,Outbox::DeferredQueue{});
            for(auto& [box,queue]:pending->recipients)captures.push_back(std::make_unique<Outbox::Capture>(*box,queue));
        }
        std::vector<Packet> body;
        for(const auto scope:candidates){
            const auto current=[&](){const auto p=players_.find(scope.actor);return p!=players_.end()&&p->second.effect_scope==scope&&p->second.resources&&p->second.effect_registry;};
            if(!current()||players_.at(scope.actor).precise_health<=0)continue;
            const auto refused=[&](const char* reason){std::cerr<<"Chilblains late player refusal actor="<<scope.actor<<" reason="<<reason<<std::endl;};
            if(!admit(players_.at(scope.actor))){refused("current_admission");continue;}
            const auto strip=[&](){
                if(!current())return true;
                const auto selected=removals(players_.at(scope.actor),*players_.at(scope.actor).effect_registry);if(!selected)return false;
                for(const auto& removal:*selected){
                    if(!current())return true;
                    auto& p=players_.at(scope.actor);
                    if(p.effect_registry->entry_sequence(scope,removal.entry.instance)!=removal.sequence)continue;
                    remove_health_effect_locked(p,removal.entry.instance,now);
                }
                return true;
            };
            if(order==skills::effects::ChilblainsOrder::remove_then_damage&&!strip()){refused("removal_before_damage");continue;}
            if(!current()||players_.at(scope.actor).precise_health<=0)continue;
            auto& p=players_.at(scope.actor);const auto hit=incoming(p);
            if(hit.reason!=combat::Reason::accepted||!commit_player_reactive_hit_locked(p,hit,source.actor,now,true,nullptr,false,true,nullptr,source)){
                refused("shared_damage");continue;
            }
            const auto target_visual=skills::particles::chilblains_target_2022(scope.actor,source.actor);
            if(publish_completion)for(auto& [id,peer]:players_)peer.outbox->enqueue({target_visual});
            else body.push_back(target_visual);
            if(order==skills::effects::ChilblainsOrder::damage_then_remove&&!strip())refused("removal_after_damage");
            if(!current())continue;
            auto& target=players_.at(scope.actor);
            if(target.precise_health<=0)kill_locked(target,now);else publish_health_locked(target);
        }
        for(const auto& candidate:npc_candidates){
            const auto expected=candidate.expected;
            const auto current=[&](){const auto n=npc_combat_.find(expected.actor);return n!=npc_combat_.end()&&n->second.lifetime()==expected;};
            if(!current())continue;
            const auto strip=[&](){
                // Select at the actual ordering boundary. A consumed307 frees
                // the removal slot; an earlier recipient's callbacks may also
                // have changed which enchantment is newest.
                auto projected=npc_combat_.at(expected.actor);
                std::vector<combat::npcs::LiveActor::EnchantmentRemoval> removals;
                for(unsigned i=0;i<plan.enchantments;++i){
                    const auto entry=projected.newest_enchantment(expected,now);if(!entry)break;
                    if(entry->skill==1392&&order!=skills::effects::captured_chilblains_order)return false;
                    const auto removal=projected.prepare_enchantment_removal(expected,entry->instance,now);
                    if(!removal||!projected.commit_enchantment_removal(*removal,now))return false;
                    removals.push_back(*removal);
                }
                for(const auto& removal:removals){
                    if(!current())return true;
                    auto& live=npc_combat_.at(expected.actor);
                    const auto result=live.commit_enchantment_removal(removal,now);
                    if(!result)return false;
                    if(*result==combat::npcs::LiveActor::EnchantmentRemovalResult::removed&&
                       (removal.entry.skill==1399||removal.entry.skill==307)){
                        npc_simple_enchantment_credit_.erase({expected.actor,removal.entry.instance});
                        for(auto& [id,peer]:players_)peer.outbox->enqueue({health_effect_remove(expected.actor,removal.entry.instance)});
                    }
                    // A consumed/replaced application never retargets another.
                }
                return true;
            };
            const auto refused=[&](const char* reason){std::cerr<<"Chilblains late NPC refusal actor="<<expected.actor<<" reason="<<reason<<std::endl;};
            if(order==skills::effects::ChilblainsOrder::remove_then_damage&&!strip()){refused("removal_before_damage");continue;}
            if(!current())continue;
            auto placed=npcs_.find(expected.actor);const auto owner=players_.find(source.actor);
            if(placed==npcs_.end()||owner==players_.end()||owner->second.effect_scope!=source)continue;
            if(!commit_npc_reversal_or_damage_locked(owner->second,placed->second,npc_combat_.at(expected.actor),npc_damage,now,nullptr,nullptr,nullptr,true)){
                refused("shared_damage");continue;
            }
            if(!current())continue;
            const auto visual=skills::particles::chilblains_target_2022(expected.actor,source.actor);
            if(publish_completion)for(auto& [id,peer]:players_)peer.outbox->enqueue({visual});
            else body.push_back(visual);
            if(order==skills::effects::ChilblainsOrder::damage_then_remove&&!strip())refused("removal_after_damage");
            if(!current())continue;
            auto& live=npc_combat_.at(expected.actor);
            for(auto& [id,peer]:players_)peer.outbox->enqueue({emotes::effects_2022(expected.actor,live.actor(expected)->effects)});
            publish_npc_enchantment_ends_locked(live);
            publish_npc_aura_ends_locked(live);
            publish_npc_simple_enchantment_ends_locked(live);
            if(!current())continue;
            const auto n=npc_combat_.find(expected.actor);placed=npcs_.find(expected.actor);
            const auto credit=players_.find(source.actor);
            if(placed!=npcs_.end()&&n->second.actor(expected)->health&&n->second.actor(expected)->health->current<=0)
                record_npc_death_locked(credit!=players_.end()&&credit->second.effect_scope==source?&credit->second:nullptr,placed->second,n->second,now);
        }
        if(pending)for(const auto& [box,queue]:pending->recipients)pending->poison_starts.push_back(queue.packets.size());
        // Recipient reactions may have changed the caster's conditions. Rebase
        // Poison on current authority instead of restoring the admission copy.
        const auto current_caster=players_.find(source.actor);
        if(current_caster!=players_.end()&&current_caster->second.effect_scope==source&&current_caster->second.precise_health>0){
            auto& owner=current_caster->second;
            const auto current_poison=skills::effects::chilblains_poison(plan,owner.conditions,source.actor,now);
            if(current_poison&&owner.next_health_effect<=UINT32_MAX-2){
                const auto previous=owner.condition_effects.find(combat::conditions::Kind::poison);
                const unsigned previous_id=previous==owner.condition_effects.end()?0:previous->second.instance;
                commit_conditions_locked(owner,*current_poison,{combat::conditions::Kind::poison},now);
                if(pending)if(const auto entry=owner.condition_effects.find(combat::conditions::Kind::poison);entry!=owner.condition_effects.end()){
                    if(previous_id&&previous_id!=entry->second.instance)pending->poison_announcements.push_back(health_effect_remove(owner.id,previous_id));
                    pending->poison_announcements.push_back(health_effect_add(entry->second,owner.id,now));
                }
                if(owner.precise_health<=0)kill_locked(owner,now);
            }else std::cerr<<"Chilblains late self-Poison refusal actor="<<source.actor<<std::endl;
        }
        if(visuals)*visuals=std::move(body);else for(auto& [id,peer]:players_)peer.outbox->enqueue(body);
        captures.clear(); // Prefix events must use the ordinary outbound queues.
        if(publish_completion){
            // The World mutex is still held and each Outbox receives one atomic
            // batch: later World events and other producers cannot split it.
            pending->publish({combat::detail::integer(58,source.actor,0),combat::detail::integer(21,source.actor,256)});
        }else if(publication)defer_or_publish_completion_locked(publication,[pending]{pending->publish();});
        return true;
    }

